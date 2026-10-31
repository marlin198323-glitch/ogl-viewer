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
    std::vector<ogl_rtx_triangle> anchored;
    const unsigned count=ogl::append_camera_hit_triangles(hits,anchored,8192);
    assert(count==1 && anchored.size()==1);
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
}
