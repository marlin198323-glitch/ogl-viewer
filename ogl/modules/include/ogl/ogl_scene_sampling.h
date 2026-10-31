#ifndef OGL_SCENE_SAMPLING_H
#define OGL_SCENE_SAMPLING_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>
#include <array>

namespace ogl {
// Cover all objects for practical OpenSim scenes in the opt-in RTX
// diagnostic. Previously rotating 16K slices of a 28K region omitted
// camera-visible geometry every other scan. At larger object counts,
// retain the same deterministic bounded rotating traversal.
inline uint32_t developer_object_scan_limit(uint32_t total) {
    return std::min(total,32768u);
}
inline uint32_t developer_object_scan_start(uint32_t total,
                                            uint32_t cursor) {
    return total<=32768u || !total ? 0u : cursor%total;
}

// Spend more CPU triangle tests on objects occupying multiple camera rays,
// without letting a single wall/mesh exhaust the whole prepass budget.
// Rank real camera-ray intersections by entry distance (nearest first).
// Coverage is a tie breaker rather than letting a giant distant skybox
// starve nearby surfaces. Invalid/empty bounds are always last.
inline float camera_ray_box_priority(float nearest_ray_m,
                                      uint32_t covered_rays) {
    if (!std::isfinite(nearest_ray_m) || nearest_ray_m<0.f ||
        covered_rays==0 || covered_rays>64u)
        return std::numeric_limits<float>::infinity();
    return std::max(0.f,nearest_ray_m -
                        0.05f*float(std::min(covered_rays,32u)));
}

// A near-only allocation can consume every 8192 Vulkan geometry slot
// before any 30-120m or 120m+ object is visited. Deterministically mix
// *actual ray-box distances*, preserving the order inside each distance
// class, with a near:medium:far service ratio of 2:1:1. A missing class
// never stalls another. This is diagnostic geometry selection, not rendering.
inline uint32_t camera_depth_band(float entry_distance_m) {
    if (!std::isfinite(entry_distance_m) || entry_distance_m<0.f)
        return 3u; // unknown / invalid, handled after known camera boxes
    return entry_distance_m < 30.f ? 0u :
           entry_distance_m < 120.f ? 1u : 2u;
}
inline std::vector<size_t> stratified_camera_order(
    const std::vector<float>& distances_m) {
    std::array<std::vector<size_t>,4> grouped{};
    for (size_t i=0;i<distances_m.size();++i)
        grouped[camera_depth_band(distances_m[i])].push_back(i);
    std::array<size_t,4> cursor{};
    std::vector<size_t> ordered;
    ordered.reserve(distances_m.size());
    constexpr unsigned sequence[]{0u,1u,0u,2u};
    while (ordered.size()<distances_m.size()) {
        const size_t before=ordered.size();
        for (unsigned group:sequence) {
            if (cursor[group]<grouped[group].size())
                ordered.push_back(grouped[group][cursor[group]++]);
        }
        if (ordered.size()==before) break;
    }
    while (cursor[3]<grouped[3].size())
        ordered.push_back(grouped[3][cursor[3]++]);
    return ordered;
}

inline uint32_t camera_ray_object_quota(uint32_t aabb_ray_count,
                                        bool sphere_fallback) {
    if (sphere_fallback) return 128u;
    if (aabb_ray_count==0u) return 0u;
    if (aabb_ray_count<=2u) return 128u;
    if (aabb_ray_count<=7u) return 256u;
    return 512u;
}

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
