#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_scene_snapshot_policy.h"
#include <cassert>
#include <limits>
int main() {
    using ogl::should_publish_scene_snapshot;
    // Newly loaded scene: any valid snapshot, regardless of size.
    assert(should_publish_scene_snapshot(0,0,800,0,0.f,1.f,0));
    // 8K scene must not be silently replaced by one terrain-only scan.
    assert(!should_publish_scene_snapshot(8192,200,800,0,0.f,1.f,0));
    assert(!should_publish_scene_snapshot(8192,200,812,1,0.f,1.f,2));
    // A normal LOD change, dense candidate scene or same-sized snapshot is OK.
    assert(should_publish_scene_snapshot(8192,200,7000,160,0.f,1.f,0));
    assert(should_publish_scene_snapshot(8192,200,812,150,0.f,1.f,0));
    assert(should_publish_scene_snapshot(8192,200,8192,201,0.f,1.f,0));
    // Never reuse frozen geometry across meaningful camera motion/rotation.
    assert(should_publish_scene_snapshot(8192,200,800,0,16.f,1.f,0));
    assert(should_publish_scene_snapshot(8192,200,800,0,0.f,0.8f,0));
    // No permanent stale-state retention: 8 skipped scans maximum.
    assert(!should_publish_scene_snapshot(8192,200,800,0,0.f,1.f,7));
    assert(should_publish_scene_snapshot(8192,200,800,0,0.f,1.f,8));
    // Handle nonfinite camera values by allowing refresh, not deadlocking.
    assert(should_publish_scene_snapshot(8192,200,800,0,
        std::numeric_limits<float>::quiet_NaN(),1.f,0));
    assert(should_publish_scene_snapshot(8192,200,800,0,0.f,
        std::numeric_limits<float>::infinity(),0));
    // Small, truly sparse regions should not be frozen by a 2K threshold.
    assert(should_publish_scene_snapshot(1024,50,200,0,0.f,1.f,0));
}
