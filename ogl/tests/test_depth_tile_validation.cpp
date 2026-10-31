// Portable deferred depth ABI tests without OpenGL, Vulkan or GPU.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ogl/ogl_depth_validation.h"
#include <cassert>
#include <array>
#include <limits>
int main() {
    std::array<float,64> samples{};
    samples.fill(1.f);
    samples[0] = 0.f;
    samples[63] = 0.5f;
    assert(ogl::valid_depth_tile(1920,1080,956,536,samples.data()));
    assert(ogl::valid_depth_tile(960,540,476,266,samples.data()));
    assert(ogl::valid_depth_tile(8,8,0,0,samples.data()));
    assert(!ogl::valid_depth_tile(7,8,0,0,samples.data()));
    assert(!ogl::valid_depth_tile(8,7,0,0,samples.data()));
    assert(!ogl::valid_depth_tile(8,8,1,0,samples.data()));
    assert(!ogl::valid_depth_tile(8,8,0,1,samples.data()));
    assert(!ogl::valid_depth_tile(0,0,0,0,samples.data()));
    assert(!ogl::valid_depth_tile(1920,1080,956,536,nullptr));
    samples[3] = std::numeric_limits<float>::quiet_NaN();
    assert(!ogl::valid_depth_tile(8,8,0,0,samples.data()));
    samples[3] = std::numeric_limits<float>::infinity();
    assert(!ogl::valid_depth_tile(8,8,0,0,samples.data()));
    samples[3] = -0.01f;
    assert(!ogl::valid_depth_tile(8,8,0,0,samples.data()));
    samples[3] = 1.01f;
    assert(!ogl::valid_depth_tile(8,8,0,0,samples.data()));
    samples[3] = 0.75f;
    assert(ogl::valid_depth_tile(8,8,0,0,samples.data()));
}
