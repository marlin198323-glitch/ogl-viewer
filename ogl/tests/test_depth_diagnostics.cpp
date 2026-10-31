// Portable GL-depth reconstruction and classification checks.
// Mirrors the depth_guided_sun.comp math, including openGL background.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_depth_diagnostics.h"
#include <cassert>
#include <array>
#include <cmath>
#include <limits>
int main() {
    std::array<float,64> grid{};
    grid.fill(1.0f);
    ogl::DepthStats stats{};
    assert(ogl::summarize_depth_grid(grid.data(),3440,1390,0.1f,256.f,stats));
    assert(stats.surface_pixels==0 && stats.background_pixels==64);
    assert(stats.view_distance_min==0.f);
    grid.fill(0.5f);
    assert(ogl::summarize_depth_grid(grid.data(),3440,1390,0.1f,256.f,stats));
    assert(stats.surface_pixels==64 && stats.background_pixels==0);
    assert(stats.depth_min==0.5f && stats.depth_max==0.5f);
    assert(stats.view_distance_min > 0.19f && stats.view_distance_max < 0.21f);
    grid[0]=1.f;grid[17]=1.f;
    assert(ogl::summarize_depth_grid(grid.data(),3440,1390,0.1f,256.f,stats));
    assert(stats.surface_pixels==62 && stats.background_pixels==2);
    assert(!ogl::gl_depth_to_view_distance(1.f,0.1f,256.f,stats.depth_min));
    assert(!ogl::gl_depth_to_view_distance(-0.1f,0.1f,256.f,stats.depth_min));
    assert(!ogl::gl_depth_to_view_distance(0.5f,0.f,256.f,stats.depth_min));
    assert(!ogl::gl_depth_to_view_distance(0.5f,0.1f,0.1f,stats.depth_min));
    grid[2]=std::numeric_limits<float>::quiet_NaN();
    assert(!ogl::summarize_depth_grid(grid.data(),3440,1390,0.1f,256.f,stats));
    grid[2]=0.5f;
    assert(!ogl::summarize_depth_grid(grid.data(),3440,1390,0.1f,0.1f,stats));
    assert(!ogl::summarize_depth_grid(grid.data(),3440,1390,0.1f,
                                      std::numeric_limits<float>::infinity(),stats));
    // Perspective GL z encoding -> GPU reconstruction must roundtrip at
    // typical 0.5m / 2m / 16m / 150m view-space distances.
    for (const float n : {0.05f,0.1f,0.5f}) {
        const float f=256.f;
        for (const float dist : {0.5f,2.f,16.f,150.f}) {
            const double normalized = (double(f)/(f-n))*(1.0-double(n)/dist);
            float back = 0.f;
            assert(ogl::gl_depth_to_view_distance(float(normalized),n,f,back));
            assert(std::fabs(back-dist)/dist < 0.0005f);
        }
    }
}
