#pragma once

#ifdef __CUDACC__
#define DISPATCH_MACRO __host__ __device__
#else
#define DISPATCH_MACRO
#endif
