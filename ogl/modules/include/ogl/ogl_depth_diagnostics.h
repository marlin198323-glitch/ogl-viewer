#ifndef OGL_DEPTH_DIAGNOSTICS_H
#define OGL_DEPTH_DIAGNOSTICS_H

#include "ogl_depth_validation.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

// Pure geometry-independent diagnostics. Normalized GL depth is the
// non-linear z-buffer, NOT a world-space distance. The projection conversion
// must match depth_guided_sun.comp, using the original Firestorm near/far.
// Diagnostic data are anonymous counts/distances; no pixel or world metadata.
namespace ogl {
struct DepthStats {
    uint32_t surface_pixels = 0;
    uint32_t background_pixels = 0;
    float depth_min = 1.f;
    float depth_max = 0.f;
    float view_distance_min = 0.f;
    float view_distance_max = 0.f;
};

inline bool gl_depth_to_view_distance(float depth, float near_clip,
                                     float far_clip, float& view_distance) {
    if (!std::isfinite(depth) || !std::isfinite(near_clip) ||
        !std::isfinite(far_clip) || depth < 0.f || depth >= 0.999999f ||
        near_clip < 0.005f || far_clip <= near_clip ||
        far_clip > 100000.f) return false;
    const double n = near_clip, f = far_clip;
    const double z_ndc = 2.0 * depth - 1.0;
    const double denom = f+n-z_ndc*(f-n);
    if (denom <= 1e-8) return false;
    const double distance = 2.0*n*f/denom;
    if (!std::isfinite(distance) || distance < 0.0 ||
        distance > double(far_clip)*1.001) return false;
    view_distance = static_cast<float>(distance);
    return true;
}

inline bool summarize_depth_grid(const float* grid, uint32_t width,
                                 uint32_t height, float near_clip,
                                 float far_clip, DepthStats& output) {
    if (!valid_depth_grid(width,height,8,8,grid) ||
        !std::isfinite(near_clip) || !std::isfinite(far_clip) ||
        near_clip < 0.005f || far_clip <= near_clip ||
        far_clip > 100000.f) return false;
    DepthStats fresh{};
    fresh.view_distance_min = std::numeric_limits<float>::max();
    for (uint32_t i=0; i<64; ++i) {
        float distance = 0.f;
        if (!gl_depth_to_view_distance(grid[i],near_clip,far_clip,distance)) {
            ++fresh.background_pixels;
            continue;
        }
        ++fresh.surface_pixels;
        fresh.depth_min = std::min(fresh.depth_min,grid[i]);
        fresh.depth_max = std::max(fresh.depth_max,grid[i]);
        fresh.view_distance_min = std::min(fresh.view_distance_min,distance);
        fresh.view_distance_max = std::max(fresh.view_distance_max,distance);
    }
    if (!fresh.surface_pixels)
        fresh.view_distance_min = 0.f;
    output = fresh;
    return fresh.surface_pixels + fresh.background_pixels == 64;
}
} // namespace ogl
#endif
