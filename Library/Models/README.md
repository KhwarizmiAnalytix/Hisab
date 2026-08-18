# Models (`Library/Models`)

ZABR/SABR teacher pricing and a **QA-trained MLP** that maps a 9-point smile to
(α, ν, ρ). β and γ stay desk-chosen. Production quoting uses the network as a
warm-start, then a short Nelder–Mead polish on the teacher (`polish_iters`,
default 12). Pass `polish_iters=0` for the raw network (microsecond path).

## Layout

- `model/zabr.h` — Hagan SABR Black vol; ZABR via ν_eff = ν α^{γ-1}
- `calibration/zabr_calibrator.h` — Nelder–Mead baseline / polish
- `calibration/qa_calibrator.h` — inverse MLP + optional polish
- `nn/` — MLP runtime and generated `qa_zabr_weights.h` (inverse + pricer)
- `Tools/models/train_qa_zabr.py` — regenerate weights

Training is a two-stage recipe against the same C++ teacher:

1. **Pricer** (well-posed): (α, ν, ρ, β, γ, T, log F) → 9 vols
2. **Inverse**: (9 vols, T, log F, β, γ) → (α, ν, ρ), with parameter MSE
   plus ATM-weighted smile RMSE through the analytic Hagan teacher

Draws mix a desk-like cluster (FX and rates forwards, Beta-distributed
tenors) with a uniform exploration box. The inverse checkpoint is selected
on teacher-smile RMSE, not parameter MAE.

```
python3 Tools/models/train_qa_zabr.py
cd Scripts && python3 setup.py config.build.test --project.models
```
