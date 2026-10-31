#ifndef OGL_SCENE_SNAPSHOT_POLICY_H
#define OGL_SCENE_SNAPSHOT_POLICY_H

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ogl {
// Developer-mode safety net for discontinuities in a rotating OpenSim object
// snapshot. A normal viewer render or a region change must not reuse stale
// objects. This delays *replacement* only, for a small number of scans.
inline bool should_publish_scene_snapshot(uint32_t previous_triangles,
    uint32_t previous_objects, uint32_t current_triangles,
    uint32_t current_objects, float eye_translation_m, float forward_cosine,
    uint32_t prior_deferrals, uint32_t max_deferrals=8u) {
    if (!previous_triangles || !previous_objects || !current_triangles)
        return true;
    if (!std::isfinite(eye_translation_m) ||
        !std::isfinite(forward_cosine))
        return true;
    if (eye_translation_m >= 15.f || forward_cosine < 0.94f)
        return true; // different view; do not anchor to old camera
    if (prior_deferrals >= max_deferrals)
        return true; // bounded hold time, even if real geometry disappeared
    if (previous_triangles < 2048u)
        return true;
    // Trigger only for a catastrophic loss in BOTH triangle coverage and
    // the number of objects. An intentional lower LOD is not rejected.
    const bool triangle_collapse =
        uint64_t(current_triangles)*100u <
        uint64_t(previous_triangles)*35u;
    const bool object_collapse =
        uint64_t(current_objects)*3u <
        uint64_t(std::max(previous_objects, 9u));
    return !(triangle_collapse && object_collapse);
}
} // namespace ogl
#endif
