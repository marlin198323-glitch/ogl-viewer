#include "ogl/ogl_rtx_api.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <vector>
#include <dlfcn.h>

void log_cb(int, const char*) {}

int main(int argc, char** argv) {
    assert(argc == 2);
    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    assert(library);
    const auto start = reinterpret_cast<ogl_module_start_fn>(
        dlsym(library, "ogl_module_start"));
    const auto stop = reinterpret_cast<ogl_module_stop_fn>(
        dlsym(library, "ogl_module_stop"));
    const auto status = reinterpret_cast<const ogl_rtx_status*(*)()>(
        dlsym(library, "ogl_rtx_get_status"));
    const auto submit = reinterpret_cast<int(*)(const ogl_rtx_frame_state*)>(
        dlsym(library, "ogl_rtx_submit_frame"));
    const auto count = reinterpret_cast<uint64_t(*)()>(
        dlsym(library, "ogl_rtx_received_frames"));
    const auto submit_geometry = reinterpret_cast<int(*)(const ogl_rtx_triangle*, uint32_t)>(
        dlsym(library, "ogl_rtx_submit_geometry"));
    const auto triangle_count = reinterpret_cast<uint32_t(*)()>(
        dlsym(library, "ogl_rtx_received_triangles"));
    const auto geometry_revision = reinterpret_cast<uint64_t(*)()>(
        dlsym(library, "ogl_rtx_geometry_revision"));
    const auto copy_geometry = reinterpret_cast<uint32_t(*)(ogl_rtx_triangle*,uint32_t)>(
        dlsym(library, "ogl_rtx_copy_geometry"));
    const auto submit_depth = reinterpret_cast<int(*)(const ogl_rtx_depth_debug*)>(
        dlsym(library, "ogl_rtx_submit_depth_debug"));
    const auto copy_depth = reinterpret_cast<int(*)(ogl_rtx_depth_debug*)>(
        dlsym(library, "ogl_rtx_copy_depth_debug"));
    assert(start && stop && status && submit && count &&
           submit_geometry && triangle_count &&
           geometry_revision && copy_geometry && submit_depth && copy_depth);
    ogl_host_api host{sizeof(host), OGL_MODULE_ABI_VERSION, log_cb};
    assert(start(&host) == 0);
    assert(count() == 0);

    ogl_rtx_frame_state valid{};
    valid.struct_size = sizeof(valid);
    valid.frame_version = OGL_RTX_FRAME_VERSION;
    valid.width = 3440;
    valid.height = 1440;
    valid.camera_forward[0] = 1.0f;
    valid.camera_right[1] = 1.0f;
    valid.camera_up[2] = 1.0f;
    valid.vertical_fov_radians = 1.0f;
    valid.aspect_ratio = float(valid.width) / valid.height;
    valid.sun_direction[2] = 1.0f;

    ogl_rtx_frame_state invalid = valid;
    invalid.width = 0;
    assert(submit(&invalid) == 0);
    assert(submit(nullptr) == 0);
    assert(count() == 0);
    if (status()->hardware_ready) {
        assert(submit(&valid) == 1);
        assert(submit(&valid) == 1);
        assert(count() == 2);
        ogl_rtx_depth_debug depth{};
        depth.struct_size = sizeof(depth);
        depth.version = OGL_RTX_DEPTH_DEBUG_VERSION;
        depth.viewport_width = 1720; // half-resolution deferred target
        depth.viewport_height = 720;
        depth.x = 850;
        depth.y = 350;
        for (float& pixel : depth.normalized_depth) pixel = 0.75f;
        ogl_rtx_depth_debug output{};
        output.struct_size = sizeof(output);
        output.version = OGL_RTX_DEPTH_DEBUG_VERSION;
        assert(copy_depth(&output) == 0); // no depth yet
        assert(submit_depth(&depth) == 1);
        assert(copy_depth(&output) == 1);
        assert(output.normalized_depth[63] == 0.75f);
        depth.normalized_depth[0] = std::numeric_limits<float>::quiet_NaN();
        assert(submit_depth(&depth) == 0);
        depth.normalized_depth[0] = 0.75f;
        depth.viewport_width = 1600; // incompatible aspect
        assert(submit_depth(&depth) == 0);
        depth.viewport_width = 1720;
        depth.version = 999;
        assert(submit_depth(&depth) == 0);
        depth.version = OGL_RTX_DEPTH_DEBUG_VERSION;
        assert(submit(&valid) == 1); // new frame discards older tile
        assert(copy_depth(&output) == 0);
    } else {
        assert(submit(&valid) == 0);
        assert(count() == 0);
    }
    assert(triangle_count() == 0);
    assert(geometry_revision() == 0);
    assert(copy_geometry(nullptr, 0) == 0);
    ogl_rtx_triangle valid_triangle{};
    valid_triangle.position[3] = 1.0f;
    valid_triangle.position[7] = 1.0f;
    assert(submit_geometry(nullptr, 1) == 0);
    // On machines without RT hardware the module may reject even a clear
    // request, and must never crash or claim rendering is supported.
    assert(submit_geometry(&valid_triangle, 0) == (status()->hardware_ready ? 1 : 0));
    std::vector<ogl_rtx_triangle> oversized(OGL_RTX_MAX_SCENE_TRIANGLES + 1);
    assert(submit_geometry(oversized.data(), oversized.size()) == 0);
    ogl_rtx_triangle nan_triangle = valid_triangle;
    nan_triangle.position[0] = std::numeric_limits<float>::quiet_NaN();
    assert(submit_geometry(&nan_triangle, 1) == 0);
    ogl_rtx_triangle degenerate_triangle{};
    assert(submit_geometry(&degenerate_triangle, 1) == 0);
    ogl_rtx_triangle out_of_range = valid_triangle;
    out_of_range.position[0] = 1.1e6f;
    assert(submit_geometry(&out_of_range, 1) == 0);
    if (status()->hardware_ready) {
        assert(submit_geometry(&valid_triangle, 1) == 1);
        assert(triangle_count() == 1);
        assert(geometry_revision() == 1);
        ogl_rtx_triangle copied{};
        assert(copy_geometry(&copied, 0) == 0);
        assert(copy_geometry(&copied, 1) == 1);
        for (unsigned j = 0; j < 9; ++j)
            assert(copied.position[j] == valid_triangle.position[j]);
        assert(submit_geometry(&valid_triangle, 1) == 1);
        assert(geometry_revision() == 1); // byte-identical scene deduped
        ogl_rtx_triangle moved = valid_triangle;
        moved.position[0] = 0.2f;
        assert(submit_geometry(&moved, 1) == 1);
        assert(geometry_revision() == 2);
        assert(copy_geometry(&copied, 1) == 1);
        assert(copied.position[0] == 0.2f);
        assert(submit_geometry(&nan_triangle, 1) == 0);
        assert(triangle_count() == 1); // invalid updates cannot destroy last valid geometry
        assert(submit_geometry(nullptr, 0) == 1);
        assert(triangle_count() == 0); // teleport/region change invalidates stale geometry
        assert(geometry_revision() == 3);
        assert(copy_geometry(&copied, 1) == 0);
        assert(submit_geometry(nullptr, 0) == 1);
        assert(geometry_revision() == 3); // repeated clear is idempotent
        std::vector<ogl_rtx_triangle> bulk(OGL_RTX_MAX_SCENE_TRIANGLES);
        for (size_t i = 0; i < bulk.size(); ++i) {
            bulk[i] = valid_triangle;
            const float offset = float(i) * 0.25f;
            for (unsigned vertex = 0; vertex < 3; ++vertex)
                bulk[i].position[3 * vertex] += offset;
        }
        assert(submit_geometry(bulk.data(), bulk.size()) == 1);
        assert(triangle_count() == bulk.size());
        assert(geometry_revision() == 4);
        std::vector<ogl_rtx_triangle> buffer(bulk.size());
        assert(copy_geometry(buffer.data(), bulk.size() - 1) == 0);
        assert(copy_geometry(buffer.data(), bulk.size()) == bulk.size());
        assert(buffer[1023].position[0] == bulk[1023].position[0]);
        assert(submit_geometry(bulk.data(), bulk.size()) == 1);
        assert(geometry_revision() == 4);
        assert(submit_geometry(nullptr, 0) == 1);
        assert(triangle_count() == 0);
        assert(geometry_revision() == 5);
    } else {
        assert(submit_geometry(&valid_triangle, 1) == 0);
        assert(triangle_count() == 0);
        assert(geometry_revision() == 0);
    }
    assert(status()->rendering_active == 0);
    stop();
    dlclose(library);
    std::puts("OGL RTX camera/sun frame bridge API: PASS (raytraced pixels inactive)");
}
