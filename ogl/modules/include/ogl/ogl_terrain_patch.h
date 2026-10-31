#ifndef OGL_TERRAIN_PATCH_H
#define OGL_TERRAIN_PATCH_H
#include "ogl_scene_validation.h"
#include <cmath>
#include <cstdint>
#include <vector>

// Bounded, vendor-neutral approximate terrain occluder capture. This is
// only for the optional ray-query diagnostic, NOT a replacement for
// Firestorm's actual terrain mesh or a production terrain acceleration
// structure. World coords = region-local XY/Z plus origin_agent XYZ.
namespace ogl {
template<class HeightSample>
std::size_t append_nearby_terrain_triangles(
    std::vector<ogl_rtx_triangle>& out,
    float eye_x_region, float eye_y_region, float region_width,
    const float origin_agent[3], std::size_t triangle_budget,
    HeightSample sample_height)
{
    constexpr int radius_cells = 10;
    constexpr float step = 5.f; // 100m wide grid centered on current view
    if (!origin_agent || !std::isfinite(eye_x_region) ||
        !std::isfinite(eye_y_region) ||
        !std::isfinite(region_width) || region_width < step ||
        triangle_budget < 2 || out.size() >= triangle_budget)
        return 0;
    for (unsigned i=0;i<3;++i)
        if (!std::isfinite(origin_agent[i])) return 0;
    const std::size_t initial = out.size();
    const float x_start = std::floor(eye_x_region/step)*step;
    const float y_start = std::floor(eye_y_region/step)*step;
    for (int y = -radius_cells; y < radius_cells; ++y) {
        for (int x = -radius_cells; x < radius_cells; ++x) {
            if (out.size() + 2 > triangle_budget) return out.size() - initial;
            const float x0 = x_start + float(x)*step;
            const float x1 = x0 + step;
            const float y0 = y_start + float(y)*step;
            const float y1 = y0 + step;
            if (x0 < 0.f || y0 < 0.f ||
                x1 > region_width || y1 > region_width) continue;
            const float z00 = sample_height(x0, y0);
            const float z10 = sample_height(x1, y0);
            const float z01 = sample_height(x0, y1);
            const float z11 = sample_height(x1, y1);
            if (!std::isfinite(z00) || !std::isfinite(z10) ||
                !std::isfinite(z01) || !std::isfinite(z11)) continue;
            const float pts[4][3] = {
                {x0+origin_agent[0], y0+origin_agent[1], z00+origin_agent[2]},
                {x1+origin_agent[0], y0+origin_agent[1], z10+origin_agent[2]},
                {x0+origin_agent[0], y1+origin_agent[1], z01+origin_agent[2]},
                {x1+origin_agent[0], y1+origin_agent[1], z11+origin_agent[2]}
            };
            // Consistent upward-facing triangles; Vulkan diagnostic geometry
            // is explicitly opaque and two-sided, irrespective of winding.
            constexpr unsigned indices[2][3] = {{0,1,2},{1,3,2}};
            for (const auto& idx : indices) {
                ogl_rtx_triangle tri{};
                for (unsigned corner = 0; corner < 3; ++corner)
                    for (unsigned axis=0; axis<3; ++axis)
                        tri.position[corner*3+axis] = pts[idx[corner]][axis];
                if (is_valid_triangle(tri)) out.push_back(tri);
            }
        }
    }
    return out.size() - initial;
}
} // namespace ogl
#endif
