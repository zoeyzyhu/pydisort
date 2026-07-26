#pragma once

// torch
#include <ATen/TensorIterator.h>
#include <ATen/native/DispatchStub.h>

// disort
#include <cdisort213/cdisort.hpp>

namespace at::native {

using disort_fn = void (*)(at::TensorIterator &iter, int upward,
                           bool force_general,
                           disort_state *ds, disort_output *ds_out,
                           at::Tensor *cuda_workspace);

DECLARE_DISPATCH(disort_fn, call_disort);

}  // namespace at::native
