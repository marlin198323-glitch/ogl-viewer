// Terrain generator tests: portable across Linux and Windows, no OpenGL.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_terrain_patch.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>
int main() {
    float origin[3]{1000.f,2000.f,12.f};
    std::vector<ogl_rtx_triangle> t;
    const auto flat=[](float x,float y) { return 3.f+0.02f*x+0.03f*y; };
    auto count=ogl::append_nearby_terrain_triangles(t,128.f,128.f,256.f,
                                                      origin,8192,flat);
    assert(count==800 && t.size()==800);
    for (const auto& tri:t) {
        assert(ogl::is_valid_triangle(tri));
        assert(tri.position[0]>=1000.f && tri.position[0]<=1256.f);
        assert(tri.position[1]>=2000.f && tri.position[1]<=2256.f);
        for(unsigned i=2;i<9;i+=3)
            assert(std::isfinite(tri.position[i]));
    }
    // Centre grid, not a grid over the entire OpenSim region.
    assert(t.front().position[0] >= 1075.f &&
           t.front().position[0] <= 1090.f);
    t.clear();
    count=ogl::append_nearby_terrain_triangles(t,2.f,3.f,256.f,
                                                origin,8192,flat);
    assert(count>0 && count<800); // skip cells off region bounds
    t.clear();
    count=ogl::append_nearby_terrain_triangles(t,128.f,128.f,256.f,
                                                origin,40,flat);
    assert(count==40 && t.size()==40); // strict GPU memory budget
    t.clear();
    const auto invalid=[](float x,float) {
        return x<125.f ? std::numeric_limits<float>::quiet_NaN():4.f;
    };
    count=ogl::append_nearby_terrain_triangles(t,128.f,128.f,256.f,
                                                origin,8192,invalid);
    assert(count>0 && count<800);
    for(const auto& tri:t) assert(ogl::is_valid_triangle(tri));
    t.clear();
    assert(ogl::append_nearby_terrain_triangles(t,128.f,128.f,0.f,
                                                 origin,8192,flat)==0);
    assert(ogl::append_nearby_terrain_triangles(t,128.f,128.f,256.f,
                                                 nullptr,8192,flat)==0);
    // Mid/far sparse rings must extend beyond the 100m near patch,
    // preserve strict GPU/caller budgets and never create invalid geometry.
    t.clear();
    const auto mid=ogl::append_terrain_lod_ring(t,128.f,128.f,256.f,
        origin,10.f,12,50.f,320u,8192u,flat);
    assert(mid>0 && mid<=320u && mid%2==0);
    const auto far=ogl::append_terrain_lod_ring(t,128.f,128.f,256.f,
        origin,20.f,12,120.f,320u,8192u,flat);
    assert(far>0 && far<=320u && far%2==0);
    assert(t.size()==mid+far);
    bool saw_mid=false, saw_far=false;
    for (const auto& tri:t) {
        assert(ogl::is_valid_triangle(tri));
        assert(tri.position[0]>=1000.f && tri.position[0]<=1256.f);
        assert(tri.position[1]>=2000.f && tri.position[1]<=2256.f);
        const float dx=tri.position[0]-1128.f;
        const float dy=tri.position[1]-2128.f;
        const float distance=std::sqrt(dx*dx+dy*dy);
        saw_mid=saw_mid || (distance>55.f && distance<120.f);
        saw_far=saw_far || distance>125.f;
    }
    assert(saw_mid && saw_far);
    assert(ogl::append_terrain_lod_ring(t,128.f,128.f,256.f,
        origin,20.f,12,120.f,320u,t.size(),flat)==0);
    assert(ogl::append_terrain_lod_ring(t,128.f,128.f,256.f,
        origin,0.f,12,120.f,320u,8192u,flat)==0);
    assert(ogl::append_terrain_lod_ring(t,128.f,128.f,256.f,
        origin,20.f,0,120.f,320u,8192u,flat)==0);
    // The public OpenGL geometry path does not run this function at all.
}
