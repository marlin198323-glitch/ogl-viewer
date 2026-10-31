#ifndef OGL_CPU_BUFFER_H
#define OGL_CPU_BUFFER_H
#include <cstddef>
namespace ogl {
// Compare memory buffers for cache/texture invalidation. Does NOT alter data.
// Runtime picks AVX2 on supporting x86, baseline SSE2, ARM64 NEON, or scalar.
std::size_t count_changed_bytes(const unsigned char* old_data,
                                const unsigned char* new_data,
                                std::size_t size);
std::size_t count_changed_bytes_scalar(const unsigned char* old_data,
                                       const unsigned char* new_data,
                                       std::size_t size);
const char* selected_buffer_backend();
}
#endif
