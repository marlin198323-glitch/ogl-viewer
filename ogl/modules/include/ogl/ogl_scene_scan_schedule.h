#ifndef OGL_SCENE_SCAN_SCHEDULE_H
#define OGL_SCENE_SCAN_SCHEDULE_H

#include <algorithm>
#include <cstdint>

// Pure, vendor-independent scheduling policy. Render-frame counting alone
// can take minutes during initial OpenSim texture/mesh loading (observed
// 0.1 FPS): the first diagnostic geometry snapshot must also be time based.
// Only the opt-in developer scene path gets the early scan; the normal
// OpenGL path retains its original >=180-frame throttle.
namespace ogl {
inline bool scene_scan_due(unsigned frames_since_scan, bool developer,
                           bool region_ready, uint32_t loaded_objects,
                           uint32_t last_scanned_objects,
                           bool scene_published, int64_t region_age_ms,
                           int64_t last_attempt_age_ms) {
    if (frames_since_scan>=180u) return true;
    if (!developer || !region_ready || loaded_objects==0 ||
        region_age_ms<5000 || last_attempt_age_ms<10000)
        return false;
    if (!scene_published) return true;
    const uint32_t growth_threshold=
        std::max(128u,last_scanned_objects/4u);
    // Within the loading phase, a substantially populated region may
    // replace a tiny initial scan without another 180 slow frames.
    return loaded_objects>last_scanned_objects &&
           loaded_objects-last_scanned_objects>=growth_threshold;
}
} // namespace ogl
#endif
