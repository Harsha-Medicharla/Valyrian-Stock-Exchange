#pragma once

#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

namespace ems
{

        inline void pause() noexcept
        {
#if defined(__i386__) || defined(__x86_64__)
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
