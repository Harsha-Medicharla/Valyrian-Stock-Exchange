/*
 this is for busy waiting condition when the threads have to wait preventing context switch
*/

#pragma once

// Portable pause/yield primitive for busy-wait loops.
// On x86/x64 we use _mm_pause() for better power/latency under contention.
// On other architectures we fall back to a lightweight yield.

#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

namespace ems
{

        inline void pause() noexcept
        {
#if defined(__i386__) || defined(__x86_64__)
                // _mm_pause is x86/x64 specific.
                // (Include guarded at compile-time below.)
                _mm_pause();
#else
// ARM/yield hint when available; otherwise a compiler barrier.
#if defined(__aarch64__) || defined(__arm__)
                __asm__ __volatile__("yield" ::: "memory");
#else
                __asm__ __volatile__("" ::: "memory");
#endif
#endif
        }

}
