#include "ogl/ogl_cpu_buffer.h"
#include "ogl/ogl_cpu_features.h"
#include <cstdint>
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
# include <emmintrin.h>
# define OGL_CPU_HAVE_SSE2_API 1
#endif

namespace ogl {
namespace {
inline unsigned popcount32(std::uint32_t v) {
    // Portable SWAR; do not accidentally require POPCNT on baseline CPUs.
    v -= (v>>1) & 0x55555555u;
    v = (v&0x33333333u)+((v>>2)&0x33333333u);
    return ((v+(v>>4))&0x0f0f0f0fu)*0x01010101u >> 24;
}
#if defined(OGL_CPU_HAVE_SSE2_API)
std::size_t count_sse2(const unsigned char* a,const unsigned char* b,std::size_t n) {
    std::size_t i=0,changed=0;
    for (;i+16<=n;i+=16) {
        const __m128i va=_mm_loadu_si128(reinterpret_cast<const __m128i*>(a+i));
        const __m128i vb=_mm_loadu_si128(reinterpret_cast<const __m128i*>(b+i));
        const std::uint32_t equal=static_cast<std::uint32_t>(
            _mm_movemask_epi8(_mm_cmpeq_epi8(va,vb)));
        changed+=16-popcount32(equal);
    }
    for (;i<n;++i) changed+=(a[i]!=b[i]);
    return changed;
}
#endif
} // namespace

std::size_t count_changed_bytes_scalar(const unsigned char* a,
                                       const unsigned char* b,std::size_t n) {
    if (!a || !b) return 0;
    std::size_t changes=0;
    for(std::size_t i=0;i<n;++i) changes+=(a[i]!=b[i]);
    return changes;
}
#ifdef OGL_WITH_AVX2
std::size_t count_changed_bytes_avx2(const unsigned char*,const unsigned char*,std::size_t);
#endif
#ifdef OGL_WITH_NEON
std::size_t count_changed_bytes_neon(const unsigned char*,const unsigned char*,std::size_t);
#endif

const char* selected_buffer_backend() {
    const CpuFeatures detected=detect_cpu_features();
    switch (select_cpu_path(detected)) {
#ifdef OGL_WITH_AVX2
        case CpuPath::avx2:return "avx2";
#endif
#ifdef OGL_WITH_NEON
        case CpuPath::neon:return "neon";
#endif
        case CpuPath::sse2: return "sse2";
#ifndef OGL_WITH_AVX2
        case CpuPath::avx2:
            return "sse2"; // AVX2 CPU but no compiler-provided AVX2 kernel.
#endif
        default:return "scalar";
    }
}
std::size_t count_changed_bytes(const unsigned char* a,const unsigned char* b,
                                std::size_t n) {
    if (!a || !b || n==0) return 0;
    const CpuFeatures detected=detect_cpu_features();
    switch (select_cpu_path(detected)) {
#ifdef OGL_WITH_AVX2
        case CpuPath::avx2:
            return count_changed_bytes_avx2(a,b,n);
#endif
#ifdef OGL_WITH_NEON
        case CpuPath::neon:
            return count_changed_bytes_neon(a,b,n);
#endif
        default:break;
    }
#if defined(OGL_CPU_HAVE_SSE2_API)
    if (detected.sse2) return count_sse2(a,b,n);
#endif
    return count_changed_bytes_scalar(a,b,n);
}
} // namespace ogl
