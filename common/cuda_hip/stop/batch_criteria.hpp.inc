/*******************************<GINKGO LICENSE>******************************
Copyright (c) 2017-2023, the Ginkgo authors
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************<GINKGO LICENSE>*******************************/


/**
 * @see reference/stop/batch_criteria.hpp
 */
template <typename ValueType>
class SimpleRelResidual {
public:
    using real_type = remove_complex<ValueType>;

    __device__ __forceinline__ SimpleRelResidual(
        const real_type rel_res_tol, const real_type* const rhs_b_norms)
        : rel_tol_{rel_res_tol}, rhs_norms_{rhs_b_norms}
    {}

    __device__ __forceinline__ bool check_converged(
        const real_type* const residual_norms) const
    {
        return residual_norms[0] <= (rel_tol_ * rhs_norms_[0]);
    }

private:
    const real_type rel_tol_;
    const real_type* const rhs_norms_;
};


/**
 * @see reference/stop/batch_criteria.hpp
 */
template <typename ValueType>
class SimpleAbsResidual {
public:
    using real_type = remove_complex<ValueType>;

    __device__ __forceinline__ SimpleAbsResidual(const real_type tol,
                                                 const real_type*)
        : abs_tol_{tol}
    {}

    __device__ __forceinline__ bool check_converged(
        const real_type* const residual_norms) const
    {
        return (residual_norms[0] <= abs_tol_);
    }

private:
    const real_type abs_tol_;
};