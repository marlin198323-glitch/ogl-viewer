// Full-view 8x8 depth grid alignment tests without OpenGL or GPU.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_depth_validation.h"
#include <cassert>
#include <array>
#include <limits>
#include <cmath>
int main() {
    std::array<float,64> samples{};
    samples.fill(1.f);
    samples[0] = 0.f;
    samples[63] = 0.5f;
    assert(ogl::valid_depth_grid(1920,1080,8,8,samples.data()));
    assert(ogl::valid_depth_grid(960,540,8,8,samples.data()));
    assert(ogl::valid_depth_grid(8,8,8,8,samples.data()));
    assert(!ogl::valid_depth_grid(7,8,0,0,samples.data()));
    assert(!ogl::valid_depth_grid(8,7,0,0,samples.data()));
    assert(!ogl::valid_depth_grid(8,8,9,8,samples.data()));
    assert(!ogl::valid_depth_grid(8,8,8,7,samples.data()));
    assert(!ogl::valid_depth_grid(0,0,0,0,samples.data()));
    assert(!ogl::valid_depth_grid(1920,1080,8,8,nullptr));
    samples[3] = std::numeric_limits<float>::quiet_NaN();
    assert(!ogl::valid_depth_grid(8,8,8,8,samples.data()));
    samples[3] = std::numeric_limits<float>::infinity();
    assert(!ogl::valid_depth_grid(8,8,8,8,samples.data()));
    samples[3] = -0.01f;
    assert(!ogl::valid_depth_grid(8,8,8,8,samples.data()));
    samples[3] = 1.01f;
    assert(!ogl::valid_depth_grid(8,8,8,8,samples.data()));
    samples[3] = 0.75f;
    assert(ogl::valid_depth_grid(8,8,8,8,samples.data()));
    // GPU camera shader and GL_NEAREST blit both sample grid centres.
    for (unsigned y=0; y<8; ++y) {
        for (unsigned x=0; x<8; ++x) {
            const float u = (x + 0.5f) / 8.f;
            const float v = (y + 0.5f) / 8.f;
            const float shader_x = 2.f * u - 1.f;
            const float shader_y = 2.f * v - 1.f;
            assert(u > 0.f && u < 1.f && v > 0.f && v < 1.f);
            assert(std::fabs(shader_x - ((x + 0.5f)*0.25f - 1.f)) < 1e-7f);
            assert(std::fabs(shader_y - ((y + 0.5f)*0.25f - 1.f)) < 1e-7f);
        }
    }
}
