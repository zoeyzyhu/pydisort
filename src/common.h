#pragma once

#ifdef __CUDACC__
#define DISPATCH_MACRO __host__ __device__
#else
#define DISPATCH_MACRO
#endif

namespace disort {

namespace index {
constexpr int IAB = 0;
constexpr int ISS = 1;
constexpr int IPM = 2;

constexpr int IUP = 0;
constexpr int IDN = 1;
}  // namespace index

}  // namespace disort
