"""Production-grade QA training for ZABR calibration.

Trains two networks against the same Hagan / first-order ZABR teacher used
by Library/Models/model/zabr.cpp:

1. Pricer  (well-posed): (α, ν, ρ, β, γ, T, log F) → 9 implied vols
2. Inverse (quoting):    (9 vols, T, log F, β, γ) → (α, ν, ρ)

The inverse is trained with parameter MSE **and** ATM-weighted smile RMSE
through the analytic Hagan teacher (finite-difference Jacobian through the
decode map). A companion pricer net is trained on the same smiles for
diagnostics. Draws mix a desk-like cluster with a uniform exploration box;
implausible vols are filtered. Best inverse checkpoint is selected on
teacher-smile RMSE, not parameter MAE.

Writes Library/Models/nn/qa_zabr_weights.h for C++ inference.

Usage (from repo root):
    python3 Tools/models/train_qa_zabr.py
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

import numpy as np

LOG_MONEYNESS = np.array(
    [-0.4, -0.3, -0.2, -0.1, 0.0, 0.1, 0.2, 0.3, 0.4], dtype=np.float64
)
# ATM-heavy quote weights (wider bid/ask in the wings).
STRIKE_WEIGHT = 1.0 / (0.45 + np.abs(LOG_MONEYNESS))
STRIKE_WEIGHT = STRIKE_WEIGHT / STRIKE_WEIGHT.mean()

ATM_REL = 1.0e-12
Z_TINY = 1.0e-8
MIN_POSITIVE = 1.0e-12
RHO_CAP = 0.999999

ALPHA_MIN = 0.04
ALPHA_MAX = 0.48
NU_MIN = 0.10
NU_MAX = 1.60
RHO_MAX = 0.85

HIDDEN = (96, 96)
SEED = 7
PRICER_IN = 7
INV_IN = 13
INV_OUT = 3
SMILE_N = 9


def sabr_black_vol_vec(
    forward: np.ndarray,
    strike: np.ndarray,
    expiry: np.ndarray,
    alpha: np.ndarray,
    beta: np.ndarray,
    rho: np.ndarray,
    nu: np.ndarray,
) -> np.ndarray:
    """Vectorized Hagan (2002) SABR Black vol; matches model/zabr.cpp."""
    forward = np.asarray(forward, dtype=np.float64)
    strike = np.asarray(strike, dtype=np.float64)
    expiry = np.asarray(expiry, dtype=np.float64)
    alpha = np.asarray(alpha, dtype=np.float64)
    beta = np.asarray(beta, dtype=np.float64)
    rho = np.clip(np.asarray(rho, dtype=np.float64), -RHO_CAP, RHO_CAP)
    nu = np.asarray(nu, dtype=np.float64)

    valid = (
        (forward > MIN_POSITIVE)
        & (strike > MIN_POSITIVE)
        & (expiry > MIN_POSITIVE)
        & (alpha > MIN_POSITIVE)
        & (nu > MIN_POSITIVE)
        & (beta >= 0.0)
        & (beta <= 1.0)
        & (np.abs(rho) < 1.0)
    )
    one_m_beta = 1.0 - beta
    f_safe = np.maximum(forward, MIN_POSITIVE)
    k_safe = np.maximum(strike, MIN_POSITIVE)
    a_safe = np.maximum(alpha, MIN_POSITIVE)
    f_pow = np.power(f_safe, one_m_beta)
    fk = f_safe * k_safe
    fk_pow = np.power(fk, 0.5 * one_m_beta)

    def time_corr(fk_1m_beta: np.ndarray, fk_half: np.ndarray) -> np.ndarray:
        return (
            (one_m_beta * one_m_beta / 24.0)
            * alpha
            * alpha
            / np.maximum(fk_1m_beta, MIN_POSITIVE)
            + 0.25 * rho * beta * nu * alpha / np.maximum(fk_half, MIN_POSITIVE)
            + (2.0 - 3.0 * rho * rho) / 24.0 * nu * nu
        )

    atm = np.abs(forward - strike) <= ATM_REL * f_safe
    vol_atm = (alpha / np.maximum(f_pow, MIN_POSITIVE)) * (
        1.0 + time_corr(f_pow * f_pow, f_pow) * expiry
    )

    log_fk = np.log(f_safe / k_safe)
    z = (nu / a_safe) * fk_pow * log_fk
    inner = 1.0 - 2.0 * rho * z + z * z
    inner_ok = inner >= 0.0
    inner_clamped = np.maximum(inner, 0.0)
    numer = np.sqrt(inner_clamped) + z - rho
    denom = 1.0 - rho
    numer_ok = numer > MIN_POSITIVE
    with np.errstate(divide="ignore", invalid="ignore"):
        x = np.log(np.maximum(numer, MIN_POSITIVE) / np.maximum(denom, MIN_POSITIVE))
        x_ok = np.abs(x) > MIN_POSITIVE
        z_over_x = np.where(np.abs(z) > Z_TINY, np.where(x_ok, z / x, np.nan), 1.0)
    log2 = log_fk * log_fk
    geom = (
        1.0
        + (one_m_beta * one_m_beta / 24.0) * log2
        + (np.power(one_m_beta, 4) / 1920.0) * log2 * log2
    )
    vol_k = (
        (alpha / np.maximum(fk_pow * geom, MIN_POSITIVE))
        * z_over_x
        * (1.0 + time_corr(np.power(fk, one_m_beta), fk_pow) * expiry)
    )
    vol = np.where(atm, vol_atm, vol_k)
    off_atm_bad = (~atm) & (np.abs(z) > Z_TINY) & (~(inner_ok & numer_ok & x_ok))
    bad = (~valid) | off_atm_bad | (~np.isfinite(vol)) | (vol <= 0.0)
    return np.where(bad, np.nan, vol)


def zabr_smile_batch(
    forward: np.ndarray,
    expiry: np.ndarray,
    alpha: np.ndarray,
    beta: np.ndarray,
    rho: np.ndarray,
    nu: np.ndarray,
    gamma: np.ndarray,
) -> np.ndarray:
    nu_eff = nu * np.power(np.maximum(alpha, MIN_POSITIVE), gamma - 1.0)
    cols = []
    for k_ln in LOG_MONEYNESS:
        strike = forward * np.exp(k_ln)
        cols.append(
            sabr_black_vol_vec(forward, strike, expiry, alpha, beta, rho, nu_eff)
        )
    return np.stack(cols, axis=1)


def _clip_gauss(
    rng: np.random.Generator, n: int, mean: float, std: float, lo: float, hi: float
) -> np.ndarray:
    return np.clip(mean + std * rng.standard_normal(n), lo, hi)


def sample_params(n: int, rng: np.random.Generator) -> dict[str, np.ndarray]:
    """Mixture: ~65% desk-like cluster, ~35% box exploration."""
    desk = rng.random(n) < 0.65
    n_desk = int(desk.sum())
    n_exp = n - n_desk

    alpha = np.empty(n)
    beta = np.empty(n)
    nu = np.empty(n)
    rho = np.empty(n)
    gamma = np.empty(n)
    expiry = np.empty(n)
    forward = np.empty(n)

    if n_desk:
        alpha[desk] = np.clip(
            rng.lognormal(math.log(0.20), 0.28, n_desk), ALPHA_MIN, ALPHA_MAX
        )
        beta[desk] = _clip_gauss(rng, n_desk, 0.50, 0.12, 0.20, 0.90)
        nu[desk] = np.clip(rng.lognormal(math.log(0.45), 0.35, n_desk), NU_MIN, NU_MAX)
        rho[desk] = _clip_gauss(rng, n_desk, -0.32, 0.22, -RHO_MAX, 0.55)
        gamma[desk] = _clip_gauss(rng, n_desk, 0.88, 0.12, 0.40, 1.00)
        expiry[desk] = 0.25 + 9.75 * rng.beta(2.4, 3.8, n_desk)
        bucket = rng.random(n_desk)
        fd = np.empty(n_desk)
        fx = bucket < 0.75
        rates = (bucket >= 0.75) & (bucket < 0.95)
        fd[fx] = rng.uniform(0.80, 1.25, int(fx.sum()))
        fd[rates] = rng.uniform(0.02, 0.08, int(rates.sum()))
        other = ~(fx | rates)
        fd[other] = rng.uniform(0.60, 1.50, int(other.sum()))
        forward[desk] = fd

    if n_exp:
        alpha[~desk] = rng.uniform(ALPHA_MIN, ALPHA_MAX, n_exp)
        beta[~desk] = rng.uniform(0.25, 0.85, n_exp)
        nu[~desk] = rng.uniform(NU_MIN, NU_MAX, n_exp)
        rho[~desk] = rng.uniform(-RHO_MAX, RHO_MAX, n_exp)
        gamma[~desk] = rng.uniform(0.45, 1.00, n_exp)
        expiry[~desk] = rng.uniform(0.25, 8.0, n_exp)
        forward[~desk] = rng.uniform(0.70, 1.40, n_exp)

    return {
        "alpha": alpha,
        "beta": beta,
        "nu": nu,
        "rho": rho,
        "gamma": gamma,
        "expiry": expiry,
        "forward": forward,
    }


def sample_dataset(
    n: int, rng: np.random.Generator
) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Return inverse features (N,13), params (N,3), pricer features (N,7)."""
    features = []
    params = []
    pricer = []
    batch = max(4096, n // 2)
    attempts = 0
    n_have = 0
    while n_have < n and attempts < 40:
        attempts += 1
        raw = sample_params(batch, rng)
        vols = zabr_smile_batch(
            raw["forward"],
            raw["expiry"],
            raw["alpha"],
            raw["beta"],
            raw["rho"],
            raw["nu"],
            raw["gamma"],
        )
        ok = np.all(np.isfinite(vols), axis=1) & np.all(vols > 0.0, axis=1)
        atm = vols[:, SMILE_N // 2]
        ok &= (atm >= 0.05) & (atm <= 0.65)
        ok &= np.all(vols >= 0.02, axis=1) & np.all(vols <= 1.00, axis=1)
        if not np.any(ok):
            continue
        log_f = np.log(raw["forward"][ok])
        feat = np.concatenate(
            [
                vols[ok],
                raw["expiry"][ok, None],
                log_f[:, None],
                raw["beta"][ok, None],
                raw["gamma"][ok, None],
            ],
            axis=1,
        )
        tgt = np.stack([raw["alpha"][ok], raw["nu"][ok], raw["rho"][ok]], axis=1)
        prc = np.stack(
            [
                raw["alpha"][ok],
                raw["nu"][ok],
                raw["rho"][ok],
                raw["beta"][ok],
                raw["gamma"][ok],
                raw["expiry"][ok],
                log_f,
            ],
            axis=1,
        )
        features.append(feat)
        params.append(tgt)
        pricer.append(prc)
        n_have += feat.shape[0]
    x = np.concatenate(features, axis=0)[:n]
    y = np.concatenate(params, axis=0)[:n]
    p = np.concatenate(pricer, axis=0)[:n]
    if x.shape[0] < n:
        raise RuntimeError(f"only generated {x.shape[0]} valid smiles (wanted {n})")
    if not np.all(np.isfinite(x)) or not np.all(np.isfinite(y)):
        raise RuntimeError("dataset contains non-finite values")
    return x, y, p


def sigmoid(x: np.ndarray) -> np.ndarray:
    return 1.0 / (1.0 + np.exp(-np.clip(x, -40.0, 40.0)))


def logit_unit(p: np.ndarray) -> np.ndarray:
    p = np.clip(p, 1.0e-6, 1.0 - 1.0e-6)
    return np.log(p / (1.0 - p))


def encode_targets(y: np.ndarray) -> np.ndarray:
    a = logit_unit((y[:, 0] - ALPHA_MIN) / (ALPHA_MAX - ALPHA_MIN))
    n = logit_unit((y[:, 1] - NU_MIN) / (NU_MAX - NU_MIN))
    r = np.arctanh(np.clip(y[:, 2] / RHO_MAX, -0.999, 0.999))
    return np.stack([a, n, r], axis=1)


def decode_targets(z: np.ndarray) -> np.ndarray:
    a = ALPHA_MIN + (ALPHA_MAX - ALPHA_MIN) * sigmoid(z[:, 0])
    n = NU_MIN + (NU_MAX - NU_MIN) * sigmoid(z[:, 1])
    r = RHO_MAX * np.tanh(z[:, 2])
    return np.stack([a, n, r], axis=1)


def decode_jacobian(z: np.ndarray) -> np.ndarray:
    """d(α,ν,ρ)/d(z) diagonal, shape (N, 3)."""
    s0 = sigmoid(z[:, 0])
    s1 = sigmoid(z[:, 1])
    t2 = np.tanh(z[:, 2])
    d_a = (ALPHA_MAX - ALPHA_MIN) * s0 * (1.0 - s0)
    d_n = (NU_MAX - NU_MIN) * s1 * (1.0 - s1)
    d_r = RHO_MAX * (1.0 - t2 * t2)
    return np.stack([d_a, d_n, d_r], axis=1)


def cosine_lr(
    epoch: int, n_epochs: int, lr_max: float, lr_min: float = 2.0e-5
) -> float:
    if n_epochs <= 1:
        return lr_max
    frac = epoch / (n_epochs - 1)
    return lr_min + 0.5 * (lr_max - lr_min) * (1.0 + math.cos(math.pi * frac))


class ReluMlp:
    """ReLU MLP with Adam, gradient clipping, and output-gradient backward."""

    def __init__(self, sizes: list[int], rng: np.random.Generator):
        self.sizes = sizes
        self.weights: list[np.ndarray] = []
        self.biases: list[np.ndarray] = []
        for i in range(len(sizes) - 1):
            n_in = sizes[i]
            n_out = sizes[i + 1]
            scale = math.sqrt(2.0 / n_in)
            self.weights.append(rng.normal(0.0, scale, size=(n_out, n_in)))
            self.biases.append(np.zeros(n_out, dtype=np.float64))
        self.m_w = [np.zeros_like(w) for w in self.weights]
        self.v_w = [np.zeros_like(w) for w in self.weights]
        self.m_b = [np.zeros_like(b) for b in self.biases]
        self.v_b = [np.zeros_like(b) for b in self.biases]
        self.t = 0

    def clone_weights(self) -> tuple[list[np.ndarray], list[np.ndarray]]:
        return [w.copy() for w in self.weights], [b.copy() for b in self.biases]

    def load_weights(self, weights: list[np.ndarray], biases: list[np.ndarray]) -> None:
        self.weights = [w.copy() for w in weights]
        self.biases = [b.copy() for b in biases]

    def forward(self, x: np.ndarray) -> tuple[np.ndarray, list[np.ndarray]]:
        acts = [x]
        cur = x
        for i, (w, b) in enumerate(zip(self.weights, self.biases)):
            with np.errstate(over="ignore", invalid="ignore", divide="ignore"):
                cur = cur @ w.T + b
            cur = np.clip(cur, -40.0, 40.0)
            if i + 1 < len(self.weights):
                cur = np.maximum(cur, 0.0)
            acts.append(cur)
        return cur, acts

    def apply_output_grad(
        self, acts: list[np.ndarray], grad: np.ndarray, lr: float, clip: float = 5.0
    ) -> None:
        dw = [None] * len(self.weights)
        db = [None] * len(self.biases)
        g = grad
        with np.errstate(over="ignore", invalid="ignore", divide="ignore"):
            for i in range(len(self.weights) - 1, -1, -1):
                db[i] = np.sum(g, axis=0)
                dw[i] = g.T @ acts[i]
                if i > 0:
                    g = (g @ self.weights[i]) * (acts[i] > 0.0)
        total = 0.0
        for a, b in zip(dw, db):
            if (
                a is None
                or b is None
                or not np.all(np.isfinite(a))
                or not np.all(np.isfinite(b))
            ):
                return
            total += float(np.sum(a * a) + np.sum(b * b))
        total = math.sqrt(total)
        if total > clip and total > 0.0:
            scale = clip / total
            dw = [a * scale for a in dw]
            db = [b * scale for b in db]
        self.t += 1
        beta1, beta2, eps = 0.9, 0.999, 1.0e-8
        for i in range(len(self.weights)):
            self.m_w[i] = beta1 * self.m_w[i] + (1.0 - beta1) * dw[i]
            self.v_w[i] = beta2 * self.v_w[i] + (1.0 - beta2) * (dw[i] * dw[i])
            self.m_b[i] = beta1 * self.m_b[i] + (1.0 - beta1) * db[i]
            self.v_b[i] = beta2 * self.v_b[i] + (1.0 - beta2) * (db[i] * db[i])
            mw_hat = self.m_w[i] / (1.0 - beta1**self.t)
            vw_hat = self.v_w[i] / (1.0 - beta2**self.t)
            mb_hat = self.m_b[i] / (1.0 - beta1**self.t)
            vb_hat = self.v_b[i] / (1.0 - beta2**self.t)
            self.weights[i] -= lr * mw_hat / (np.sqrt(vw_hat) + eps)
            self.biases[i] -= lr * mb_hat / (np.sqrt(vb_hat) + eps)
            self.weights[i] = np.clip(self.weights[i], -25.0, 25.0)
            self.biases[i] = np.clip(self.biases[i], -25.0, 25.0)

    def step_mse(self, x: np.ndarray, y: np.ndarray, lr: float) -> float:
        pred, acts = self.forward(x)
        diff = pred - y
        loss = float(np.mean(diff * diff))
        if not math.isfinite(loss):
            return loss
        grad = (2.0 / x.shape[0]) * diff
        self.apply_output_grad(acts, grad, lr)
        return loss


def standardize(train: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    mean = train.mean(axis=0)
    std = train.std(axis=0)
    std = np.where(std < 1.0e-8, 1.0, std)
    return (train - mean) / std, mean, std


def teacher_smile_rmse(inv_x: np.ndarray, params_hat: np.ndarray) -> float:
    vols = inv_x[:, :SMILE_N]
    expiry = inv_x[:, SMILE_N]
    forward = np.exp(inv_x[:, SMILE_N + 1])
    beta = inv_x[:, SMILE_N + 2]
    gamma = inv_x[:, SMILE_N + 3]
    pred = zabr_smile_batch(
        forward,
        expiry,
        params_hat[:, 0],
        beta,
        params_hat[:, 2],
        params_hat[:, 1],
        gamma,
    )
    ok = np.all(np.isfinite(pred), axis=1)
    if not np.any(ok):
        return 1.0e3
    err = pred[ok] - vols[ok]
    return float(np.mean(np.sqrt(np.mean(err * err, axis=1))))


def format_array(name: str, values: np.ndarray, ctype: str = "double") -> str:
    flat = np.asarray(values, dtype=np.float64).ravel()
    body = ", ".join(f"{v:.12g}" for v in flat)
    return f"inline constexpr {ctype} {name}[] = {{{body}}};\n"


def write_header(
    path: Path,
    inv: ReluMlp,
    pricer: ReluMlp,
    inv_mean: np.ndarray,
    inv_std: np.ndarray,
    prc_mean: np.ndarray,
    prc_std: np.ndarray,
    val_mae: np.ndarray,
    val_rmse: float,
) -> None:
    iw = np.concatenate([w.ravel() for w in inv.weights])
    ib = np.concatenate([b.ravel() for b in inv.biases])
    pw = np.concatenate([w.ravel() for w in pricer.weights])
    pb = np.concatenate([b.ravel() for b in pricer.biases])
    mae_txt = ", ".join(f"{v:.6f}" for v in val_mae)
    text = f"""/*
 * Generated by Tools/models/train_qa_zabr.py — do not edit by hand.
 * Inverse val MAE (α, ν, ρ): {mae_txt}
 * Inverse val teacher-smile RMSE: {val_rmse:.6f}
 */
#pragma once

// Trained coefficients are not mathematical constants; keep clang-tidy off this file.
// NOLINTBEGIN(modernize-use-std-numbers)

namespace models {{
namespace qa_zabr_weights {{

inline constexpr int k_n_features = {inv.sizes[0]};
inline constexpr int k_n_outputs = {inv.sizes[-1]};
inline constexpr int k_n_layers = {len(inv.sizes)};
inline constexpr int k_n_weights = {iw.size};
inline constexpr int k_n_biases = {ib.size};
inline constexpr double k_alpha_min = {ALPHA_MIN};
inline constexpr double k_alpha_max = {ALPHA_MAX};
inline constexpr double k_nu_min = {NU_MIN};
inline constexpr double k_nu_max = {NU_MAX};
inline constexpr double k_rho_max = {RHO_MAX};

{format_array("k_layer_sizes", np.array(inv.sizes, dtype=np.int32), "int")}
{format_array("k_weights", iw)}
{format_array("k_biases", ib)}
{format_array("k_input_mean", inv_mean)}
{format_array("k_input_std", inv_std)}

inline constexpr int k_pricer_n_features = {pricer.sizes[0]};
inline constexpr int k_pricer_n_outputs = {pricer.sizes[-1]};
inline constexpr int k_pricer_n_layers = {len(pricer.sizes)};
inline constexpr int k_pricer_n_weights = {pw.size};
inline constexpr int k_pricer_n_biases = {pb.size};
{format_array("k_pricer_layer_sizes", np.array(pricer.sizes, dtype=np.int32), "int")}
{format_array("k_pricer_weights", pw)}
{format_array("k_pricer_biases", pb)}
{format_array("k_pricer_input_mean", prc_mean)}
{format_array("k_pricer_input_std", prc_std)}

}}  // namespace qa_zabr_weights
}}  // namespace models
// NOLINTEND(modernize-use-std-numbers)
"""
    path.write_text(text)


def repo_root() -> Path:
    here = Path(__file__).resolve()
    for parent in here.parents:
        if (parent / "Library" / "Models").is_dir():
            return parent
    return here.parents[2]


def train_pricer(
    net: ReluMlp,
    x_tr: np.ndarray,
    y_tr: np.ndarray,
    x_val: np.ndarray,
    y_val: np.ndarray,
    epochs: int,
    batch: int,
    lr_max: float,
    rng: np.random.Generator,
) -> ReluMlp:
    best_w, best_b = net.clone_weights()
    best_mae = 1.0e9
    patience = 0
    n = x_tr.shape[0]
    print(f"training pricer {net.sizes} for up to {epochs} epochs...")
    for epoch in range(epochs):
        lr = cosine_lr(epoch, epochs, lr_max)
        perm = rng.permutation(n)
        losses = []
        for start in range(0, n, batch):
            idx = perm[start : start + batch]
            losses.append(net.step_mse(x_tr[idx], y_tr[idx], lr))
        pred = net.forward(x_val)[0]
        mae = float(np.mean(np.abs(pred - y_val)))
        if mae < best_mae - 1.0e-5:
            best_mae = mae
            best_w, best_b = net.clone_weights()
            patience = 0
        else:
            patience += 1
        if (epoch + 1) % 10 == 0 or epoch == 0:
            print(
                f"  pricer epoch {epoch + 1:3d}  loss={np.mean(losses):.6f}  "
                f"val vol MAE={mae:.5f}  lr={lr:.2e}"
            )
        if patience >= 12:
            print(f"  pricer early stop at epoch {epoch + 1}")
            break
    net.load_weights(best_w, best_b)
    print(f"  pricer best val vol MAE={best_mae:.5f}")
    return net


def train_inverse(
    inv: ReluMlp,
    x_tr: np.ndarray,
    y_tr: np.ndarray,
    x_val_raw: np.ndarray,
    x_val: np.ndarray,
    y_val: np.ndarray,
    inv_mean: np.ndarray,
    inv_std: np.ndarray,
    epochs: int,
    batch: int,
    lr_max: float,
    w_param: float,
    w_smile: float,
    rng: np.random.Generator,
) -> ReluMlp:
    z_tr = encode_targets(y_tr)
    best_w, best_b = inv.clone_weights()
    best_rmse = 1.0e9
    patience = 0
    n = x_tr.shape[0]
    print(f"training inverse {inv.sizes} for up to {epochs} epochs...")
    for epoch in range(epochs):
        lr = cosine_lr(epoch, epochs, lr_max)
        perm = rng.permutation(n)
        losses = []
        for start in range(0, n, batch):
            idx = perm[start : start + batch]
            xb = x_tr[idx]
            zb = z_tr[idx]
            raw = xb * inv_std + inv_mean
            vols = raw[:, :SMILE_N]
            expiry = raw[:, SMILE_N]
            forward = np.exp(raw[:, SMILE_N + 1])
            beta = raw[:, SMILE_N + 2]
            gamma = raw[:, SMILE_N + 3]
            z_hat, acts = inv.forward(xb)
            params_hat = decode_targets(z_hat)
            vols_hat = zabr_smile_batch(
                forward,
                expiry,
                params_hat[:, 0],
                beta,
                params_hat[:, 2],
                params_hat[:, 1],
                gamma,
            )
            finite = np.all(np.isfinite(vols_hat), axis=1)
            if not np.any(finite):
                continue
            w = STRIKE_WEIGHT[None, :]
            smile_err = np.where(finite[:, None], (vols_hat - vols) * w, 0.0)
            param_err = z_hat - zb
            n_ok = float(np.count_nonzero(finite))
            loss = w_param * float(np.mean(param_err * param_err)) + w_smile * float(
                np.mean(smile_err * smile_err)
            )
            if not math.isfinite(loss) or n_ok < 1.0:
                continue
            losses.append(loss)
            grad_param = w_param * (2.0 / float(xb.shape[0])) * param_err
            dL_dv = w_smile * (2.0 / n_ok) * smile_err * w
            eps = 1.0e-4
            dL_dp = np.zeros_like(params_hat)
            for i in range(3):
                bumped = params_hat.copy()
                bumped[:, i] += eps
                vols_b = zabr_smile_batch(
                    forward,
                    expiry,
                    bumped[:, 0],
                    beta,
                    bumped[:, 2],
                    bumped[:, 1],
                    gamma,
                )
                dvols = (vols_b - vols_hat) / eps
                dvols = np.where(np.isfinite(dvols), dvols, 0.0)
                dL_dp[:, i] = np.sum(dL_dv * dvols, axis=1)
            grad_smile = dL_dp * decode_jacobian(z_hat)
            inv.apply_output_grad(acts, grad_param + grad_smile, lr)
        pred = decode_targets(inv.forward(x_val)[0])
        mae = np.mean(np.abs(pred - y_val), axis=0)
        rmse = teacher_smile_rmse(x_val_raw, pred)
        if rmse < best_rmse - 1.0e-5:
            best_rmse = rmse
            best_w, best_b = inv.clone_weights()
            patience = 0
        else:
            patience += 1
        if (epoch + 1) % 10 == 0 or epoch == 0:
            mean_loss = float(np.mean(losses)) if losses else float("nan")
            print(
                f"  inverse epoch {epoch + 1:3d}  loss={mean_loss:.5f}  "
                f"val MAE α={mae[0]:.4f} ν={mae[1]:.4f} ρ={mae[2]:.4f}  "
                f"smile RMSE={rmse:.5f}  lr={lr:.2e}"
            )
        if patience >= 18:
            print(f"  inverse early stop at epoch {epoch + 1}")
            break
    inv.load_weights(best_w, best_b)
    print(f"  inverse best teacher-smile RMSE={best_rmse:.5f}")
    return inv


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--samples", type=int, default=64000)
    parser.add_argument("--epochs-pricer", type=int, default=50)
    parser.add_argument("--epochs-inverse", type=int, default=100)
    parser.add_argument("--batch", type=int, default=256)
    parser.add_argument("--lr-pricer", type=float, default=1.5e-3)
    parser.add_argument("--lr-inverse", type=float, default=8.0e-4)
    parser.add_argument("--w-param", type=float, default=0.45)
    parser.add_argument("--w-smile", type=float, default=1.00)
    args = parser.parse_args()

    rng = np.random.default_rng(SEED)
    print(f"sampling {args.samples} ZABR smiles (desk/exploration mixture)...")
    x, y, p = sample_dataset(args.samples, rng)
    n_val = max(1, args.samples // 10)
    if n_val >= args.samples:
        raise RuntimeError(
            f"--samples={args.samples} is too small for a train/val split"
        )
    perm = rng.permutation(x.shape[0])
    x, y, p = x[perm], y[perm], p[perm]
    x_val_raw, y_val, p_val = x[:n_val], y[:n_val], p[:n_val]
    x_tr_raw, y_tr, p_tr = x[n_val:], y[n_val:], p[n_val:]

    x_tr, inv_mean, inv_std = standardize(x_tr_raw)
    x_val = (x_val_raw - inv_mean) / inv_std
    p_tr_n, prc_mean, prc_std = standardize(p_tr)
    p_val_n = (p_val - prc_mean) / prc_std
    vols_tr = x_tr_raw[:, :SMILE_N]
    vols_val = x_val_raw[:, :SMILE_N]

    pricer = ReluMlp([PRICER_IN, *HIDDEN, SMILE_N], rng)
    pricer = train_pricer(
        pricer,
        p_tr_n,
        vols_tr,
        p_val_n,
        vols_val,
        args.epochs_pricer,
        args.batch,
        args.lr_pricer,
        rng,
    )

    inv = ReluMlp([INV_IN, *HIDDEN, INV_OUT], rng)
    inv = train_inverse(
        inv,
        x_tr,
        y_tr,
        x_val_raw,
        x_val,
        y_val,
        inv_mean,
        inv_std,
        args.epochs_inverse,
        args.batch,
        args.lr_inverse,
        args.w_param,
        args.w_smile,
        rng,
    )

    pred = decode_targets(inv.forward(x_val)[0])
    mae = np.mean(np.abs(pred - y_val), axis=0)
    rmse = teacher_smile_rmse(x_val_raw, pred)
    print(f"final inverse val MAE  α={mae[0]:.4f} ν={mae[1]:.4f} ρ={mae[2]:.4f}")
    print(f"final inverse val teacher-smile RMSE={rmse:.5f}")
    if not np.all(np.isfinite(mae)) or not math.isfinite(rmse):
        raise RuntimeError(
            "training produced non-finite validation metrics; refusing to write weights"
        )

    out = repo_root() / "Library" / "Models" / "nn" / "qa_zabr_weights.h"
    write_header(out, inv, pricer, inv_mean, inv_std, prc_mean, prc_std, mae, rmse)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
