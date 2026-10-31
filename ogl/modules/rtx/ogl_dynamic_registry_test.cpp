// Real RTX module object registry ABI test, no OpenSim server required.
#include "ogl/ogl_rtx_api.h"
#include "ogl/ogl_module_api.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
static void logger(int,const char*) {}
int main(int argc,char** argv) {
    if (argc!=2) return 1;
    void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if (!lib) return 1;
    auto start=reinterpret_cast<ogl_module_start_fn>(dlsym(lib,"ogl_module_start"));
    auto stop=reinterpret_cast<ogl_module_stop_fn>(dlsym(lib,"ogl_module_stop"));
    auto get_status=reinterpret_cast<const ogl_rtx_status*(*)()>(
        dlsym(lib,"ogl_rtx_get_status"));
    auto track=reinterpret_cast<decltype(&ogl_rtx_track_object)>(
        dlsym(lib,"ogl_rtx_track_object"));
    auto pose=reinterpret_cast<decltype(&ogl_rtx_track_transform)>(
        dlsym(lib,"ogl_rtx_track_transform"));
    auto erase=reinterpret_cast<decltype(&ogl_rtx_forget_object)>(
        dlsym(lib,"ogl_rtx_forget_object"));
    auto count=reinterpret_cast<decltype(&ogl_rtx_tracked_objects)>(
        dlsym(lib,"ogl_rtx_tracked_objects"));
    auto ggen=reinterpret_cast<decltype(&ogl_rtx_object_geometry_generation)>(
        dlsym(lib,"ogl_rtx_object_geometry_generation"));
    auto tgen=reinterpret_cast<decltype(&ogl_rtx_object_transform_generation)>(
        dlsym(lib,"ogl_rtx_object_transform_generation"));
    assert(start&&stop&&get_status&&track&&pose&&erase&&count&&ggen&&tgen);
    ogl_host_api host{sizeof(host),OGL_MODULE_ABI_VERSION,logger};
    assert(start(&host)==0);
    if (!get_status()->hardware_ready) {stop();dlclose(lib);return 77;}
    uint8_t id[16]{};id[0]=1;id[15]=73;
    ogl_rtx_triangle triangle{};
    triangle.position[3]=1.f;triangle.position[7]=1.f;
    const float initial[12]{1,0,0,10,0,1,0,20,0,0,1,30};
    float moved[12]{1,0,0,12,0,1,0,20,0,0,1,30};
    assert(track(id,&triangle,1,initial)==1);
    assert(count()==1 && ggen()>0 && tgen()>0);
    const uint64_t initial_geo=ggen(), initial_pose=tgen();
    assert(pose(id,moved)==1 && ggen()==initial_geo);
    assert(tgen()==initial_pose+1);
    assert(pose(id,moved)==1 && tgen()==initial_pose+1);
    triangle.position[0]=0.1f;
    assert(track(id,&triangle,1,moved)==1);
    assert(ggen()==initial_geo+1 && tgen()==initial_pose+1);
    moved[2]=NAN;
    assert(pose(id,moved)==0 && tgen()==initial_pose+1);
    assert(erase(id)==1 && count()==0);
    stop();
    dlclose(lib);
    std::puts("PASS: real RTX module tracks one UUID's local mesh and per-frame pose independently");
}
