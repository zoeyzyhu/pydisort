// torch
#include <ATen/Dispatch.h>
#include <ATen/TensorIterator.h>
#include <ATen/native/ReduceOpsUtils.h>
#include <ATen/native/cpu/Loops.h>
#include <torch/torch.h>

// disort
#include "disort_impl.h"

namespace disort {

void call_disort_cpu(at::TensorIterator& iter, int rank_in_column,
                     disort_state* ds, disort_output* ds_out) {
  AT_DISPATCH_FLOATING_TYPES(iter.dtype(), "disort_cpu", [&] {
    auto nprop = at::native::ensure_nonempty_size(iter.output(), -1);

    iter.for_each([&](char** data, const int64_t* strides, int64_t n) {
      for (int i = 0; i < n; i++) {
        auto out = reinterpret_cast<scalar_t*>(data[0] + i * strides[0]);
        auto prop = reinterpret_cast<scalar_t*>(data[1] + i * strides[1]);
        auto fbeam = reinterpret_cast<scalar_t*>(data[2] + i * strides[2]);
        auto umu0 = reinterpret_cast<scalar_t*>(data[3] + i * strides[3]);
        auto phi0 = reinterpret_cast<scalar_t*>(data[4] + i * strides[4]);
        auto albedo = reinterpret_cast<scalar_t*>(data[5] + i * strides[5]);
        auto fluor = reinterpret_cast<scalar_t*>(data[6] + i * strides[6]);
        auto fisot = reinterpret_cast<scalar_t*>(data[7] + i * strides[7]);
        auto temf = reinterpret_cast<scalar_t*>(data[8] + i * strides[8]);
        auto idx = reinterpret_cast<int64_t*>(data[9] + i * strides[9]);
        disort_impl(out, prop, fbeam, umu0, phi0, albedo, fluor, fisot, temf,
                    rank_in_column, ds[*idx], ds_out[*idx], nprop);
      }
    });
  });
}

}  // namespace disort
