#ifndef OGL_SCENE_SAMPLING_H
#define OGL_SCENE_SAMPLING_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace ogl {
// Spread a strict budget over the *whole* triangle index list. Sampling
// only the first 32 triangles causes severe missing-foreground artifacts on
// OpenSim mesh objects. 64-bit multiplication avoids large-index overflow.
inline uint32_t spread_triangle_index(uint32_t triangle_count,
                                      uint32_t sample_count,
                                      uint32_t sample_index) {
    if (!triangle_count || !sample_count || sample_count > triangle_count ||
        sample_index >= sample_count) return UINT32_MAX;
    if (triangle_count == sample_count) return sample_index;
    const uint64_t numerator =
        (uint64_t(2) * sample_index + 1) * triangle_count;
    return static_cast<uint32_t>(
        std::min<uint64_t>(triangle_count-1,numerator/(2*sample_count)));
}

// Allow a farther cone in front of the camera to reach background geometry
// visible in OpenSim depth. Still bound near offscreen/sun occluders; the
// budget-limited RT probe is NOT an exhaustive scene collector.
inline bool scene_candidate_in_range(float distance_m, float forward_cos,
                                     float object_extent_m) {
    if (!std::isfinite(distance_m) || !std::isfinite(forward_cos) ||
        !std::isfinite(object_extent_m) || distance_m < 0.f ||
        object_extent_m < 0.f || forward_cos < -1.01f ||
        forward_cos > 1.01f) return false;
    const float radius = 0.5f*std::min(object_extent_m, 40.f);
    if (distance_m <= 90.f+radius) return true;
    // Wide cone also covers ultrawide viewport edges; camera-space
    // projection and per-triangle visibility are not asserted here.
    // OpenGL's actual 8x8 depth reports visible surfaces beyond 600 m in
    // some OpenSim scenes. Far candidates remain lower priority than nearby
    // foreground objects and are still constrained by the triangle budget.
    return forward_cos >= 0.15f && distance_m <= 768.f+radius;
}

// Lower score = collect first. Favour closer objects in front of camera,
// while retaining some behind-camera occluders for incoming sunlight.
// Bounding extent is approximate; never used for a GPU hit or world shading.
inline float scene_priority(float distance_m, float forward_cos,
                            float object_extent_m) {
    if (!std::isfinite(distance_m) || !std::isfinite(forward_cos) ||
        !std::isfinite(object_extent_m) || distance_m < 0.f ||
        forward_cos < -1.01f || forward_cos > 1.01f ||
        object_extent_m < 0.f) return std::numeric_limits<float>::infinity();
    return distance_m + 30.f*std::max(0.f,0.35f-forward_cos)
           - 0.5f*std::min(object_extent_m,12.f);
}
} // namespace ogl
#endif
