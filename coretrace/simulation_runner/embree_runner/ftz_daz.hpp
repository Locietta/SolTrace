#ifndef SOLTRACE_FTZ_DAZ_HPP
#define SOLTRACE_FTZ_DAZ_HPP

#include <cstdint>

// Set Flush-to-Zero (FTZ) and Denormals-are-Zero (DAZ) floating-point flags
// for the calling thread. These are thread-local CPU register settings that
// avoid slow denormal handling in the FPU, as recommended by the Embree docs.
// On ARM, DAZ is implicit when FZ is set (no separate bit).

#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
#  include <xmmintrin.h>
// Set FTZ (bit 15, 0x8000) and DAZ (bit 6, 0x0040) via MXCSR.
// <xmmintrin.h> is sufficient; <pmmintrin.h> (SSE3) is intentionally avoided
// so this compiles on SSE-only targets (-msse without -msse3).
#  define SOLTRACE_SET_FTZ_DAZ() \
     _mm_setcsr(_mm_getcsr() | 0x8040u)
#elif defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
#  if defined(_MSC_VER)
#    include <intrin.h>
#    define SOLTRACE_SET_FTZ_DAZ() do { \
       uint64_t _fpcr = _ReadStatusReg(ARM64_FPCR); \
       _fpcr |= (1ULL << 24); \
       _WriteStatusReg(ARM64_FPCR, _fpcr); \
     } while(0)
#  else
     /* GCC / Clang on ARM64 */
#    define SOLTRACE_SET_FTZ_DAZ() do { \
       uint64_t _fpcr; \
       __asm__ __volatile__("mrs %0, fpcr" : "=r"(_fpcr)); \
       _fpcr |= (1ULL << 24); \
       __asm__ __volatile__("msr fpcr, %0" : : "r"(_fpcr)); \
     } while(0)
#  endif
#else
#  define SOLTRACE_SET_FTZ_DAZ() /* unsupported architecture, no-op */
#endif

#endif // SOLTRACE_FTZ_DAZ_HPP
