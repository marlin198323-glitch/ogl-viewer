#ifndef OGL_CAMERA_VALIDATION_H
#define OGL_CAMERA_VALIDATION_H

#include "ogl_rtx_api.h"
#include <cmath>

namespace ogl {
// Portable API boundary. The basis comes from Firestorm's real camera.
// Validate before copying frame or using Vulkan push constants.
inline bool valid_camera_frame(const ogl_rtx_frame_state& f) {
    if (f.struct_size != sizeof(f) || f.frame_version != OGL_RTX_FRAME_VERSION ||
        f.width == 0 || f.height == 0 || f.width > 32768 || f.height > 32768 ||
        !std::isfinite(f.vertical_fov_radians) ||
        f.vertical_fov_radians < 0.05f || f.vertical_fov_radians > 3.1f ||
        !std::isfinite(f.aspect_ratio) ||
        f.aspect_ratio < 0.1f || f.aspect_ratio > 12.f)
        return false;

    const float* vectors[] = {f.camera_forward, f.camera_right, f.camera_up};
    for (const float* v : vectors) {
        float length_sq = 0.f;
        for (unsigned i=0; i<3; ++i) {
            if (!std::isfinite(v[i])) return false;
            length_sq += v[i]*v[i];
        }
        if (!std::isfinite(length_sq) || length_sq < 0.5f || length_sq > 1.5f)
            return false;
    }
    for (unsigned a=0; a<3; ++a) {
        if (!std::isfinite(f.camera_position[a]) ||
            std::fabs(f.camera_position[a]) > 1.e6f ||
            !std::isfinite(f.sun_direction[a]))
            return false;
    }
    for (unsigned a=0; a<3; ++a)
        for (unsigned b=a+1; b<3; ++b) {
            float dot = 0.f;
            for (unsigned i=0; i<3; ++i) dot += vectors[a][i]*vectors[b][i];
            if (!std::isfinite(dot) || std::fabs(dot) > 0.15f)
                return false;
        }
    return true;
}
} // namespace ogl
#endif
