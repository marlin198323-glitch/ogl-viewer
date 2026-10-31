// Portable scene-selection checks, no graphics device or simulator needed.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_scene_sampling.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <vector>
int main() {
    for (uint32_t total: {1u,4u,32u,33u,100u,500u,65535u}) {
        const uint32_t count=std::min(total,32u);
        uint32_t prev=0;
        for (uint32_t i=0;i<count;++i) {
            const uint32_t sampled=ogl::spread_triangle_index(total,count,i);
            assert(sampled<total);
            if (i>0) assert(sampled>prev);
            prev=sampled;
        }
    }
    assert(ogl::spread_triangle_index(100,10,0)==5u);
    assert(ogl::spread_triangle_index(100,10,9)==95u);
    assert(ogl::spread_triangle_index(100,0,0)==UINT32_MAX);
    assert(ogl::spread_triangle_index(100,101,0)==UINT32_MAX);
    assert(ogl::spread_triangle_index(100,10,10)==UINT32_MAX);
    const float near_front=ogl::scene_priority(5.f,1.f,2.f);
    const float far_front=ogl::scene_priority(30.f,1.f,2.f);
    const float near_back=ogl::scene_priority(5.f,-1.f,2.f);
    assert(near_front<far_front);
    assert(near_front<near_back);
    assert(std::isinf(ogl::scene_priority(
        std::numeric_limits<float>::quiet_NaN(),1.f,1.f)));
    assert(std::isinf(ogl::scene_priority(10.f,2.f,1.f)));
    assert(std::isinf(ogl::scene_priority(10.f,1.f,-1.f)));
    // A real 223 m GL surface must be eligible, unlike the old 90 m
    // radius, but distant objects behind the viewer are excluded.
    assert(ogl::scene_candidate_in_range(223.f,0.95f,2.f));
    assert(ogl::scene_candidate_in_range(75.f,-1.f,2.f));
    assert(!ogl::scene_candidate_in_range(223.f,-0.5f,2.f));
    assert(ogl::scene_candidate_in_range(300.f,1.f,2.f));
    assert(ogl::scene_candidate_in_range(632.f,0.5f,2.f));
    assert(!ogl::scene_candidate_in_range(800.f,1.f,2.f));
    assert(!ogl::scene_candidate_in_range(632.f,-0.5f,2.f));
    assert(ogl::scene_candidate_in_range(93.f,-0.5f,8.f));
    assert(!ogl::scene_candidate_in_range(
        std::numeric_limits<float>::infinity(),1.f,1.f));
    assert(!ogl::scene_candidate_in_range(10.f,1.f,-1.f));
    // 8192 triangles after reserving a bounded terrain sample can feed
    // many independent foreground objects rather than 16 huge meshes.
    uint32_t used=800u;
    uint32_t objects=0;
    while (used + 128u<=8192u) {used+=128u;++objects;}
    assert(objects>=50u && used<=8192u);
}
