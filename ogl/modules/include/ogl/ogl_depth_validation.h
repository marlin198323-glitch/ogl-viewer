#ifndef OGL_DEPTH_VALIDATION_H
#define OGL_DEPTH_VALIDATION_H
#include <cmath>
#include <cstdint>

// OpenGL normalized depth values from a 8x8 contiguous central viewport
// tile. NOT a full-frame 8x8 reduction, and not a shadow map.
// Reject invalid values before exposing a sample to the Vulkan bridge.
namespace ogl {
constexpr uint32_t kDepthTileWidth = 8;
constexpr uint32_t kDepthTileHeight = 8;
constexpr uint32_t kDepthTilePixels = 64;
inline bool valid_depth_tile(uint32_t width, uint32_t height,
                             uint32_t x, uint32_t y, const float* tile) {
    if (!tile || width < kDepthTileWidth || height < kDepthTileHeight ||
        width > 32768 || height > 32768 ||
        x > width - kDepthTileWidth || y > height - kDepthTileHeight)
        return false;
    for (unsigned i=0; i<kDepthTilePixels; ++i) {
        if (!std::isfinite(tile[i]) || tile[i] < 0.f || tile[i] > 1.f)
            return false;
    }
    return true;
}
}
#endif
