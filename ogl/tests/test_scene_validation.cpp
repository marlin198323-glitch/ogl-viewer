#include "ogl/ogl_scene_validation.h"
#include <cassert>
#include <cmath>
#include <limits>

static ogl_rtx_triangle unit_triangle() {
    ogl_rtx_triangle triangle{};
    triangle.position[3] = 1.0f;
    triangle.position[7] = 1.0f;
    return triangle;
}

int main() {
    auto t = unit_triangle();
    assert(ogl::is_valid_triangle(t));
    for (unsigned i = 0; i < 9; ++i) {
        auto invalid = t;
        invalid.position[i] = std::numeric_limits<float>::quiet_NaN();
        assert(!ogl::is_valid_triangle(invalid));
        invalid.position[i] = std::numeric_limits<float>::infinity();
        assert(!ogl::is_valid_triangle(invalid));
        invalid.position[i] = 1.01e6f;
        assert(!ogl::is_valid_triangle(invalid));
    }
    auto flat = t;
    flat.position[7] = 0.0f;
    assert(!ogl::is_valid_triangle(flat));
    auto tiny = t;
    tiny.position[3] = 1.e-4f;
    tiny.position[7] = 1.e-4f;
    assert(!ogl::is_valid_triangle(tiny));
    auto moved = t;
    for (unsigned i = 0; i < 9; i += 3) {
        moved.position[i] += 100000.0f;
        moved.position[i + 1] -= 100000.0f;
    }
    assert(ogl::is_valid_triangle(moved));
}
