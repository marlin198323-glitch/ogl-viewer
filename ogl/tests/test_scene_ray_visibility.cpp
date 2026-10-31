#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_scene_ray_visibility.h"
#include <cassert>
#include <cmath>
#include <vector>
int main() {
    const float o[3]={0.f,0.f,2.f};
    const float f[3]={0.f,0.f,-1.f};
    const float r[3]={1.f,0.f,0.f};
    const float u[3]={0.f,1.f,0.f};
    const auto rays=ogl::camera_probe_rays(o,f,r,u,1.04719755f,1.f);
    ogl_rtx_triangle tri{};
    // An intentionally large single triangle covers multiple rays.
    const float coords[9]={-5.f,-5.f,0.f,5.f,-5.f,0.f,0.f,5.f,0.f};
    for (unsigned i=0;i<9;++i) tri.position[i]=coords[i];
    ogl::SceneRayHits hits{};
    ogl::retain_nearest_camera_triangles(rays,hits,tri);
    unsigned coverage=0;
    for (const auto& hit:hits)
        if (std::isfinite(hit.distance)) ++coverage;
    assert(coverage>=16);
    assert(ogl::count_camera_hit_samples(hits)==coverage);
    std::vector<ogl_rtx_triangle> anchored;
    const unsigned count=ogl::append_camera_hit_triangles(hits,anchored,8192);
    assert(count==1 && anchored.size()==1);
    // A large triangle covers multiple actual rays, despite being one
    // distinct anchoring triangle in the scene buffer.
    assert(coverage>anchored.size());
    const auto none=ogl::SceneRayHits{};
    assert(ogl::count_camera_hit_samples(none)==0);
    assert(ogl::append_camera_hit_triangles(none,anchored,8192)==0);
    // A second, nearer face replaces each overlapped camera hit.
    ogl_rtx_triangle front=tri;
    for (unsigned v=0;v<3;++v) front.position[3*v+2]=1.f;
    ogl::retain_nearest_camera_triangles(rays,hits,front);
    for (const auto& hit:hits)
        if (std::isfinite(hit.distance))
            assert(hit.distance<3.f);
    assert(ogl::ray_hits_sphere(rays[27],o,1.f));
    const float behind[3]={0.f,0.f,100.f};
    assert(!ogl::ray_hits_sphere(rays[27],behind,1.f));
    const float distant[3]={0.f,0.f,-20.f};
    assert(ogl::ray_hits_sphere(rays[27],distant,40.f));
    assert(!ogl::ray_hits_sphere(rays[27],distant,-1.f));
    // An actual drawable can lie far from the nominal object centre:
    // its broad-phase AABB must recover a valid camera candidate.
    const float displaced_center[3]={100.f,100.f,-10.f};
    assert(!ogl::ray_hits_sphere(rays[27],displaced_center,2.f));
    const float actual_low[3]={-10.f,-10.f,-4.f};
    const float actual_high[3]={10.f,10.f,-1.f};
    assert(ogl::ray_hits_aabb(rays[27],actual_low,actual_high));
    float box_entry=-10.f;
    assert(ogl::ray_aabb_entry_distance(
        rays[27],actual_low,actual_high,box_entry));
    assert(box_entry>=0.f && box_entry<10.f);
    const float close_low[3]={-10.f,-10.f,0.f};
    const float close_high[3]={10.f,10.f,1.f};
    float close_entry=-1.f;
    assert(ogl::ray_aabb_entry_distance(
        rays[27],close_low,close_high,close_entry));
    assert(close_entry<=box_entry);
    const float miss_low[3]={40.f,40.f,-4.f};
    const float miss_high[3]={45.f,45.f,-1.f};
    assert(!ogl::ray_hits_aabb(rays[27],miss_low,miss_high));
    box_entry=123.f;
    assert(!ogl::ray_aabb_entry_distance(rays[27],
        miss_low,miss_high,box_entry));
    assert(box_entry==123.f);
    const float swapped_low[3]={20.f,20.f,20.f};
    const float swapped_high[3]={-20.f,-20.f,-20.f};
    assert(!ogl::ray_hits_aabb(rays[27],swapped_low,swapped_high));
    assert(!ogl::ray_hits_aabb(rays[27],actual_low,actual_high,-1.f));
}
