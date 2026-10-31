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
    assert(ogl::developer_object_scan_limit(28654u)==28654u);
    assert(ogl::developer_object_scan_start(28654u,16384u)==0u);
    assert(ogl::developer_object_scan_limit(50000u)==32768u);
    assert(ogl::developer_object_scan_start(50000u,32768u)==32768u);
    assert(ogl::developer_object_scan_start(50000u,50005u)==5u);
    assert(ogl::developer_object_scan_limit(0u)==0u);
    assert(ogl::developer_object_scan_start(0u,500u)==0u);
    assert(ogl::camera_ray_object_quota(0u,false)==0u);
    assert(ogl::camera_ray_object_quota(1u,false)==128u);
    assert(ogl::camera_ray_object_quota(2u,false)==128u);
    assert(ogl::camera_ray_object_quota(4u,false)==256u);
    assert(ogl::camera_ray_object_quota(8u,false)==512u);
    assert(ogl::camera_depth_band(5.f)==0u);
    assert(ogl::camera_depth_band(30.f)==1u);
    assert(ogl::camera_depth_band(120.f)==2u);
    assert(ogl::camera_depth_band(-1.f)==3u);
    assert(ogl::camera_depth_band(
        std::numeric_limits<float>::infinity())==3u);
    const auto order=ogl::stratified_camera_order(
        {10.f,11.f,12.f,13.f,75.f,150.f,90.f,320.f});
    // Near:mid:near:far, then repeat. All original indices appear once.
    assert((order==std::vector<size_t>{0,4,1,5,2,6,3,7}));
    assert(ogl::stratified_camera_order({6.f,10.f,15.f}) ==
           std::vector<size_t>({0u,1u,2u}));
    assert(ogl::stratified_camera_order({65.f,132.f,90.f,250.f}) ==
           std::vector<size_t>({0u,1u,2u,3u}));
    assert(ogl::stratified_camera_order({}).empty());
    const auto unknown=ogl::stratified_camera_order({0.f,-1.f,190.f});
    assert((unknown==std::vector<size_t>{0,2,1}));
    assert(ogl::camera_ray_object_quota(0u,true)==128u);
    assert(ogl::camera_ray_box_priority(4.f,1) <
           ogl::camera_ray_box_priority(70.f,64));
    assert(ogl::camera_ray_box_priority(4.f,12) <
           ogl::camera_ray_box_priority(4.f,1));
    assert(std::isinf(ogl::camera_ray_box_priority(0.f,0)));
    assert(std::isinf(ogl::camera_ray_box_priority(-1.f,20)));
    assert(std::isinf(ogl::camera_ray_box_priority(
        std::numeric_limits<float>::infinity(),1)));
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
