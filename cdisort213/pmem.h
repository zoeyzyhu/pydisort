/* pmem.h -- pool-aware allocation wrappers for cdisort.
 *
 * Host code uses libc allocation. CUDA code uses one bump-pointer slice per
 * solver thread; pool_init() resets the slice for each element, so pfree() is
 * a no-op on the device. The device globals are TU-local because the launcher
 * and bind_workspace() must refer to the same copy. The helpers are static
 * for the same reason: each CUDA translation unit must use its own symbols.
 */
#ifndef __cdisort_pmem_h
#define __cdisort_pmem_h

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef DISPATCH_MACRO
#ifdef __CUDACC__
#define DISPATCH_MACRO __host__ __device__
#else
#define DISPATCH_MACRO
#endif
#endif

#ifdef __CUDACC__

#include <cstdio>

namespace pmem {

/* Workspace binding; TU-local by design (see file comment).  A thread's
 * pool slice is g_pool_base + global_thread_id * g_pool_stride. */
static __device__ char *g_pool_base = nullptr;
static __device__ unsigned long long g_pool_stride = 0;

constexpr size_t ALIGNMENT = 8;

struct PoolState {    /* at the head of each thread's slice */
  uint32_t offset;     /* next free byte, relative to slice start */
  uint32_t pool_bytes; /* total slice size */
};

constexpr size_t STATE_SIZE = sizeof(PoolState);

static __device__ inline size_t align_up(size_t n) {
  return (n + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

static __device__ inline char *slice_base() {
  unsigned long long tid =
      (unsigned long long)blockIdx.x * blockDim.x + threadIdx.x;
  return g_pool_base + tid * g_pool_stride;
}

static __device__ inline PoolState *pool_state(char *base) {
  return reinterpret_cast<PoolState *>(base);
}

static __device__ inline void trap(const char *what, unsigned long long n) {
  printf("\n ******* ERROR >>>>>> pmem: %s (%llu bytes)\n", what, n);
  __trap();
}

/* Reset this thread's slice (bump cursor back to the start).  Must be
 * called (once) at the top of the kernel lambda before any pmalloc.
 * Host trajectory is a no-op so that it can appear in __host__ __device__
 * lambdas. */
DISPATCH_MACRO inline void pool_init() {
#ifdef __CUDA_ARCH__
  if (g_pool_base == nullptr || g_pool_stride == 0)
    trap("workspace not bound", 0);
  char *base = slice_base();
  PoolState *st = pool_state(base);
  st->pool_bytes = (uint32_t)g_pool_stride;
  st->offset = (uint32_t)align_up(STATE_SIZE);
#endif /* __CUDA_ARCH__ */
}

static __device__ inline void *device_malloc(size_t size) {
  if (size == 0) size = ALIGNMENT;
  char *base = slice_base();
  PoolState *st = pool_state(base);
  size_t payload = align_up(size);
  size_t start = st->offset;
  if (start + payload > st->pool_bytes)
    trap("pool exhausted", (unsigned long long)size);
  st->offset = (uint32_t)(start + payload);
  return base + start;
}

/* No-op: see file comment -- cdisort never needs freed space back
 * within a single call, and the next element's pool_init() resets the
 * whole slice anyway. */
static __device__ inline void device_free(void *) {}

/* Host-side: bind the workspace buffer for subsequent kernel launches.
 * Binds this translation unit's copy of the symbols -- call it from the
 * same TU that launches the kernels. */
static inline cudaError_t bind_workspace(void *base, size_t stride) {
  char *b = static_cast<char *>(base);
  unsigned long long s = stride;
  cudaError_t err = cudaMemcpyToSymbol(g_pool_base, &b, sizeof(b));
  if (err != cudaSuccess) return err;
  return cudaMemcpyToSymbol(g_pool_stride, &s, sizeof(s));
}

}  // namespace pmem

#endif  /* __CUDACC__ */

DISPATCH_MACRO inline void *pmalloc(size_t size) {
#ifdef __CUDA_ARCH__
  return pmem::device_malloc(size);
#else
  return malloc(size);
#endif
}

DISPATCH_MACRO inline void *pcalloc(size_t nmemb, size_t size) {
#ifdef __CUDA_ARCH__
  void *p = pmem::device_malloc(nmemb * size);
  memset(p, 0, nmemb * size);
  return p;
#else
  return calloc(nmemb, size);
#endif
}

DISPATCH_MACRO inline void pfree(void *ptr) {
#ifdef __CUDA_ARCH__
  pmem::device_free(ptr);
#else
  free(ptr);
#endif
}

#endif /* !__cdisort_pmem_h */
