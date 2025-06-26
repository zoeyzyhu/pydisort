// torch
#include <ATen/Dispatch.h>
#include <ATen/TensorIterator.h>
#include <ATen/native/ReduceOpsUtils.h>
#include <ATen/native/DispatchStub.h>
#include <c10/cuda/CUDAGuard.h>

// disort
#include <disort/loops.cuh>
#include "disort_dispatch.hpp"
#include "disort_impl.h"

namespace disort {

void call_disort_cuda(at::TensorIterator& iter, int rank_in_column,
                      disort_state *ds, disort_output *ds_out) {
  at::cuda::CUDAGuard device_guard(iter.device());

  AT_DISPATCH_FLOATING_TYPES(iter.dtype(), "call_disort_cuda", [&] {
    auto nprop = at::native::ensure_nonempty_size(iter.output(), -1);

    native::gpu_kernel<12>(
        iter, [=] GPU_LAMBDA(char* const data[12], unsigned int strides[12]) {
          auto out = reinterpret_cast<scalar_t*>(data[0] + strides[0]);
          auto prop = reinterpret_cast<scalar_t*>(data[1] + strides[1]);
          auto umu0 = reinterpret_cast<scalar_t*>(data[2] + strides[2]);
          auto phi0 = reinterpret_cast<scalar_t*>(data[3] + strides[3]);
          auto fbeam = reinterpret_cast<scalar_t*>(data[4] + strides[4]);
          auto albedo = reinterpret_cast<scalar_t*>(data[5] + strides[5]);
          auto fluor = reinterpret_cast<scalar_t*>(data[6] + strides[6]);
          auto fisot = reinterpret_cast<scalar_t*>(data[7] + strides[7]);
          auto temis = reinterpret_cast<scalar_t*>(data[8] + strides[8]);
          auto btemp = reinterpret_cast<scalar_t*>(data[9] + strides[9]);
          auto ttemp = reinterpret_cast<scalar_t*>(data[10] + strides[10]);
          auto temf = reinterpret_cast<scalar_t*>(data[11] + strides[11]);
          auto idxf = reinterpret_cast<scalar_t*>(data[12] + strides[12]);
          int idx = static_cast<int>(*idxf);
          //  disort_impl(out, prop, ftoa, temf, rank_in_column, ds[*idx],
          //            ds_out[*idx], nprop);
        });
  });
}

}  // namespace disort

namespace at::native {

REGISTER_CUDA_DISPATCH(call_disort, &disort::call_disort_cuda);

} // namespace at::native
