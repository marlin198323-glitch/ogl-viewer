#ifndef OGL_SCENE_VALIDATION_H
#define OGL_SCENE_VALIDATION_H

#include "ogl_rtx_api.h"
#include <cmath>

// Shared, vendor-neutral geometry guard for the OpenSim renderer and RTX ABI.
// Keep mesh validation independent of Vulkan, drivers, login and file I/O.
namespace ogl {
inline bool is_valid_triangle(const ogl_rtx_triangle& triangle) {
    for (float coordinate : triangle.position)
        if (!std::isfinite(coordinate) || std::fabs(coordinate) > 1.0e6f)
            return false;

    const float* v = triangle.position;
    const float ax = v[3] - v[0], ay = v[4] - v[1], az = v[5] - v[2];
    const float bx = v[6] - v[0], by = v[7] - v[1], bz = v[8] - v[2];
    const float cx = ay * bz - az * by;
    const float cy = az * bx - ax * bz;
    const float cz = ax * by - ay * bx;
    const float area_sqr = cx * cx + cy * cy + cz * cz;
    return std::isfinite(area_sqr) && area_sqr > 1.0e-12f;
}
} // namespace ogl

#endif
