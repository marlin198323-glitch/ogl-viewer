// Vendor-independent object identity/dirty-tracking acceptance.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_dynamic_scene.h"
#include <cassert>
#include <limits>
#include <array>
int main() {
    ogl::DynamicScene scene;
    ogl_rtx_triangle tri{};
    tri.position[3]=1.f;
    tri.position[7]=1.f;
    ogl::ObjectKey prim{};prim[0]=4;
    ogl::ObjectKey avatar{};avatar[0]=5;
    ogl::Affine3x4 initial{
        1.f,0.f,0.f,10.f,0.f,1.f,0.f,20.f,0.f,0.f,1.f,30.f};
    assert(scene.upsert(prim,&tri,1,initial)==ogl::SceneChange::NewInstance);
    const auto original_geometry=scene.geometry_generation();
    const auto original_transform=scene.transform_generation();
    assert(scene.triangles()==1 && scene.objects()==1);
    assert(scene.upsert(prim,&tri,1,initial)==ogl::SceneChange::Unchanged);
    assert(scene.geometry_generation()==original_geometry);
    auto moved=initial;moved[3]=12.f;
    assert(scene.update_transform(prim,moved)==ogl::SceneChange::TransformChanged);
    assert(scene.geometry_generation()==original_geometry);
    assert(scene.transform_generation()==original_transform+1);
    assert(scene.find(prim)->transform[3]==12.f);
    assert(scene.upsert(avatar,&tri,1,initial)==ogl::SceneChange::NewInstance);
    assert(scene.triangles()==2 && scene.objects()==2);
    tri.position[0]=0.1f; // deformed local geometry (same topology)
    const auto prev_geom=scene.geometry_generation();
    assert(scene.upsert(prim,&tri,1,moved)==ogl::SceneChange::GeometryChanged);
    assert(scene.geometry_generation()==prev_geom+1);
    auto invalid=moved;invalid[0]=std::numeric_limits<float>::infinity();
    assert(scene.update_transform(prim,invalid)==ogl::SceneChange::Invalid);
    assert(scene.find(prim)->transform[3]==12.f);
    assert(scene.erase(avatar)==ogl::SceneChange::Removed);
    assert(scene.triangles()==1 && scene.objects()==1);
    assert(scene.erase(avatar)==ogl::SceneChange::Unchanged);
    auto counted=0u;
    scene.visit([&](const ogl::SceneEntry& entry) {
        ++counted;
        assert(entry.key==prim && entry.local_geometry.size()==1);
    });
    assert(counted==1);
    scene.clear();
    assert(scene.objects()==0 && scene.triangles()==0 && !scene.find(prim));
    // Full UUID identity rather than low-bits hash prevents accidental
    // aliasing between unrelated region objects.
    ogl::ObjectKey another{}; another[15]=4;
    assert(prim!=another);
}
