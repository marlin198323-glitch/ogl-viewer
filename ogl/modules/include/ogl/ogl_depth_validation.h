#ifndef OGL_DEPTH_VALIDATION_H
#define OGL_DEPTH_VALIDATION_H
#include <cmath>
#include <cstdint>

// OpenGL normalized depths from the entire viewport downsampled to an
// 8x8 grid, aligned to camera-ray centres (x+0.5)/8, (y+0.5)/8.
// Not a shadow map. Reject invalid values before exposing any sample.
namespace ogl {
constexpr uint32_t kDepthGridWidth = 8;
constexpr uint32_t kDepthGridHeight = 8;
constexpr uint32_t kDepthGridPixels = 64;
inline bool valid_depth_grid(uint32_t width, uint32_t height,
                             uint32_t grid_width, uint32_t grid_height,
                             const float* tile) {
    if (!tile || width < kDepthGridWidth || height < kDepthGridHeight ||
        width > 32768 || height > 32768 ||
        grid_width != kDepthGridWidth || grid_height != kDepthGridHeight)
        return false;
    for (unsigned i=0; i<kDepthGridPixels; ++i) {
        if (!std::isfinite(tile[i]) || tile[i] < 0.f || tile[i] > 1.f)
            return false;
    }
    return true;
}
}
#endif
