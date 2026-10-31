#ifndef OGL_CPU_FEATURES_H
#define OGL_CPU_FEATURES_H

// CPU feature decisions for Linux, Windows and macOS; C++17, no GUI required.
// A different CPU manufacturer is NOT a different instruction set.
// Runtime dispatch is mandatory before using an AVX2 kernel.
#include <cstdint>
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
# include <intrin.h>
#elif (defined(__x86_64__) || defined(__i386__)) && (defined(__GNUC__) || defined(__clang__))
# include <cpuid.h>
#endif

namespace ogl {
enum class CpuArch { x86_64, x86, arm64, other };
enum class CpuPath { scalar, sse2, avx2, neon };
struct CpuFeatures {
    CpuArch arch=CpuArch::other;
    bool sse2=false;
    bool avx2=false;
    bool neon=false;
};

inline CpuArch target_architecture() {
#if defined(__aarch64__) || defined(_M_ARM64)
    return CpuArch::arm64;
#elif defined(__x86_64__) || defined(_M_X64)
    return CpuArch::x86_64;
#elif defined(__i386__) || defined(_M_IX86)
    return CpuArch::x86;
#else
    return CpuArch::other;
#endif
}
inline const char* arch_name(CpuArch arch) {
    switch (arch) {
        case CpuArch::x86_64: return "x86_64";
        case CpuArch::x86: return "x86";
        case CpuArch::arm64: return "arm64";
        default: return "other";
    }
}
inline const char* path_name(CpuPath path) {
    switch (path) {
        case CpuPath::sse2: return "sse2";
        case CpuPath::avx2: return "avx2";
        case CpuPath::neon: return "neon";
        default: return "scalar";
    }
}
// Shared portable feature policy. AVX2 is only valid if the OS SAVES
// YMM registers, not merely if the processor advertises AVX2.
inline CpuPath select_cpu_path(const CpuFeatures& f) {
    if (f.arch==CpuArch::arm64) return f.neon ? CpuPath::neon : CpuPath::scalar;
    if (f.arch==CpuArch::x86_64 || f.arch==CpuArch::x86) {
        if (f.avx2) return CpuPath::avx2;
        if (f.sse2) return CpuPath::sse2;
    }
    return CpuPath::scalar;
}
inline CpuFeatures detect_cpu_features() {
    CpuFeatures features{};
    features.arch=target_architecture();
#if defined(__aarch64__) || defined(_M_ARM64)
    // NEON/Advanced SIMD is mandatory in the AArch64 base ISA.
    features.neon=true;
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    int regs[4]{};
    __cpuid(regs,0);
    const int maxleaf=regs[0];
    if (maxleaf>=1) {
        __cpuidex(regs,1,0);
        features.sse2=(regs[3] & (1<<26))!=0;
        const bool osxsave=(regs[2] & (1<<27))!=0;
        const bool avx=(regs[2] & (1<<28))!=0;
        if(osxsave && avx && ((_xgetbv(0)&6)==6) && maxleaf>=7) {
            __cpuidex(regs,7,0);
            features.avx2=(regs[1] & (1<<5))!=0;
        }
    }
#elif (defined(__x86_64__) || defined(__i386__)) && (defined(__GNUC__) || defined(__clang__))
    unsigned eax=0,ebx=0,ecx=0,edx=0;
    const unsigned maxleaf=__get_cpuid_max(0,nullptr);
    if(maxleaf>=1 && __get_cpuid(1,&eax,&ebx,&ecx,&edx)) {
        features.sse2=(edx & (1u<<26))!=0;
        const bool osxsave=(ecx & (1u<<27))!=0;
        const bool avx=(ecx & (1u<<28))!=0;
        if (osxsave && avx && maxleaf>=7) {
            unsigned xlo=0,xhi=0;
            // XGETBV may only execute when OSXSAVE reports support.
            __asm__ volatile("xgetbv" : "=a"(xlo),"=d"(xhi) : "c"(0));
            if ((xlo&6u)==6u &&
                __get_cpuid_count(7,0,&eax,&ebx,&ecx,&edx))
                features.avx2=(ebx&(1u<<5))!=0;
        }
    }
#endif
    return features;
}
} // namespace ogl
#endif
