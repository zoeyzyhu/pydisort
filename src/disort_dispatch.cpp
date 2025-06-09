// torch
#include <ATen/Dispatch.h>
#include <ATen/native/ReduceOpsUtils.h>
#include <ATen/native/cpu/Loops.h>
#include <torch/torch.h>

// disort
#include "disort_dispatch.hpp"
#include "disort_impl.h"

namespace disort {

void call_disort_cpu(at::TensorIterator &iter, int upward, disort_state *ds,
                     disort_output *ds_out) {
  AT_DISPATCH_FLOATING_TYPES(iter.dtype(), "call_disort_cpu", [&] {
    auto nprop = at::native::ensure_nonempty_size(iter.input(0), -1);
    int grain_size = iter.numel() / at::get_num_threads();

    iter.for_each(
        [&](char **data, const int64_t *strides, int64_t n) {
          for (int i = 0; i < n; i++) {
            auto out = reinterpret_cast<scalar_t *>(data[0] + i * strides[0]);
            auto prop = reinterpret_cast<scalar_t *>(data[1] + i * strides[1]);
            auto umu0 = reinterpret_cast<scalar_t *>(data[2] + i * strides[2]);
            auto phi0 = reinterpret_cast<scalar_t *>(data[3] + i * strides[3]);
            auto fbeam = reinterpret_cast<scalar_t *>(data[4] + i * strides[4]);
            auto albedo =
                reinterpret_cast<scalar_t *>(data[5] + i * strides[5]);
            auto fluor = reinterpret_cast<scalar_t *>(data[6] + i * strides[6]);
            auto fisot = reinterpret_cast<scalar_t *>(data[7] + i * strides[7]);
            auto temis = reinterpret_cast<scalar_t *>(data[8] + i * strides[8]);
            auto btemp = reinterpret_cast<scalar_t *>(data[9] + i * strides[9]);
            auto ttemp =
                reinterpret_cast<scalar_t *>(data[10] + i * strides[10]);
            auto temf =
                reinterpret_cast<scalar_t *>(data[11] + i * strides[11]);
            auto idxf =
                reinterpret_cast<scalar_t *>(data[12] + i * strides[12]);
            int idx = static_cast<int>(*idxf);
            disort_impl(out, prop, umu0, phi0, fbeam, albedo, fluor, fisot,
                        temis, btemp, ttemp, temf, upward, ds[idx], ds_out[idx],
                        nprop);
          }
        },
        grain_size);
  });
}

}  // namespace disort

namespace at::native {

DEFINE_DISPATCH(call_disort);
REGISTER_ALL_CPU_DISPATCH(call_disort, &disort::call_disort_cpu);

}  // namespace at::native
