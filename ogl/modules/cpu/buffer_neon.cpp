#include <cstddef>
#include <arm_neon.h>
namespace ogl {
std::size_t count_changed_bytes_neon(const unsigned char* a,
                                     const unsigned char* b,std::size_t n) {
    std::size_t i=0,changed=0;
    for(;i+16<=n;i+=16) {
        const uint8x16_t eq=vceqq_u8(vld1q_u8(a+i),vld1q_u8(b+i));
        changed+=16-vaddvq_u8(vshrq_n_u8(eq,7));
    }
    for(;i<n;++i) changed+=(a[i]!=b[i]);
    return changed;
}
}
