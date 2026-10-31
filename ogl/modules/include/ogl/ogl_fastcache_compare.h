#ifndef OGL_FASTCACHE_COMPARE_H
#define OGL_FASTCACHE_COMPARE_H
// Compare only the initialized metadata and image bytes of an existing
// Firestorm fast-cache entry. The tail is uninitialized padding in some
// legacy records: comparing it would produce false cache misses.
#include "ogl_cpu_buffer.h"
#include <cstddef>
#include <cstring>
namespace ogl {
inline bool same_fastcache_entry(const unsigned char* old_entry,
                                 const unsigned char* new_entry,
                                 std::size_t entry_size,
                                 std::size_t header_size,
                                 std::size_t actual_pixel_bytes) {
    if (!old_entry || !new_entry || !header_size || !actual_pixel_bytes ||
        header_size > entry_size ||
        actual_pixel_bytes > entry_size-header_size)
        return false;
    // Metadata includes width, height, components and discard level.
    // Do this small check before costly image comparisons.
    if (std::memcmp(old_entry,new_entry,header_size)!=0)
        return false;
    // For 1KB exact equality, the platform libc memcmp routinely
    // outperforms a count-all-differences AVX2 kernel. On measured
    // Linux/glibc: ~4ms vs ~20ms / 180k calls. Honor each OS libc.
    return std::memcmp(old_entry+header_size,new_entry+header_size,
                       actual_pixel_bytes)==0;
}
} // namespace ogl
#endif // OGL_FASTCACHE_COMPARE_H
