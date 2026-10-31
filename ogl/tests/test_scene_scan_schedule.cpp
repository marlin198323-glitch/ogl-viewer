#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_scene_scan_schedule.h"
#include <cassert>
int main() {
    using ogl::scene_scan_due;
    // Original public OpenGL path is unaffected, with or without region.
    assert(!scene_scan_due(179,false,true,25000,0,false,600000,600000));
    assert(scene_scan_due(180,false,false,0,0,false,0,0));
    // No early scan before actual region and object arrival.
    assert(!scene_scan_due(2,true,false,28000,0,false,20000,20000));
    assert(!scene_scan_due(2,true,true,0,0,false,20000,20000));
    assert(!scene_scan_due(2,true,true,100,0,false,4999,20000));
    assert(!scene_scan_due(2,true,true,100,0,false,5000,9999));
    // A 0.1 FPS client can begin its first diagnostic scan at frame 2.
    assert(scene_scan_due(2,true,true,100,0,false,5000,10000));
    // Failed initial snapshots retry after a cooldown.
    assert(!scene_scan_due(2,true,true,100,0,false,20000,8000));
    assert(scene_scan_due(2,true,true,100,0,false,20000,10000));
    // Once a scene was published, don't continuously rescan every frame.
    assert(!scene_scan_due(2,true,true,100,100,true,50000,30000));
    assert(!scene_scan_due(2,true,true,22500,20000,true,50000,30000));
    assert(scene_scan_due(2,true,true,26000,20000,true,50000,30000));
    assert(scene_scan_due(2,true,true,220,40,true,50000,30000));
    assert(!scene_scan_due(2,true,true,130,40,true,50000,30000));
    assert(!scene_scan_due(2,true,true,26000,20000,true,50000,5000));
    // Existing 180-frame cadence overrides developer-only cooldown.
    assert(scene_scan_due(180,true,true,26000,20000,true,0,0));
    return 0;
}
