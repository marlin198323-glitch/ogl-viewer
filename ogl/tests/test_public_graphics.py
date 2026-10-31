"""Distribution contracts: every install, never one developer's GPU/profile."""
from pathlib import Path
import xml.etree.ElementTree as ET
import unittest

ROOT = Path(__file__).resolve().parents[2]
BUILTIN = ROOT / "indra/newview/app_settings/ogl/NexVerse%20Cinematic.xml"


class PublicGraphicsContracts(unittest.TestCase):
    def test_universal_preset_is_bundled_without_any_personal_settings(self):
        root = ET.parse(BUILTIN).getroot()
        self.assertEqual(root.tag, "llsd")
        controls = root.find("map")
        self.assertIsNotNone(controls)
        items = {k.text: v for k,v in zip(controls[0::2], controls[1::2])}
        self.assertEqual(len(items), 19)
        for key in ("RenderShadowDetail", "RenderScreenSpaceReflections",
                    "RenderReflectionProbeLevel", "RenderWaterRefResolution"):
            self.assertIn(key, items)
        data = BUILTIN.read_text()
        self.assertNotIn("/home/jam", data)
        self.assertNotIn("4070", data)
        self.assertNotIn("nvidia", data.lower())
        self.assertNotIn("password", data.lower())

    def test_native_viewer_installs_optional_preset_only_when_missing(self):
        text = (ROOT / "indra/newview/llpresetsmanager.cpp").read_text()
        self.assertIn('subdirectory == PRESETS_GRAPHIC', text)
        self.assertIn('LL_PATH_APP_SETTINGS, "ogl", name', text)
        self.assertIn('!gDirUtilp->fileExists(target)', text)
        self.assertIn('LLFile::copy(source, target)', text)
        manifest = (ROOT / "indra/newview/viewer_manifest.py").read_text()
        self.assertIn('self.path("NexVerse%20Cinematic.xml")', manifest)
        self.assertIn("ogl_rtx.dll", manifest)
        self.assertIn("ogl_rtx.so", manifest)

    def test_vulkan_rt_uses_features_not_a_specific_gpu_brand(self):
        source = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        self.assertNotIn('gpu.vendorID !=', source)
        self.assertNotIn('0x10DE', source)
        self.assertIn('VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME', source)
        self.assertIn('bufferDeviceAddress == VK_TRUE', source)
        self.assertIn('gpu.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU', source)
        self.assertIn('status.hardware_ready = 1', source)
        self.assertIn('rendering_active=0', source)

    def test_linux_and_windows_module_loading_without_user_paths(self):
        plugin = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        host = (ROOT / "indra/newview/oglrtxhost.cpp").read_text()
        cmake = (ROOT / "indra/newview/CMakeLists.txt").read_text()
        self.assertIn('LoadLibraryExW(L"vulkan-1.dll"', plugin)
        self.assertIn('LOAD_LIBRARY_SEARCH_SYSTEM32', plugin)
        self.assertIn('LoadLibraryExW(dll.c_str()', host)
        self.assertIn('GetModuleFileNameW(', host)
        self.assertIn('dlopen("./lib/ogl_rtx.so"', host)
        self.assertIn('add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../ogl/modules"', cmake)
        self.assertIn('add_dependencies(${VIEWER_BINARY_NAME} ogl_rtx_stage)', cmake)

    def test_real_vulkan_blas_tlas_gpu_smoke_is_cross_vendor_and_optional(self):
        src = (ROOT / "ogl/modules/rtx/vulkan_blas_smoke.cpp").read_text()
        cmake = (ROOT / "ogl/modules/CMakeLists.txt").read_text()
        self.assertIn('VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR', src)
        self.assertIn('VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR', src)
        self.assertIn('build_blas(c.command, 1, &build_info, ranges)', src)
        self.assertIn('build_blas(c.command, 1, &top_build, top_ranges)', src)
        self.assertIn('top_range.primitiveCount = instance_count', src)
        self.assertIn('range.primitiveCount = primitive_count', src)
        self.assertIn('ogl_vulkan_module_scene_64', cmake)
        self.assertIn('ogl_vulkan_module_scene_1024', cmake)
        self.assertIn('copy(scene.data(), count) == count', src)
        self.assertIn('VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR', src)
        self.assertIn('instance.accelerationStructureReference = blas_address', src)
        self.assertIn('vkQueueWaitIdle(c.queue)', src)
        self.assertIn('vkResetCommandPool(c.device, c.pool, 0)', src)
        self.assertIn('return 77', src)
        self.assertNotIn('vendorID !=', src)
        self.assertIn('SKIP_RETURN_CODE 77 TIMEOUT 30', cmake)
        self.assertIn('ogl_vulkan_blas_tlas_gpu', cmake)

    def test_spirv_shader_is_valid_binary_and_matches_shadow_contract(self):
        from struct import unpack
        path = ROOT / "indra/newview/app_settings/ogl/rtx/ray_query_shadow.comp.spv"
        shader = ROOT / "ogl/modules/rtx/shaders/ray_query_shadow.comp"
        data = path.read_bytes()
        self.assertGreater(len(data), 200)
        self.assertEqual(len(data) % 4, 0)
        self.assertEqual(unpack("<I", data[:4])[0], 0x07230203)
        source = shader.read_text()
        self.assertIn("GL_EXT_ray_query", source)
        self.assertIn("layout(local_size_x = 8, local_size_y = 8", source)
        self.assertIn("rayQueryInitializeEXT", source)
        self.assertIn("rayQueryProceedEXT", source)
        self.assertIn("uint occlusion[64]", source)

    def test_geometry_validation_shared_between_viewer_and_module(self):
        pipeline = (ROOT / "indra/newview/pipeline.cpp").read_text()
        module = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        self.assertIn("ogl::is_valid_triangle(triangle)", pipeline)
        self.assertIn("ogl::is_valid_triangle(triangles[i])", module)
        self.assertIn("ogl_rtx_geometry_revision", module)
        self.assertIn("ogl_rtx_copy_geometry", module)
        self.assertIn("OGL RTX CPU scene snapshot: triangles=", module)
        self.assertIn("OGL RTX CPU scene cleared revision=", module)

    def test_opt_in_gpu_scene_probe_is_packaged_but_disabled_by_default(self):
        gpu = (ROOT / "ogl/modules/rtx/vulkan_blas_smoke.cpp").read_text()
        module = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        cmake = (ROOT / "indra/newview/CMakeLists.txt").read_text()
        manifest = (ROOT / "indra/newview/viewer_manifest.py").read_text()
        shader = (ROOT / "ogl/modules/rtx/shaders/ray_query_shadow.comp").read_text()
        self.assertIn("ogl_gpu_probe_run(", gpu)
        self.assertIn("vkCmdPushConstants(", gpu)
        self.assertIn("OGL_RTX_SCENE_PROBE", module)
        self.assertIn('std::strcmp(env, "1") == 0', module)
        self.assertIn("std::chrono::seconds(60)", module)
        self.assertIn("probe_worker->enqueue", module)
        self.assertIn("probe_worker.reset();", module)
        self.assertIn("ogl::AsyncGpuQueue", module)
        queue = (ROOT / "ogl/modules/include/ogl/ogl_async_gpu_queue.h").read_text()
        self.assertIn("m_stats.disabled = true", queue)
        self.assertIn("std::thread m_thread;", queue)
        self.assertIn("m_pending = std::move(job)", queue)
        self.assertIn("m_min_valid_revision", queue)
        self.assertIn("gpu_probe_cleanup();", module)
        self.assertIn("rendering_active=0", module)
        self.assertIn("add_dependencies(${VIEWER_BINARY_NAME} ogl_gpu_probe_stage)", cmake)
        self.assertIn("ogl_gpu_probe.dll", manifest)
        self.assertIn("ogl_gpu_probe.so", manifest)
        self.assertIn("layout(push_constant)", shader)
        self.assertIn("params.ray_direction.xyz", shader)

    def test_persistent_gpu_resources_are_vendor_neutral_and_opt_in(self):
        source = (ROOT / "ogl/modules/rtx/vulkan_blas_smoke.cpp").read_text()
        api = (ROOT / "ogl/modules/include/ogl/ogl_gpu_probe_api.h").read_text()
        test = (ROOT / "ogl/modules/rtx/ogl_gpu_persistent_test.cpp").read_text()
        log = (ROOT / "ogl/scripts/ogl-test-report.py").read_text()
        self.assertIn("#define OGL_GPU_PROBE_VERSION 2u", api)
        for key in ("vulkan_device_creations", "blas_builds", "cache_hits"):
            self.assertIn(key, api)
        self.assertIn("thread_local std::unique_ptr<VulkanContext>", source)
        self.assertIn("reuse_geometry", source)
        self.assertIn("persistent->reset_query()", source)
        self.assertIn("persistent->reset_task()", source)
        self.assertIn("cached_scene.assign(scene, scene + wanted_triangles)", source)
        self.assertIn("ogl_gpu_device_persistent_and_scene_cache",
                      (ROOT / "ogl/modules/CMakeLists.txt").read_text())
        self.assertIn("cached_large.blas_builds == 3", test)
        self.assertIn("cached_large.cache_hits == 2", test)
        self.assertIn("RTX GPU reuse:", log)
        self.assertNotIn("0x10DE", source)

    def test_no_rt_hardware_preserves_opengl_fallback(self):
        module = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        self.assertIn('No suitable Vulkan ray tracing GPU found; OpenGL fallback.', module)
        host = (ROOT / "indra/newview/oglrtxhost.cpp").read_text()
        self.assertIn('OpenGL remains the active renderer', host)


if __name__ == "__main__":
    unittest.main()
