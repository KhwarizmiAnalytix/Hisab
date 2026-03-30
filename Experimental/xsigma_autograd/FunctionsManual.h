#pragma once

// NB: Must be quarisma the top of file to avoid including the deprecated "math.h".
// https://stackoverflow.com/questions/6563810/m-pi-works-with-math-h-but-not-with-cmath-in-visual-studio
#ifdef _MSC_VER
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#endif

#include <Quarisma/Quarisma.h>
#include <torch/csrc/autograd/generated/Functions.h>

namespace torch::autograd::generated::details
{

extern const char* kCudnnDoubleBackwardMsg;

// A simple way to imperatively compute index ranges for slots
// that have been flattened
struct TORCH_API IndexRangeGenerator
{
    IndexRange range(size_t range_size)
    {
        i += range_size;
        return {i - range_size, i};
    }
    size_t size() { return i; }

private:
    size_t i = 0;
};

TORCH_API Tensor toNonOptFwGrad(const std::optional<Tensor>& t);
TORCH_API Tensor toNonOptPrimal(const std::optional<Tensor>& t);
TORCH_API Tensor toNonOptTensor(const std::optional<Tensor>& t);

inline std::optional<Tensor> wrap_opt_if(const Tensor& t, const bool cond)
{
    using OptTensor = std::optional<Tensor>;
    return cond ? OptTensor(t) : static_cast<OptTensor>(std::nullopt);
}

TORCH_API Tensor apply_loss_reduction(const Tensor& unreduced, int64_t reduction);
TORCH_API bool   any_variable_defined(const variable_list& variables);
TORCH_API void   copy_range(variable_list& out, IndexRange range, const quarisma::Tensor& t);
TORCH_API void   copy_range(
      variable_list& out, IndexRange range, quarisma::ArrayRef<quarisma::Tensor> t);
TORCH_API quarisma::Tensor copysign_tensor_self_backward(
    const Tensor& grad, const Tensor& self, const Tensor& result);
TORCH_API quarisma::Tensor not_implemented(const char* name, const char* reason = "");
TORCH_API std::vector<Tensor> not_implemented_list(const char* name, const char* reason = "");
quarisma::Tensor              handle_r_to_c(ScalarType self_st, Tensor gradient_result);
quarisma::Tensor              maybe_multiply(const quarisma::Tensor& t, const quarisma::Scalar& s);
int64_t                       _safe_size(IntArrayRef sizes, IntArrayRef dim);
Tensor           restore_reduced_dims(const Tensor& output, IntArrayRef dims, bool keepdim);
Tensor           scale_grad_by_count(const Tensor& grad, const Tensor& mask, IntArrayRef dims);
quarisma::Tensor norm_backward(
    const quarisma::Tensor&                grad,
    const quarisma::Tensor&                self,
    const std::optional<quarisma::Scalar>& p_,
    const quarisma::Tensor&                norm);
quarisma::Tensor norm_backward(
    quarisma::Tensor                       grad,
    const quarisma::Tensor&                self,
    const std::optional<quarisma::Scalar>& p_,
    quarisma::Tensor                       norm,
    quarisma::IntArrayRef                  dim,
    bool                                   keepdim);
Tensor norm_jvp(
    const Tensor&                self_p,
    const Tensor&                self_t,
    const std::optional<Scalar>& p_,
    Tensor                       norm,
    IntArrayRef                  dim,
    bool                         keepdim);
Tensor norm_jvp(
    const Tensor& grad, const Tensor& self, const std::optional<Scalar>& p_, Tensor norm);
Tensor _nested_from_padded_backward(
    const Tensor& grad, const Tensor& input, const bool do_transform_0213);
std::tuple<Tensor, Tensor, Tensor> linear_double_backward(
    const variable_list& grads,
    const Tensor&        self,
    const Tensor&        grad_output,
    const Tensor&        weight);
Tensor linalg_vector_norm_jvp(
    const Tensor&                        self_p,
    const Tensor&                        self_t,
    const Scalar&                        scalar_ord,
    Tensor                               norm,
    const quarisma::OptionalIntArrayRef& opt_dim,
    bool                                 keepdim);
quarisma::Tensor linalg_vector_norm_backward(
    quarisma::Tensor                     grad,
    const quarisma::Tensor&              self,
    const quarisma::Scalar&              ord,
    quarisma::Tensor                     norm,
    const quarisma::OptionalIntArrayRef& opt_dim,
    bool                                 keepdim);
quarisma::Tensor pow_backward(
    quarisma::Tensor grad, const quarisma::Tensor& self, const quarisma::Scalar& exponent_);
quarisma::Tensor pow_backward_self(
    const quarisma::Tensor& grad, const quarisma::Tensor& self, const quarisma::Tensor& exponent);
quarisma::Tensor pow_backward_exponent(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& self,
    const quarisma::Tensor& exponent,
    const quarisma::Tensor& result);
quarisma::Tensor pow_backward_exponent(
    const quarisma::Tensor& grad,
    const quarisma::Scalar& base,
    const quarisma::Tensor& exponent,
    const quarisma::Tensor& result);
quarisma::Tensor angle_backward(const quarisma::Tensor& grad, const quarisma::Tensor& self);
template <typename T>
quarisma::Tensor mul_tensor_backward(const Tensor& grad, T other, ScalarType self_st);
template <typename T>
quarisma::Tensor div_tensor_self_backward(
    const Tensor&                          grad,
    T                                      other,
    ScalarType                             self_st,
    const std::optional<std::string_view>& rounding_mode = std::nullopt);
quarisma::Tensor div_tensor_other_backward(
    const Tensor&                          grad,
    const Tensor&                          self,
    const Tensor&                          other,
    const std::optional<std::string_view>& rounding_mode = std::nullopt);
quarisma::Tensor mvlgamma_backward(
    const quarisma::Tensor& grad, const quarisma::Tensor& self, int64_t p);
quarisma::Tensor permute_backwards(const quarisma::Tensor& grad, quarisma::IntArrayRef fwd_dims);
quarisma::Tensor rad2deg_backward(const quarisma::Tensor& grad);
quarisma::Tensor deg2rad_backward(const quarisma::Tensor& grad);
quarisma::Tensor unsqueeze_multiple(
    const quarisma::Tensor& t, quarisma::OptionalIntArrayRef opt_dim, size_t n_dims);
quarisma::Tensor sum_backward(
    const quarisma::Tensor&       grad,
    quarisma::SymIntArrayRef      sizes,
    quarisma::OptionalIntArrayRef opt_dims,
    bool                          keepdim);
quarisma::Tensor sum_backward(
    const quarisma::Tensor&  grad,
    quarisma::SymIntArrayRef sizes,
    quarisma::IntArrayRef    dims,
    bool                     keepdim);
quarisma::Tensor nansum_backward(
    const quarisma::Tensor&       grad,
    const quarisma::Tensor&       self,
    quarisma::OptionalIntArrayRef dims,
    bool                          keepdim);
std::vector<int64_t>          reverse_list(const quarisma::IntArrayRef list);
std::vector<quarisma::SymInt> reverse_list_symint(const quarisma::SymIntArrayRef list);
quarisma::Tensor              reverse_dim(const quarisma::Tensor& t, int64_t dim);
quarisma::Tensor              prod_safe_zeros_backward(
                 const quarisma::Tensor& grad, const quarisma::Tensor& inp, int64_t dim);
quarisma::Tensor prod_backward(
    const quarisma::Tensor& grad, const quarisma::Tensor& input, const quarisma::Tensor& result);
quarisma::Tensor prod_backward(
    quarisma::Tensor        grad,
    const quarisma::Tensor& input,
    quarisma::Tensor        result,
    int64_t                 dim,
    bool                    keepdim);
quarisma::Tensor solve_jvp(const Tensor& X, const Tensor& A, const Tensor& dA, const Tensor& dB);
quarisma::Tensor solve_backward_self(
    const quarisma::Tensor& grad, const quarisma::Tensor& self, const quarisma::Tensor& A);
quarisma::Tensor solve_backward_A(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& self,
    const quarisma::Tensor& A,
    const quarisma::Tensor& solution);
quarisma::Tensor cumsum_backward(const quarisma::Tensor& grad, int64_t dim);
quarisma::Tensor logsumexp_backward(
    quarisma::Tensor        grad,
    const quarisma::Tensor& self,
    quarisma::Tensor        result,
    quarisma::IntArrayRef   dim,
    bool                    keepdim);
quarisma::Tensor logsumexp_jvp(
    const quarisma::Tensor& self_p, const quarisma::Tensor& self_t, IntArrayRef dim, bool keepdim);
quarisma::Tensor safe_logsumexp_jvp(
    const quarisma::Tensor& self_p, const quarisma::Tensor& self_t, IntArrayRef dim, bool keepdim);
quarisma::Tensor logcumsumexp_backward(
    quarisma::Tensor        grad,
    const quarisma::Tensor& self,
    const quarisma::Tensor& result,
    int64_t                 dim);
quarisma::Tensor logcumsumexp_jvp(
    const quarisma::Tensor& self_p, const quarisma::Tensor& self_t, int64_t dim);
quarisma::Tensor unbind_backward(const variable_list& grads, int64_t dim);
quarisma::Tensor unbind_backward_nested(
    const variable_list&           grads,
    const Tensor&                  nt_sizes,
    int64_t                        dim,
    const quarisma::TensorOptions& options);
quarisma::Tensor unbind_backward_nested_jagged(
    const variable_list& grads, const Tensor& self, int64_t dim);
quarisma::Tensor unsqueeze_to(const quarisma::Tensor& self, quarisma::SymIntArrayRef sym_sizes);
quarisma::Tensor unsqueeze_to(
    const quarisma::Tensor& self, int64_t dim, quarisma::SymIntArrayRef sym_sizes);
quarisma::Tensor unsqueeze_to(
    const quarisma::Tensor& self, IntArrayRef dim, quarisma::SymIntArrayRef sym_sizes);
std::vector<quarisma::Tensor> cat_tensors_backward(
    const quarisma::Tensor&                           grad,
    const std::vector<std::vector<quarisma::SymInt>>& sizes,
    const std::vector<ScalarType>&                    dtypes,
    int64_t                                           dim);
std::vector<quarisma::Tensor> stack_tensors_backward(
    const quarisma::Tensor& grad, int64_t dim, const std::vector<ScalarType>& dtypes);
std::vector<quarisma::Tensor> block_diag_backward(
    const quarisma::Tensor&                  grad,
    const std::vector<std::vector<int64_t>>& sizes,
    const std::vector<ScalarType>&           dtypes);
quarisma::Tensor clamp_backward(
    const quarisma::Tensor&                grad,
    const quarisma::Tensor&                self,
    const std::optional<quarisma::Scalar>& min,
    const std::optional<quarisma::Scalar>& max);
quarisma::Tensor clamp_backward(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& self,
    const quarisma::Tensor& min,
    const quarisma::Tensor& max);
std::tuple<quarisma::Tensor, quarisma::Tensor> clamp_backward_min_max(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& self,
    const quarisma::Tensor& min,
    const quarisma::Tensor& max,
    const std::array<bool, 2>& /*grad_input_mask*/);
quarisma::Tensor clamp_jvp(
    const Tensor& self_p,
    const Tensor& self_t,
    const Tensor& min_p,
    const Tensor& min_t,
    const Tensor& max_p,
    const Tensor& max_t);
quarisma::SymIntArrayRef strides_or_error(const Tensor& input, std::string_view const& input_name);
quarisma::Tensor         mm_mat1_backward(
            const Tensor&            grad,
            const Tensor&            mat2,
            quarisma::SymIntArrayRef mat1_sizes,
            quarisma::SymIntArrayRef mat1_strides,
            quarisma::Layout         mat1_layout,
            const Scalar&            alpha);
quarisma::Tensor mm_mat2_backward(
    const quarisma::Tensor&  grad,
    const quarisma::Tensor&  mat1,
    quarisma::SymIntArrayRef sizes,
    quarisma::SymIntArrayRef strides,
    quarisma::Layout         layout,
    const quarisma::Scalar&  alpha);
quarisma::Tensor _grouped_mm_mat1_backward(
    const Tensor&            grad,
    const Tensor&            mat2,
    quarisma::SymIntArrayRef mat1_sizes,
    quarisma::SymIntArrayRef mat1_strides,
    quarisma::Layout         mat1_layout,
    std::optional<Tensor>    offs,
    const Scalar&            alpha);
quarisma::Tensor _grouped_mm_mat2_backward(
    const quarisma::Tensor&  grad,
    const quarisma::Tensor&  mat1,
    quarisma::SymIntArrayRef sizes,
    quarisma::SymIntArrayRef strides,
    quarisma::Layout         layout,
    std::optional<Tensor>    offs,
    const quarisma::Scalar&  alpha);
quarisma::Tensor mm_mat1_sparse_backward(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& mat1,
    const quarisma::Tensor& mat2,
    const quarisma::Scalar& alpha);
std::tuple<Tensor, Tensor, Tensor> sparse_sampled_addmm_backward(
    const Tensor&                grad,
    const Tensor&                self,
    const std::optional<Tensor>& mat1,
    const std::optional<Tensor>& mat2,
    const Scalar&                alpha,
    const Scalar&                beta,
    const std::array<bool, 3>&   grad_input_mask);
quarisma::Tensor sparse_mask_backward(
    const quarisma::Tensor& grad, const quarisma::Tensor& mask, quarisma::Layout self_layout);
quarisma::Tensor sparse_sparse_matmul_backward(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& mat1,
    const quarisma::Tensor& mat2,
    int64_t                 grad_order);
quarisma::Tensor renorm_backward(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& self,
    const quarisma::Scalar& p,
    int64_t                 dim,
    const quarisma::Scalar& maxnorm);
quarisma::Tensor renorm_jvp(
    const quarisma::Tensor& self_p,
    const quarisma::Tensor& self_t,
    const quarisma::Scalar& p,
    int64_t                 dim,
    const quarisma::Scalar& maxnorm);
quarisma::Tensor repeat_backward(
    quarisma::Tensor grad, quarisma::SymIntArrayRef repeats, quarisma::SymIntArrayRef input_shape);
quarisma::Tensor _fused_dropout_backward(
    const quarisma::Tensor& grad, const quarisma::Tensor& mask, double p1m);
quarisma::Tensor infinitely_differentiable_native_dropout_backward(
    const quarisma::Tensor& grad, const quarisma::Tensor& mask, double scale);
quarisma::Tensor native_dropout_double_backward(
    const quarisma::Tensor& ggI,
    const quarisma::Tensor& grad,
    const quarisma::Tensor& mask,
    double                  scale);
quarisma::Tensor evenly_distribute_backward(
    const quarisma::Tensor& grad, const quarisma::Tensor& input, const quarisma::Tensor& value);
Tensor           sgn_backward(const Tensor& x, const Tensor& gx, const Tensor& sgn);
Tensor           masked_fill_backward(const Tensor& grad, const Tensor& mask);
quarisma::Tensor var_backward(
    quarisma::Tensor                       grad,
    const quarisma::Tensor&                self,
    quarisma::OptionalIntArrayRef          dim,
    const std::optional<quarisma::Scalar>& correction,
    bool                                   keepdim);
quarisma::Tensor var_jvp(
    const quarisma::Tensor&                self_t,
    const quarisma::Tensor&                self_p,
    const quarisma::Tensor&                result,
    quarisma::OptionalIntArrayRef          dim_opt,
    const std::optional<quarisma::Scalar>& correction,
    bool                                   keepdim);
quarisma::Tensor std_backward(
    const quarisma::Tensor&                result,
    const quarisma::Tensor&                grad,
    const quarisma::Tensor&                self,
    quarisma::OptionalIntArrayRef          dim,
    const std::optional<quarisma::Scalar>& correction,
    bool                                   keepdim);
Tensor mean_backward(
    const Tensor&                 grad,
    quarisma::SymIntArrayRef      shape,
    quarisma::OptionalIntArrayRef opt_dim,
    quarisma::SymInt              numel,
    bool                          keepdim);
Tensor var_mean_backward(
    const Tensor&                          gvar,
    const Tensor&                          gmean,
    const Tensor&                          self,
    quarisma::OptionalIntArrayRef          dim_opt,
    const std::optional<quarisma::Scalar>& correction,
    bool                                   keepdim);
Tensor std_mean_backward(
    const Tensor&                          gstd,
    const Tensor&                          gmean,
    const Tensor&                          self,
    const Tensor&                          std,
    quarisma::OptionalIntArrayRef          dim_opt,
    const std::optional<quarisma::Scalar>& correction,
    bool                                   keepdim);
quarisma::Tensor cholesky_backward(
    const quarisma::Tensor& grad, bool upper, const quarisma::Tensor& L);
quarisma::Tensor cholesky_jvp(
    const quarisma::Tensor& input_tangent, const quarisma::Tensor& L, bool upper);
quarisma::Tensor cholesky_inverse_backward(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& L,
    bool                    upper,
    const quarisma::Tensor& inverse);
quarisma::Tensor cholesky_inverse_jvp(
    const quarisma::Tensor& F, const quarisma::Tensor& dF, const quarisma::Tensor& X, bool upper);
Tensor pinv_jvp(const Tensor& A, const Tensor& pinvA, const Tensor& dA);
Tensor pinv_backward(const Tensor& grad, const Tensor& pinvA, const Tensor& A);
Tensor chunk_backward_nested(
    const std::vector<torch::autograd::Variable>& grads,
    const Tensor&                                 self,
    int64_t                                       chunks,
    int64_t                                       dim);
quarisma::Tensor split_with_sizes_backward(
    const std::vector<torch::autograd::Variable>& grads,
    quarisma::SymIntArrayRef                      split_sizes,
    int64_t                                       dim,
    quarisma::SymIntArrayRef                      sizes,
    const quarisma::TensorOptions&                options);
quarisma::Tensor _nested_split_with_sizes_backward(
    const std::vector<torch::autograd::Variable>& grads,
    quarisma::SymIntArrayRef                      split_sizes,
    int64_t                                       dim,
    const Tensor&                                 nt_sizes,
    const quarisma::TensorOptions&                options);
quarisma::Tensor split_backward(
    const std::vector<torch::autograd::Variable>& grads,
    const quarisma::SymInt&                       split_size,
    int64_t                                       dim,
    quarisma::SymIntArrayRef                      sizes,
    const quarisma::TensorOptions&                options);
quarisma::Tensor max_pool_double_backward(
    const quarisma::Tensor& grad, const quarisma::Tensor& indices, int dim);
quarisma::Tensor error_for_max_pool2d_double_backward();
quarisma::Tensor glu_double_backward(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& grad_output,
    const quarisma::Tensor& input,
    int64_t                 dim);
quarisma::Tensor glu_double_backward_grad_output(
    const quarisma::Tensor& grad, const quarisma::Tensor& input, int64_t dim);
quarisma::Tensor infinitely_differentiable_silu_backward(
    const quarisma::Tensor& grad_output, const quarisma::Tensor& input);
quarisma::Tensor infinitely_differentiable_mish_backward(
    const quarisma::Tensor& grad_output, const quarisma::Tensor& input);
Tensor infinitely_differentiable_logit_backward(
    const Tensor& grad, const Tensor& self, std::optional<double> eps);
Tensor binary_cross_entropy_target_backward(
    const Tensor&                grad,
    const Tensor&                self,
    const Tensor&                target,
    const std::optional<Tensor>& weight,
    int64_t                      reduction);
Tensor binary_cross_entropy_double_backward_target(
    const Tensor&                grad,
    const Tensor&                grad_output,
    const Tensor&                self,
    const Tensor&                target,
    const std::optional<Tensor>& weight,
    int64_t                      reduction);
Tensor binary_cross_entropy_with_logits_backward(
    const Tensor&                grad,
    const Tensor&                input,
    const Tensor&                target,
    const std::optional<Tensor>& weight_opt,
    const std::optional<Tensor>& pos_weight_opt,
    int64_t                      reduction);
quarisma::Tensor binary_cross_entropy_with_logits_target_backward(
    const quarisma::Tensor&                grad_output,
    const quarisma::Tensor&                self,
    const quarisma::Tensor&                target,
    const std::optional<quarisma::Tensor>& weight,
    const std::optional<quarisma::Tensor>& pos_weight,
    int64_t                                reduction);
quarisma::Tensor log_sigmoid_double_backward(
    const quarisma::Tensor& grad, const quarisma::Tensor& input);
quarisma::Tensor softmax_double_backward(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& grad_output,
    int                     dim,
    const quarisma::Tensor& output);
quarisma::Tensor binary_cross_entropy_double_backward(
    const quarisma::Tensor&                grad_output,
    const quarisma::Tensor&                grad,
    const quarisma::Tensor&                input,
    const quarisma::Tensor&                target,
    const std::optional<quarisma::Tensor>& weight,
    int64_t                                reduction);
quarisma::Tensor binary_cross_entropy_double_backward_grad_output(
    const quarisma::Tensor&                grad,
    const quarisma::Tensor&                input,
    const quarisma::Tensor&                target,
    const std::optional<quarisma::Tensor>& weight,
    int64_t                                reduction);
quarisma::Tensor smooth_l1_loss_double_backward(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& input,
    const quarisma::Tensor& target,
    int64_t                 reduction,
    double                  beta);
quarisma::Tensor huber_loss_double_backward(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& input,
    const quarisma::Tensor& target,
    int64_t                 reduction,
    double                  delta);
quarisma::Tensor huber_loss_double_backward_grad_output(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& grad_output,
    const quarisma::Tensor& input,
    const quarisma::Tensor& target,
    int64_t                 reduction,
    double                  delta);
quarisma::Tensor mse_loss_double_backward(
    const quarisma::Tensor& grad, const quarisma::Tensor& input, int64_t reduction);
quarisma::Tensor soft_margin_loss_double_backward(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& input,
    const quarisma::Tensor& target,
    int64_t                 reduction);
quarisma::Tensor soft_margin_loss_double_backward_grad_output(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& grad_output,
    const quarisma::Tensor& input,
    const quarisma::Tensor& target,
    int64_t                 reduction);
quarisma::Tensor softplus_double_backward(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& input,
    const quarisma::Scalar& beta,
    const quarisma::Scalar& threshold);
std::tuple<quarisma::Tensor, quarisma::Tensor> slogdet_jvp(
    const quarisma::Tensor& LU,
    const quarisma::Tensor& pivots,
    const quarisma::Tensor& dA,
    const quarisma::Tensor& sign,
    const bool              use_A_T);
quarisma::Tensor slogdet_backward(
    const quarisma::Tensor& grad_sign,
    const quarisma::Tensor& grad_logabsdet,
    const quarisma::Tensor& A,
    const quarisma::Tensor& signdet,
    const quarisma::Tensor& LU,
    const quarisma::Tensor& pivots);
quarisma::Tensor log1p_backward(const quarisma::Tensor& grad, const quarisma::Tensor& self);
quarisma::Tensor sinc_backward(const quarisma::Tensor& grad, const quarisma::Tensor& self);
quarisma::Tensor sparse_constructor_values_backward(
    const quarisma::Tensor& sparse_grad_out, const quarisma::Tensor& indices);
quarisma::Tensor embedding_dense_double_backward_symint(
    const quarisma::Tensor& grad,
    const quarisma::Tensor& indices,
    const quarisma::SymInt& padding_idx);
quarisma::Tensor index_backward(
    quarisma::Tensor                          zeros_like_self,
    const torch::List<std::optional<Tensor>>& indices,
    const quarisma::Tensor&                   grad);
quarisma::Tensor _cudnn_ctc_loss_backward(
    const quarisma::Tensor& grad_out,
    const quarisma::Tensor& loss,
    const quarisma::Tensor& raw_grad,
    bool                    zero_infinity);
quarisma::Tensor elu_double_backward(
    const Tensor& grad,
    const Tensor& grad_output,
    const Scalar& alpha,
    const Scalar& scale,
    const Scalar& input_scale,
    bool          is_result,
    const Tensor& self_or_result);

Tensor svd_backward(
    const Tensor& gU,
    const Tensor& gS,
    const Tensor& gVh,
    const Tensor& U,
    const Tensor& S,
    const Tensor& Vh);

std::tuple<Tensor, Tensor, Tensor> linalg_svd_jvp(
    const Tensor& dA, const Tensor& U, const Tensor& S, const Tensor& Vh, const bool full_matrices);
Tensor slice_backward_wrapper(
    const quarisma::Tensor&         grad,
    const quarisma::SymIntArrayRef& input_sizes,
    int64_t                         dim,
    std::optional<quarisma::SymInt> start,
    std::optional<quarisma::SymInt> end,
    quarisma::SymInt                step);
std::tuple<Tensor, Tensor> linalg_eig_jvp(
    const Tensor& dA, const Tensor& L, const Tensor& V, const bool is_hermitian);
Tensor linalg_eig_backward(
    const Tensor& gL,
    const Tensor& gV,
    const Tensor& L,
    const Tensor& V,
    const bool    is_hermitian,
    const bool    symeig_eigenvectors = true);
Tensor linalg_lstsq_solution_jvp(
    const Tensor& A, const Tensor& B_, const Tensor& dA, const Tensor& dB_);
Tensor linalg_lstsq_residuals_jvp(
    const Tensor& A,
    const Tensor& B_,
    const Tensor& dA,
    const Tensor& dB_,
    const Tensor& X_,
    const Tensor& L);
std::tuple<Tensor, Tensor> triangular_solve_backward(
    const Tensor&       grad_x,
    const Tensor&       grad_m,
    const Tensor&       b,
    const Tensor&       a,
    const Tensor&       x,
    const bool          upper,
    const bool          transpose,
    const bool          unitriangular,
    std::array<bool, 2> output_mask);
Tensor triangular_solve_jvp(
    const Tensor& X,
    const Tensor& A,
    const Tensor& dA,
    const Tensor& dB,
    const bool    upper,
    const bool    transpose,
    const bool    unitriangular);
Tensor linalg_solve_triangular_forward_AD(
    const Tensor& A_t,
    const Tensor& B_t,
    const Tensor& A,
    const Tensor& X,
    const bool    upper,
    const bool    left,
    const bool    unitriangular);
std::tuple<Tensor, Tensor> linalg_solve_triangular_backward(
    const Tensor&       grad,
    const Tensor&       A,
    const Tensor&       X,
    const bool          upper,
    const bool          left,
    const bool          unitriangular,
    std::array<bool, 2> output_mask);
std::tuple<Tensor, Tensor, Tensor> _trilinear_backward(
    const Tensor&                grad_out,
    const std::optional<Tensor>& i1,
    const std::optional<Tensor>& i2,
    const std::optional<Tensor>& i3,
    IntArrayRef                  expand1,
    IntArrayRef                  expand2,
    IntArrayRef                  expand3,
    IntArrayRef                  sumdim,
    std::array<bool, 3>          grad_mask);
std::tuple<Tensor, Tensor> linalg_qr_jvp(
    const Tensor& dA, const Tensor& Q, const Tensor& R, const std::string_view mode);
Tensor linalg_qr_backward(
    const Tensor&          gQ,
    const Tensor&          gR,
    const Tensor&          Q,
    const Tensor&          R,
    const std::string_view mode);
Tensor linalg_matrix_exp_differential(const Tensor& self, const Tensor& grad, bool adjoint);
std::tuple<Tensor, Tensor, Tensor> batchnorm_double_backward(
    const Tensor&                input,
    const std::optional<Tensor>& gamma,
    const Tensor&                ggI,
    const Tensor&                ggG,
    const Tensor&                ggB,
    const Tensor&                gO,
    const std::optional<Tensor>& running_mean,
    const std::optional<Tensor>& running_var,
    bool                         training,
    double                       eps,
    const std::optional<Tensor>& save_mean,
    const std::optional<Tensor>& save_invstd,
    std::array<bool, 3>          output_mask);
std::tuple<Tensor, Tensor> _euclidean_dist_backward(
    const Tensor& grad, const Tensor& x1, const Tensor& x2, const Tensor& res);
Tensor fft_backward(
    const Tensor& self,
    const Tensor& grad,
    int64_t       signal_ndim,
    bool          complex_input,
    bool          complex_output,
    bool          inverse,
    IntArrayRef   checked_signal_sizes,
    int64_t       normalization,
    bool          onesided,
    IntArrayRef   output_sizes);
Tensor fft_r2c_backward(
    const Tensor&           grad,
    quarisma::IntArrayRef   dim,
    int64_t                 normalization,
    bool                    onesided,
    const quarisma::SymInt& last_dim_size);
Tensor fft_c2r_backward(const Tensor& grad, IntArrayRef dim, int64_t normalization);
Tensor constant_pad_nd_backward(const Tensor& grad, quarisma::SymIntArrayRef pad);
std::tuple<Tensor, Tensor> cholesky_solve_backward(
    const Tensor&       grad_x,
    const Tensor&       self,
    const Tensor&       input2,
    const Tensor&       result,
    const bool          upper,
    std::array<bool, 2> output_mask);
Tensor cholesky_solve_jvp(
    const Tensor& X, const Tensor& U, const Tensor& dU, const Tensor& dB, const bool upper);
std::tuple<Tensor, Tensor, Tensor> infinitely_differentiable_native_group_norm_backward(
    const Tensor&                dY,
    const Tensor&                dmean,
    const Tensor&                drstd,
    const Tensor&                X,
    const Tensor&                mean,
    const Tensor&                rstd,
    const std::optional<Tensor>& gamma,
    quarisma::SymInt             N,
    const quarisma::SymInt&      C,
    quarisma::SymInt             HxW,
    int64_t                      group,
    double                       eps,
    std::array<bool, 3>          grad_input_mask);
Tensor gelu_double_backward(
    const Tensor& ggI, const Tensor& gO, const Tensor& input, std::string_view approximate);
Tensor as_strided_backward(
    Tensor                                 grad,
    const TensorGeometry&                  input_geometry,
    quarisma::SymIntArrayRef               sizes,
    quarisma::SymIntArrayRef               strides,
    const std::optional<quarisma::SymInt>& storage_offset_);
Tensor as_strided_scatter_backward(
    const Tensor&                   grad,
    const TensorGeometry&           input_geometry,
    const TensorGeometry&           src_geometry,
    quarisma::SymIntArrayRef        sizes,
    quarisma::SymIntArrayRef        strides,
    std::optional<quarisma::SymInt> storage_offset);
std::tuple<Tensor, Tensor> atan2_backward(
    const Tensor& grad, const Tensor& self, const Tensor& other, std::array<bool, 2> output_mask);
Tensor amaxamin_jvp(
    const Tensor& x, const Tensor& dx, const Tensor& result, IntArrayRef dim, bool keepdim);
std::tuple<Tensor, Tensor, Tensor> layer_norm_double_backward(
    const Tensor&                input,
    const std::optional<Tensor>& gamma,
    const Tensor&                ggI,
    const Tensor&                ggG,
    const Tensor&                ggB,
    const Tensor&                gO,
    const Tensor&                save_mean,
    const Tensor&                save_invstd,
    quarisma::SymIntArrayRef     normalized_shape,
    std::array<bool, 3>          output_mask);

std::tuple<Tensor, Tensor> infinitely_differentiable_native_rms_norm_backward(
    const Tensor&                dY,
    const Tensor&                drstd,
    const Tensor&                input,
    IntArrayRef                  normalized_shape,
    const Tensor&                rstd,
    const std::optional<Tensor>& weight_opt,
    std::array<bool, 2>          grad_input_mask);

std::tuple<Tensor, Tensor> householder_product_backward(
    const Tensor& grad,
    const Tensor& result,
    const Tensor& input,
    const Tensor& tau,
    const bool    flip_order = false);
Tensor householder_product_jvp(
    const Tensor& dV, const Tensor& dtau, const Tensor& prod, const Tensor& V, const Tensor& tau);
std::tuple<Tensor, Tensor, Tensor> ormqr_backward(
    const Tensor&       grad,
    const Tensor&       result,
    const Tensor&       self,
    const Tensor&       tau,
    const Tensor&       other,
    bool                left,
    bool                transpose,
    std::array<bool, 3> grad_output_mask);
std::tuple<Tensor, Tensor> polar_backward(const Tensor& grad, const Tensor& result);
Tensor i1_backward(const Tensor& grad, const Tensor& self, const Tensor& result);
Tensor i1e_backward(const Tensor& grad, const Tensor& self, const Tensor& result);
Tensor linalg_lu_solve_LU(
    const Tensor& grad,
    const Tensor& LU,
    const Tensor& pivots,
    const Tensor& X,
    const bool    left,
    const bool    adjoint);
Tensor linalg_lu_solve_jvp(
    const Tensor& X,
    const Tensor& LU,
    const Tensor& pivots,
    const Tensor& dLU,
    const Tensor& dB,
    const bool    left,
    const bool    adjoint);
std::tuple<Tensor, Tensor> linalg_solve_backward(
    const Tensor& gX,
    const Tensor& X,
    const Tensor& A,
    const Tensor& LU,
    const Tensor& pivots,
    const bool    left,
    const bool    B_requires_grad);
Tensor linalg_solve_jvp(
    const Tensor& dA,
    const Tensor& dB,
    const Tensor& X,
    const Tensor& LU,
    const Tensor& pivots,
    const bool    left,
    const bool    use_A_T);
Tensor lu_unpack_backward(
    const Tensor&           L_grad,
    const Tensor&           U_grad,
    const quarisma::SymInt& m,
    const quarisma::SymInt& n);

Tensor linalg_det_backward(
    const Tensor& grad, const Tensor& det, const Tensor& A, const Tensor& LU, const Tensor& pivots);
Tensor linalg_det_jvp(
    const Tensor& dA,
    const Tensor& det,
    const Tensor& LU,
    const Tensor& pivots,
    const bool    use_A_T);
std::tuple<Tensor, Tensor> linalg_lstsq_backward(
    const Tensor&              gX_,
    const Tensor&              gL,
    const Tensor&              A,
    const Tensor&              B_,
    const Tensor&              X_,
    const std::array<bool, 2>& grad_input_mask);
Tensor linalg_lu_backward(
    const Tensor& L_grad,
    const Tensor& U_grad,
    const Tensor& P,
    const Tensor& L,
    const Tensor& U,
    const bool    pivot);

std::tuple<Tensor, Tensor> linalg_lu_jvp(
    const Tensor& dA, const Tensor& P, const Tensor& L, const Tensor& U, const bool pivot);

Tensor lu_factor_ex_backward(
    const Tensor& grad, const Tensor& LU, const Tensor& pivs, const bool pivot);
Tensor lu_factor_ex_jvp(const Tensor& dX, const Tensor& LU, const Tensor& pivs, const bool pivot);

Tensor batch_norm_jvp(
    const Tensor&                input_p,
    const Tensor&                input_t,
    const Tensor&                weight_p,
    const Tensor&                weight_t,
    const Tensor&                bias_p,
    const Tensor&                bias_t,
    const std::optional<Tensor>& running_mean,
    const std::optional<Tensor>& running_var,
    const Tensor&                saved_mean,
    const Tensor&                saved_invstd,
    bool                         train,
    double                       eps);

Tensor layer_norm_jvp(
    const Tensor&            input_p,
    const Tensor&            input_t,
    const Tensor&            weight_p,
    const Tensor&            weight_t,
    const Tensor&            bias_p,
    const Tensor&            bias_t,
    const Tensor&            saved_mean,
    const Tensor&            saved_invstd,
    quarisma::SymIntArrayRef normalized_shape);

Tensor rms_norm_jvp(
    const Tensor& input_p,
    const Tensor& input_t,
    const Tensor& weight_p,
    const Tensor& weight_t,
    const Tensor& saved_rstd,
    IntArrayRef   normalized_shape);

Tensor rms_norm_rstd_jvp(
    const Tensor& input_p,
    const Tensor& input_t,
    const Tensor& saved_rstd,
    IntArrayRef   normalized_shape);

Tensor group_norm_jvp(
    const Tensor& input_p,
    const Tensor& input_t,
    const Tensor& weight_p,
    const Tensor& weight_t,
    const Tensor& bias_p,
    const Tensor& bias_t,
    const Tensor& saved_mean,
    const Tensor& saved_invstd,
    int64_t       groups);
Tensor group_norm_mean_jvp(const Tensor& input_t, const Tensor& mean_p, int64_t groups);
Tensor group_norm_invstd_jvp(
    const Tensor& input_p,
    const Tensor& input_t,
    const Tensor& mean_p,
    const Tensor& invstd_p,
    int64_t       groups);

Tensor convolution_jvp(
    const Tensor&            input_p,
    const Tensor&            input_t,
    const Tensor&            weight_p,
    const Tensor&            weight_t,
    const Tensor&            bias_p,
    const Tensor&            bias_t,
    quarisma::SymIntArrayRef stride,
    quarisma::SymIntArrayRef padding,
    quarisma::SymIntArrayRef dilation,
    bool                     transposed,
    quarisma::SymIntArrayRef output_padding,
    const quarisma::SymInt&  groups);

Tensor _convolution_jvp(
    const Tensor&            input_p,
    const Tensor&            input_t,
    const Tensor&            weight_p,
    const Tensor&            weight_t,
    const Tensor&            bias_p,
    const Tensor&            bias_t,
    quarisma::SymIntArrayRef stride,
    quarisma::SymIntArrayRef padding,
    quarisma::SymIntArrayRef dilation,
    bool                     transposed,
    quarisma::SymIntArrayRef output_padding,
    const quarisma::SymInt&  groups,
    bool                     benchmark,
    bool                     deterministic,
    bool                     cudnn_enabled,
    bool                     allow_tf32);

Tensor convolution_backward_jvp_grad_bias(const Tensor& grad_out_t, const Tensor& grad_bias);

Tensor cat_jvp(const quarisma::ITensorListRef& tensors, int64_t dim);
Tensor block_diag_jvp(quarisma::TensorList tensors);
Tensor stack_jvp(quarisma::TensorList tensors, int64_t dim);
Tensor cumprod_jvp(const Tensor& self_t, const Tensor& self_p, const Tensor& result, int dim);
Tensor gather_with_keepdimed_indices(
    const Tensor& input, int64_t dim, const Tensor& indices, bool keepdim);
Tensor evenly_read_jvp(const Tensor& fw_grad, const Tensor& input, const Tensor& value);
Tensor warn_backwards(const Tensor& grad_output);

std::tuple<Tensor, Tensor> _cudnn_convolution_backward(
    const quarisma::Tensor&  self,
    const quarisma::Tensor&  grad_output,
    const quarisma::Tensor&  weight,
    quarisma::SymIntArrayRef padding,
    quarisma::SymIntArrayRef output_padding,
    quarisma::SymIntArrayRef stride,
    quarisma::SymIntArrayRef dilation,
    bool                     transposed,
    quarisma::SymInt         groups,
    ::std::array<bool, 2>    output_mask);

Tensor scatter_reduce_jvp(
    const Tensor&    self_p,
    const Tensor&    self_t,
    int              dim,
    const Tensor&    index,
    const Tensor&    src_p,
    const Tensor&    src_t,
    std::string_view reduce,
    bool             include_self,
    const Tensor&    result);

std::tuple<Tensor, Tensor> scatter_reduce_backward(
    const Tensor&    grad,
    const Tensor&    self,
    int              dim,
    const Tensor&    index,
    const Tensor&    src,
    std::string_view reduce,
    bool             include_self,
    const Tensor&    result);

Tensor _to_copy_backward(const Tensor& grad, const quarisma::TensorOptions& self_options);

std::tuple<Tensor, Tensor> index_reduce_backward(
    const Tensor&    grad,
    const Tensor&    self,
    int              dim,
    const Tensor&    index,
    const Tensor&    source,
    std::string_view reduce,
    bool             include_self,
    const Tensor&    result);

Tensor take_backward(const Tensor& grad, const Tensor& self, const Tensor& indices);

Tensor to_sparse_backward(
    const Tensor&                                       grad,
    const quarisma::Layout                              self_layout,
    const quarisma::OptionalArrayRef<quarisma::SymInt>& self_blocksize);

std::tuple<Tensor, Tensor, Tensor, Tensor, Tensor, Tensor, Tensor>
mkldnn_rnn_layer_differentiable_backward(
    const Tensor&                input,
    const Tensor&                weight0,
    const Tensor&                weight1,
    const Tensor&                weight2,
    const Tensor&                weight3,
    const Tensor&                hx_,
    const Tensor&                cx_tmp,
    const Tensor&                output,
    const Tensor&                hy_,
    const Tensor&                cy_,
    const std::optional<Tensor>& grad_output_r_opt,
    const std::optional<Tensor>& grad_hy_r_opt,
    const std::optional<Tensor>& grad_cy_r_opt,
    bool                         reverse,
    int64_t                      mode,
    int64_t                      hidden_size,
    int64_t                      num_layers,
    bool                         has_biases,
    bool                         train,
    bool                         bidirectional,
    quarisma::IntArrayRef        batch_sizes,
    bool                         batch_first,
    const quarisma::Tensor&      workspace);

Tensor values_backward(const Tensor& grad, const Tensor& self);

}  // namespace torch::autograd::generated::details
