#include <cstddef>
#include <cstdint>
#include <immintrin.h>
namespace ogl {
namespace {
inline unsigned popcount32(std::uint32_t v) {
    v-=(v>>1)&0x55555555u;
    v=(v&0x33333333u)+((v>>2)&0x33333333u);
    return ((v+(v>>4))&0x0f0f0f0fu)*0x01010101u>>24;
}
}
std::size_t count_changed_bytes_avx2(const unsigned char* a,
                                     const unsigned char* b,std::size_t n) {
    std::size_t i=0,changed=0;
    for(;i+32<=n;i+=32) {
        const __m256i va=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(a+i));
        const __m256i vb=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(b+i));
        const auto mask=static_cast<std::uint32_t>(
            _mm256_movemask_epi8(_mm256_cmpeq_epi8(va,vb)));
        changed+=32-popcount32(mask);
    }
    for(;i<n;++i) changed+=(a[i]!=b[i]);
    return changed;
}
}
