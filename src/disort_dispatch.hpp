#pragma once

// torch
#include <ATen/TensorIterator.h>
#include <ATen/native/DispatchStub.h>

namespace at::native {

using disort_fn = void (*)(at::TensorIterator &iter, int upward,
                           disort_state *ds, disort_output *ds_out);

DECLARE_DISPATCH(disort_fn, call_disort);

}  // namespace at::native
