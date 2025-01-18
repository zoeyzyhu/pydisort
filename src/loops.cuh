#pragma once

// torch
#include <ATen/TensorIterator.h>
#include <ATen/native/cuda/Loops.cuh>

namespace disort {
namespace native {

template <typename scalar_t, int Arity, typename func_t>
void gpu_kernel(at::TensorIterator& iter, const func_t& f) {
  TORCH_CHECK(iter.ninputs() + iter.noutputs() == Arity);

  at::detail::Array<char*, Arity> data;
  for (int i = 0; i < Arity; i++) {
    data[i] = reinterpret_cast<char*>(iter.data_ptr(i));
  }

  auto offset_calc = ::make_offset_calculator<Arity>(iter);
  int64_t numel = iter.numel();
  constexpr int unroll_factor = sizeof(scalar_t) >= 4 ? 2 : 4;

  at::native::launch_legacy_kernel<128, unroll_factor>(numel,
      [=] __device__(int idx) {
      auto offsets = offset_calc.get(idx);
      f(data.data, offsets.data);
    });
}

}  // namespace native
}  // namespace disort
