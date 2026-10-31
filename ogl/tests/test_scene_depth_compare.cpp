#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_scene_depth_compare.h"
#include <array>
#include <cassert>
#include <cmath>
#include <limits>

int main() {
    const float eye[3]={0.f,0.f,2.f};
    const float forward[3]={0.f,0.f,-1.f};
    const float right[3]={1.f,0.f,0.f};
    const float up[3]={0.f,1.f,0.f};
    const float coords[2][9]={
        {-10.f,-10.f,0.f,10.f,-10.f,0.f,-10.f,10.f,0.f},
        {10.f,-10.f,0.f,10.f,10.f,0.f,-10.f,10.f,0.f}};
    std::array<ogl_rtx_triangle,2> quads{};
    for (unsigned t=0;t<2;++t)
        for (unsigned j=0;j<9;++j)quads[t].position[j]=coords[t][j];
    constexpr float near=0.1f, far=256.f;
    std::array<float,64> depth{};
    auto encode=[](float distance) ->float{
        return (far/(far-near))*(1.f-near/distance);
    };
    depth.fill(encode(2.f));
    ogl::SceneDepthAgreement ref{};
    auto check=[&]() {
        return ogl::compare_cpu_scene_to_gl_depth(quads.data(),2,depth.data(),
            eye,forward,right,up,1.04719755f,1.f,near,far,ref);
    };
    assert(check());
    assert(ref.surface==64 && ref.background==0);
    assert(ref.ray_samples==64 && ref.primary_hits==64);
    assert(ref.matching==64 && ref.closer==0 && ref.farther==0);
    assert(ref.ranges[0].surface==64);
    assert(ref.ranges[0].matching==64);
    assert(ref.ranges[1].surface==0);
    assert(ref.ranges[2].surface==0);
    depth[0]=1.f; // GL background, no eligible matching surface
    assert(check());
    assert(ref.surface==63 && ref.background==1 && ref.matching==63);
    depth[1]=encode(3.f); // reference CPU scene closer than GL
    assert(check());
    assert(ref.closer==1 && ref.matching==62);
    depth[2]=encode(1.f); // reference CPU scene farther than GL
    assert(check());
    assert(ref.closer==1 && ref.farther==1 && ref.matching==61);
    assert(ref.ranges[0].primary_hits==63);
    depth[3]=encode(55.f);
    depth[4]=encode(160.f);
    assert(check());
    assert(ref.ranges[0].surface==61);
    assert(ref.ranges[1].surface==1);
    assert(ref.ranges[2].surface==1);
    assert(ref.ranges[1].farther==0 && ref.ranges[1].closer==1);
    assert(ref.ranges[2].closer==1);
    assert(!ogl::compare_cpu_scene_to_gl_depth(quads.data(),9000,depth.data(),
        eye,forward,right,up,1.f,1.f,near,far,ref));
    assert(!ogl::compare_cpu_scene_to_gl_depth(quads.data(),2,depth.data(),
        eye,forward,right,up,1.f,1.f,100.f,10.f,ref));
}
