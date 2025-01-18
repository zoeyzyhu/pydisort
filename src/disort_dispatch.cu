// torch
#include <ATen/Dispatch.h>
#include <ATen/TensorIterator.h>
#include <ATen/native/ReduceOpsUtils.h>
#include <c10/cuda/CUDAGuard.h>

// disort
#include "loops.cuh"
#include "disort_impl.h"

namespace disort {

void call_disort_cuda(at::TensorIterator& iter, int rank_in_column,
                      disort_state *ds, disort_output *ds_out) {
  at::cuda::CUDAGuard device_guard(iter.device());

  AT_DISPATCH_FLOATING_TYPES(iter.dtype(), "disort_cuda", [&] {
    auto nprop = at::native::ensure_nonempty_size(iter.output(), -1);

    native::gpu_kernel<scalar_t, 6>(
        iter, [=] GPU_LAMBDA(char* const data[6], unsigned int strides[6]) {
          auto out = reinterpret_cast<scalar_t*>(data[0] + strides[0]);
          auto prop = reinterpret_cast<scalar_t*>(data[1] + strides[1]);
          auto ftoa = reinterpret_cast<scalar_t*>(data[2] + strides[2]);
          auto temf = reinterpret_cast<scalar_t*>(data[3] + strides[3]);
          auto idx = reinterpret_cast<int64_t*>(data[4] + strides[4]);
          //  disort_impl(out, prop, ftoa, temf, rank_in_column, ds[*idx],
          //            ds_out[*idx], nprop);
        });
  });
}

}  // namespace disort
