"""Distribution contracts: every install, never one developer's GPU/profile."""
from pathlib import Path
import xml.etree.ElementTree as ET
import unittest

ROOT = Path(__file__).resolve().parents[2]
BUILTIN = ROOT / "indra/newview/app_settings/ogl/NexVerse%20Cinematic.xml"


class PublicGraphicsContracts(unittest.TestCase):
    def test_p04_renderer_inventory_open_gl_is_the_only_world_renderer(self):
        display=(ROOT/"indra/newview/llviewerdisplay.cpp").read_text()
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        app=(ROOT/"indra/newview/llappviewer.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        cmake=(ROOT/"indra/newview/CMakeLists.txt").read_text()
        settings=(ROOT/"indra/newview/app_settings/settings.xml").read_text()
        for expression in ("gPipeline.renderGeomDeferred(",
                           "gPipeline.renderDeferredLighting()",
                           "gPipeline.renderFinalize()"):
            self.assertIn(expression,display)
        for method in ("LLPipeline::init()",
                       "LLPipeline::renderGeomDeferred(",
                       "LLPipeline::renderDeferredLighting()",
                       "LLPipeline::renderFinalize()"):
            self.assertIn(method,pipeline)
        self.assertIn("ogl_init_rtx_module();",app)
        self.assertIn("ogl_shutdown_rtx_module();",app)
        self.assertIn('dlopen("./lib/ogl_rtx.so"',host)
        self.assertIn("keeping OpenGL.",host)
        self.assertIn("OpenGL remains the active renderer.",host)
        self.assertIn("ogl_rtx_hardware_supported()",header)
        self.assertIn("ogl_rtx_rendering_active()",header)
        self.assertIn("OGL_RTX_DEBUG_OVERLAY",display)
        self.assertIn("OGL_RTX_DEPTH_GUIDED_PROBE",display)
        self.assertIn("OGL_RTX_SHADOW_VISUALIZE",pipeline)
        self.assertIn("ogl_rtx_compose_dense_sun_shadow(",pipeline)
        self.assertIn("Explicit diagnostics only. Never used as an in-world renderer.",module)
        self.assertIn("rendering_active=0",module)
        self.assertIn("ogl_rtx_stage",cmake)
        self.assertIn("ogl_gpu_probe_stage",cmake)
        self.assertIn("<key>RenderMaxOpenGLVersion</key>",settings)
        # The graphics UI now exposes the future engine, but cannot enable
        # Vulkan world rendering until the actual P05 pipeline exists.
        panel=(ROOT/"indra/newview/skins/default/xui/en/panel_preferences_graphics1.xml").read_text()
        self.assertIn('name="ogl_renderer_engine"',panel)
        self.assertIn('label="Vulkan (not implemented)"',panel)

    def test_p04_backend_contract_is_fail_closed_and_platform_neutral(self):
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        display=(ROOT/"indra/newview/llviewerdisplay.cpp").read_text()
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        self.assertIn("enum class OGLWorldBackend",header)
        self.assertIn("enum class OGLWorldFallback",header)
        self.assertIn("struct OGLWorldBackendDecision",header)
        self.assertIn("constexpr OGLWorldBackendDecision ogl_choose_world_backend(",header)
        self.assertIn("constexpr OGLWorldBackendDecision ogl_current_world_backend()",header)
        self.assertIn("OGLWorldFallback::WorldPipelineUnavailable",header)
        self.assertIn("OGLWorldFallback::DeviceUnavailable",header)
        self.assertIn("OGLWorldBackend::OpenGL, false, false",header)
        self.assertIn("static_assert(ogl_current_world_backend().active",header)
        self.assertIn("gPipeline.renderGeomDeferred(",display)
        self.assertIn("LLPipeline::renderFinalize(",pipeline)
        # The contract must neither run a Vulkan world frame nor write UI
        # settings; the real engine switch belongs to later P04/P05 blocks.
        self.assertNotIn("vkQueuePresentKHR(",header)
        self.assertNotIn("gSavedSettings",header)
        self.assertNotIn("LLRenderTarget",header)

    def test_p04_frame_lifecycle_preserves_real_opengl_world_present(self):
        source=(ROOT/"indra/newview/llviewerdisplay.cpp").read_text()
        self.assertIn('constexpr auto world_renderer = ogl_current_world_backend()',source)
        self.assertIn('world_renderer.active == OGLWorldBackend::OpenGL',source)
        self.assertIn('gPipeline.renderGeomDeferred(',source)
        self.assertIn('gPipeline.renderDeferredLighting()',source)
        swap=source.split('void swap()\n{',1)[1].split('\nvoid ',1)[0]
        self.assertIn('gViewerWindow->getWindow()->swapBuffers()',swap)
        self.assertLess(swap.index('swapBuffers()'),swap.index('ogl_record_world_frame_presented()'))
        startup=(ROOT/"indra/newview/llappviewer.cpp").read_text()
        self.assertIn('ogl_report_world_backend_startup();',startup)
        self.assertIn('ogl_report_world_backend_shutdown();',startup)
        self.assertIn('ogl_shutdown_rtx_module();',startup)
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        self.assertIn('void ogl_record_world_frame_presented();',header)

    def test_p04_graphics_engine_selector_is_explicitly_fail_closed(self):
        en=ET.parse(ROOT/"indra/newview/skins/default/xui/en/panel_preferences_graphics1.xml").getroot()
        de=ET.parse(ROOT/"indra/newview/skins/default/xui/de/panel_preferences_graphics1.xml").getroot()
        for lang,root in (("en",en),("de",de)):
            panels=[v for v in root.iter("panel") if v.get("name")=="ogl_renderer_panel"]
            self.assertEqual(len(panels),1,lang)
            self.assertEqual(sum(v.get("name")=="ogl_renderer_engine" for v in panels[0].iter()),1)
            entries=[v for v in panels[0].iter("combo_box.item")]
            self.assertEqual({v.get("value") for v in entries},{"opengl","vulkan"})
        cpp=(ROOT/"indra/newview/llfloaterpreference.cpp").read_text()
        self.assertIn('renderer->setEnabledByValue("vulkan", false)',cpp)
        self.assertIn('renderer->setSelectedByValue("opengl", true)',cpp)
        self.assertIn('void LLPanelPreferenceGraphics::onOglRendererEngineChanged()',cpp)
        self.assertIn('combo->getValue().asString() != "opengl"',cpp)
        self.assertIn('gSavedSettings.setString("OGLWorldRequestedEngine", "opengl");',cpp)
        self.assertIn('renderer_panel->getString("ogl_renderer_vram_label")',cpp)
        self.assertIn('renderer_panel->getString("ogl_renderer_unknown")',cpp)
        self.assertIn("if (version.size() > 80) version.resize(80)",cpp)
        self.assertIn('getChild<LLTextBox>("ogl_renderer_gpu")',cpp)

    def test_p04_requested_and_last_working_backend_settings_are_separate(self):
        settings=(ROOT/"indra/newview/app_settings/settings.xml").read_text()
        root=ET.fromstring(settings)
        entries=root.find("map")
        data={el.text:val for el,val in zip(entries[0::2],entries[1::2])}
        self.assertIn("OGLWorldRequestedEngine",data)
        self.assertIn("OGLWorldLastWorkingEngine",data)
        for name in ("OGLWorldRequestedEngine","OGLWorldLastWorkingEngine"):
            prop=data[name]
            tags={k.text:v for k,v in zip(prop[0::2],prop[1::2])}
            self.assertEqual(tags["Type"].text,"String")
            self.assertEqual(tags["Persist"].text,"1")
            self.assertEqual(tags["Value"].text,"opengl" if name.endswith("RequestedEngine") else None)

    def test_p04_gpu_log_is_diagnostic_not_a_vulkan_world_toggle(self):
        source=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        self.assertIn("struct OGLWorldCapabilities",header)
        self.assertIn("caps.vulkan_world_pipeline_complete = false",source)
        self.assertIn("caps.vulkan_device_usable = ogl_rtx_hardware_supported()",source)
        self.assertIn("gGLManager.mVRAMDetected > 0",source)
        self.assertIn('gSavedSettings.getU32("RenderMaxVRAMBudget")',source)
        self.assertIn("gGLManager.mDriverVersionVendorString",source)
        self.assertIn("world-pipeline-unavailable",source)
        self.assertIn("effective_VRAM_MB=",source)
        self.assertIn("caps.gl_context_ready = gGLManager.mInited",source)
        self.assertIn('World backend: OpenGL (actual)',source)
        self.assertIn('OpenGL world frame presented; last working engine saved.',source)
        self.assertIn('if (ogl_world_frame_reported || !gGLManager.mInited) return;',source)
        self.assertIn('gSavedSettings.setString("OGLWorldLastWorkingEngine", "opengl")',source)
        self.assertNotIn("gSavedSettings.setString(\"RenderMaxOpenGLVersion\"",source)

    def test_p05_surface_handoff_is_not_claimed_before_sdl_swapchain(self):
        window=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        preferences=(ROOT/"indra/newview/llfloaterpreference.cpp").read_text()
        self.assertIn("SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE",window)
        self.assertIn("SDL_GL_CreateContext(",window)
        self.assertNotIn("SDL_Vulkan_CreateSurface(",window)
        # P05 now probes a real VkSwapchainKHR on the existing X11 window,
        # but does not acquire/present or switch the OpenGL world renderer.
        self.assertIn('"vkCreateSwapchainKHR"',module)
        self.assertNotIn("vkQueuePresentKHR(",module)
        self.assertIn("surface and world renderer NOT implemented.",module)
        self.assertIn('renderer->setEnabledByValue("vulkan", false)',preferences)
        self.assertIn("VK_KHR_SWAPCHAIN_EXTENSION_NAME",module)
        # A device can advertise swapchain support without possessing a
        # surface-compatible present queue for the *actual* viewer window.

    def test_p05_actual_sdl_x11_window_surface_and_swapchain_probe(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        header=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        for symbol in (
            "ogl_vulkan_world_attach_x11_window",
            "ogl_vulkan_world_swapchain_probe_ready",
            "ogl_vulkan_world_detach_window"):
            self.assertIn(symbol,header)
            self.assertIn(symbol,module)
            self.assertIn(symbol,host)
        for symbol in ("vkCreateXlibSurfaceKHR","vkGetPhysicalDeviceSurfaceSupportKHR",
                       "vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
                       "vkGetPhysicalDeviceSurfaceFormatsKHR",
                       "vkGetPhysicalDeviceSurfacePresentModesKHR",
                       "vkCreateSwapchainKHR","vkGetSwapchainImagesKHR",
                       "vkDestroySurfaceKHR","vkDestroySwapchainKHR"):
            self.assertIn('"'+symbol+'"',module)
        self.assertIn("VK_PRESENT_MODE_FIFO_KHR",module)
        self.assertIn("detach_window(); // WSI handles must die",module)
        self.assertIn("OGL_VULKAN_WINDOW_SURFACE",host)
        self.assertIn("OGL_VULKAN_WINDOW_SURFACE",module)
        self.assertIn("LLWindowSDL::get_SDL_XWindowID()",host)
        self.assertIn("LLWindowSDL::get_SDL_Display()",host)
        self.assertIn("detach(); // default one-shot probe must leave GL alone.",host)
        self.assertNotIn("vkQueuePresentKHR(",module)
        self.assertNotIn("vkAcquireNextImageKHR(",module)
        self.assertIn("assert(window_attach(nullptr,0,1920,1080)==0)",native)
        self.assertIn("assert(window_ready() == 0)",native)
        self.assertIn("SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE",
                      (ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text())

    def test_p05_persistent_x11_wsi_window_resize_and_teardown_guard(self):
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        win=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        header=(ROOT/"indra/llwindow/llwindowsdl2.h").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        for symbol in ("setOglWindowWillDestroyCallback",
                       "setOglWindowResizedCallback",
                       "getOglNativeWindowSize"):
            self.assertIn(symbol,header)
            self.assertIn("LLWindowSDL::"+symbol,win)
            self.assertIn("LLWindowSDL::"+symbol,host)
        self.assertIn('std::getenv("OGL_VULKAN_WINDOW_PERSISTENT")',host)
        self.assertIn("ogl_x11_wsi_refresh()",host)
        self.assertIn("ogl_x11_wsi_will_destroy()",host)
        self.assertIn("if (rtx_wsi_xid==window && rtx_wsi_width==width",host)
        self.assertIn("if (gOglWindowResized)",win)
        self.assertIn("if (gOglWindowWillDestroy)",win)
        self.assertLess(win.index("if (gOglWindowWillDestroy)"),
                        win.index("SDL_QuitSubSystem(SDL_INIT_VIDEO)"))
        self.assertIn("gOglWindowResized();",win)
        self.assertIn("mCallbacks->handleResize(this, width, height);",win)
        self.assertIn("rtx_wsi_persistent=false",host)
        self.assertIn('setOglWindowResizedCallback(nullptr)',host)
        self.assertIn('setOglWindowWillDestroyCallback(nullptr)',host)
        self.assertIn("P05 persistent X11 WSI resize/window refresh:",host)
        self.assertIn("detach_window(); // WSI handles must die",module)
        self.assertNotIn("vkQueuePresentKHR(",module)
        self.assertIn('renderer->setEnabledByValue("vulkan", false)',
                      (ROOT/"indra/newview/llfloaterpreference.cpp").read_text())

    def test_p05_graphics_queue_sync_probe_does_not_present_world_frames(self):
        header=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        self.assertIn("ogl_vulkan_world_submit_sync_probe",header)
        self.assertIn("uint32_t submit_world_sync_probe()",module)
        self.assertIn('std::getenv("OGL_VULKAN_FRAME_SYNC_PROBE")',module)
        for symbol in ("vkCreateCommandPool","vkAllocateCommandBuffers",
                       "vkBeginCommandBuffer","vkEndCommandBuffer",
                       "vkCreateFence","vkQueueSubmit","vkWaitForFences",
                       "vkDestroyFence","vkDestroyCommandPool"):
            self.assertIn('"'+symbol+'"',module)
        self.assertIn("VK_COMMAND_BUFFER_LEVEL_PRIMARY",module)
        self.assertIn("VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT",module)
        self.assertIn("2000000000ULL",module)
        self.assertIn("assert(world_sync()==(ready ? 2u : 0u));",native)
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        self.assertIn('std::getenv("OGL_VULKAN_FRAME_SYNC_PROBE")',host)
        self.assertIn("ogl_vulkan_world_submit_sync_probe",host)
        self.assertIn("P05 Vulkan graphics queue sync probe: ",host)
        self.assertIn("no image present, OpenGL world unchanged.",host)
        self.assertIn('unsetenv("OGL_VULKAN_FRAME_SYNC_PROBE")',native)
        self.assertNotIn("vkQueuePresentKHR(",module)
        self.assertNotIn("vkAcquireNextImageKHR(",module)

    def test_p05_vertex_index_world_upload_is_bounded_and_rt_independent(self):
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        for symbol in ("ogl_vulkan_world_upload_triangles",
                       "ogl_vulkan_world_gpu_triangle_count"):
            self.assertIn(symbol,api)
            self.assertIn(symbol,module)
            self.assertIn(symbol,native)
        self.assertIn('std::getenv("OGL_VULKAN_WORLD_MESH_UPLOAD")',module)
        self.assertIn('std::getenv("OGL_VULKAN_WORLD_MESH_UPLOAD")',host)
        self.assertIn("VK_BUFFER_USAGE_VERTEX_BUFFER_BIT",module)
        self.assertIn("VK_BUFFER_USAGE_INDEX_BUFFER_BIT",module)
        self.assertIn("VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT",module)
        self.assertIn("VK_MEMORY_PROPERTY_HOST_COHERENT_BIT",module)
        self.assertIn("ogl::is_valid_triangle(triangles[i])",module)
        self.assertIn("world_device.clear_mesh();",module)
        self.assertIn("count>OGL_RTX_MAX_SCENE_TRIANGLES",module)
        self.assertIn("world_device.mesh_triangle_count=count",module)
        self.assertIn("world_upload(&bad,1)==0",native)
        self.assertIn("world_upload(all.data(),8192)==1",native)
        self.assertIn("world_upload(nullptr,0)==1",native)
        self.assertIn("if (!rtx_submit_geometry || !rtx_hardware_ready) return;",host)
        self.assertIn("if (world_upload_geometry && staging &&",host)
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        self.assertIn("ogl_vulkan_world_mesh_staging_enabled()",pipeline)
        self.assertIn('std::getenv("OGL_VULKAN_WORLD_MESH_UPLOAD")',pipeline)
        self.assertIn("if (publish_geometry && !triangles.empty())",pipeline)
        self.assertIn("bool ogl_vulkan_world_mesh_staging_enabled()",host)
        self.assertNotIn("vkQueuePresentKHR(",module)

    def test_p05_persistent_optin_vulkan_world_device_and_cleanup(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        header=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        self.assertIn("ogl_vulkan_world_device_bootstrapped",header)
        self.assertIn("struct WorldGraphicsDevice",module)
        for call in ("vkCreateInstance","vkEnumeratePhysicalDevices",
                     "vkGetPhysicalDeviceQueueFamilyProperties",
                     "vkEnumerateDeviceExtensionProperties","vkCreateDevice",
                     "vkGetDeviceQueue","vkDestroyDevice","vkDestroyInstance"):
            self.assertIn('"'+call+'"',module)
        self.assertIn("VK_QUEUE_GRAPHICS_BIT",module)
        self.assertIn("VK_KHR_SWAPCHAIN_EXTENSION_NAME",module)
        self.assertIn('proc("vkGetDeviceProcAddr")',module)
        self.assertIn('get_device_proc(device,"vkDestroyDevice")',module)
        self.assertIn('get_device_proc(device,"vkGetDeviceQueue")',module)
        self.assertIn("world_device.close();",module)
        self.assertIn('std::getenv("OGL_VULKAN_WORLD_BOOTSTRAP")',module)
        self.assertIn('std::strcmp(world_env,"1")==0',module)
        self.assertIn("surface and world renderer NOT implemented.",module)
        self.assertIn("status()->rendering_active == 0",native)
        self.assertIn("assert(world_device() == 0);",native)
        self.assertIn("unsetenv(\"OGL_VULKAN_WORLD_BOOTSTRAP\")",native)
        self.assertIn("setenv(\"OGL_VULKAN_WORLD_BOOTSTRAP\", \"1\", 1)",native)

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
        self.assertIn("probe_interval = std::chrono::milliseconds(60000)", module)
        self.assertIn("OGL_RTX_DEV_INTERVAL_MS", module)
        self.assertIn("probe_job_sequence", module)
        self.assertIn("schedule_probe_on_viewer_thread()", module)
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
        self.assertIn("#define OGL_GPU_PROBE_VERSION 7u", api)
        for key in ("vulkan_device_creations", "blas_builds", "cache_hits"):
            self.assertIn(key, api)
        self.assertIn("thread_local std::unique_ptr<VulkanContext>", source)
        self.assertIn("reuse_geometry", source)
        self.assertIn("persistent->reset_query()", source)
        self.assertIn("persistent->reset_task()", source)
        self.assertIn("cached_scene.assign(scene, scene + wanted_triangles)", source)
        self.assertIn("ogl_gpu_device_persistent_and_scene_cache",
                      (ROOT / "ogl/modules/CMakeLists.txt").read_text())
        self.assertIn("cached_large.blas_builds == 2", test)
        self.assertIn("moved.blas_refits == 1", test)
        self.assertIn("moved.tlas_refits == 0", test)
        self.assertIn("cached_large.cache_hits == 2", test)
        self.assertIn("RTX GPU reuse:", log)
        self.assertNotIn("0x10DE", source)

    def test_firestorm_host_can_read_verified_debug_mask_without_compositing(self):
        host = (ROOT / "indra/newview/oglrtxhost.cpp").read_text()
        header = (ROOT / "indra/newview/oglrtxhost.h").read_text()
        module = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        api = (ROOT / "ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        self.assertIn("ogl_rtx_copy_debug_shadow_mask", header)
        self.assertIn("ogl_rtx_copy_debug_shadow_mask", host)
        self.assertIn('ogl_resolve_module(rtx_module_handle, "ogl_rtx_copy_shadow_debug")', host)
        self.assertIn("capacity < OGL_RTX_SHADOW_DEBUG_PIXELS", host)
        self.assertIn("if (hit_count != debug.occluded) return false;", host)
        self.assertIn("ogl_rtx_copy_shadow_debug(", module)
        self.assertIn("#define OGL_RTX_SHADOW_DEBUG_VERSION 3u", api)
        self.assertIn("uint8_t pixels[OGL_RTX_SHADOW_DEBUG_PIXELS]", api)

    def test_camera_rayquery_path_is_perspective_and_optional(self):
        shader = (ROOT / "ogl/modules/rtx/shaders/camera_sun_shadow.comp").read_text()
        source = (ROOT / "ogl/modules/rtx/vulkan_blas_smoke.cpp").read_text()
        module = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        manifest = (ROOT / "indra/newview/viewer_manifest.py").read_text()
        cmake = (ROOT / "ogl/modules/CMakeLists.txt").read_text()
        self.assertIn("rayQueryInitializeEXT(primary", shader)
        self.assertIn("rayQueryInitializeEXT(sunRay", shader)
        self.assertIn("rayQueryGetIntersectionTEXT(primary", shader)
        self.assertIn("ogl_gpu_probe_camera_run", source)
        self.assertIn("cached_camera == requested_camera", source)
        self.assertIn("OGL_RTX_CAMERA_PROBE", module)
        self.assertIn("view_changed", module)
        self.assertIn("camera_sun_shadow.comp.spv", manifest)
        self.assertIn("ogl_gpu_perspective_sun_shadow", cmake)
        self.assertIn("ogl_live_opt_in_camera_sun_shadow", cmake)
        import struct
        spirv = (ROOT / "indra/newview/app_settings/ogl/rtx/camera_sun_shadow.comp.spv").read_bytes()
        self.assertGreater(len(spirv), 400)
        self.assertEqual(struct.unpack("<I", spirv[:4])[0], 0x07230203)

    def test_real_firestorm_projection_and_rolling_camera(self):
        rt = (ROOT / "indra/newview/pipeline.cpp").read_text()
        host = (ROOT / "indra/newview/oglrtxhost.cpp").read_text()
        api = (ROOT / "ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        gpu = (ROOT / "ogl/modules/rtx/vulkan_blas_smoke.cpp").read_text()
        self.assertIn("const LLVector3 right = -camera->getLeftAxis()", rt)
        self.assertIn("camera->getUpAxis().mV", rt)
        self.assertIn("camera->getView(), camera->getAspect()", rt)
        self.assertIn("frame.camera_right[i] = camera_right[i]", host)
        self.assertIn("#define OGL_RTX_FRAME_VERSION 3u", api)
        self.assertIn("vertical_fov_radians", api)
        self.assertIn("std::tan(camera->vertical_fov_radians * 0.5f)", gpu)
        self.assertIn("cached_camera == requested_camera", gpu)
        self.assertIn("ogl::valid_camera_frame(*camera)", gpu)
        self.assertIn("ogl_camera_frame_validation", (ROOT / "ogl/modules/CMakeLists.txt").read_text())

    def test_open_gl_depth_bridge_only_opt_in_and_state_safe(self):
        source = (ROOT / "indra/newview/oglrtxhost.cpp").read_text()
        pipeline = (ROOT / "indra/newview/pipeline.cpp").read_text()
        rtx = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        contract = (ROOT / "ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        self.assertIn('std::getenv("OGL_RTX_DEPTH_PROBE")', source)
        self.assertIn("now - previous < (dense_enabled ?",source)
        self.assertIn("std::chrono::seconds(60)",source)
        self.assertIn("std::chrono::seconds(2)",source)
        self.assertIn("glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING", source)
        self.assertIn("glBindFramebuffer(GL_READ_FRAMEBUFFER, old_read)", source)
        self.assertIn("glBindBuffer(GL_PIXEL_PACK_BUFFER, old_pack_buffer)", source)
        self.assertIn("glBlitFramebuffer(0, 0, width, height, 0, 0,",source)
        self.assertIn("grid_w,grid_h",source)
        self.assertIn("glReadPixels(0, 0, 8, 8, GL_DEPTH_COMPONENT, GL_FLOAT,", source)
        self.assertIn("glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING", source)
        self.assertIn("glBindFramebuffer(GL_DRAW_FRAMEBUFFER, old_draw)", source)
        self.assertIn("glDisable(GL_SCISSOR_TEST)", source)
        self.assertIn("glBindRenderbuffer(GL_RENDERBUFFER, old_renderbuffer)", source)
        self.assertIn("ogl_rtx_submit_depth_debug", rtx)
        self.assertIn("ogl_rtx_copy_depth_debug", rtx)
        self.assertIn("OGL_RTX_DEPTH_DEBUG_VERSION 2u", contract)
        self.assertIn("grid_width", contract)
        shader = (ROOT / "ogl/modules/rtx/shaders/camera_sun_shadow.comp").read_text()
        self.assertIn("(vec2(pixel) + vec2(0.5)) * (2.0 / 8.0)", shader)
        self.assertIn("mRT->deferredScreen.getDepth()", pipeline)
        self.assertNotIn("glReadPixels(", pipeline)

    def test_depth_guided_rayquery_is_bounded_and_opt_in(self):
        gpu = (ROOT / "ogl/modules/rtx/vulkan_blas_smoke.cpp").read_text()
        shader = (ROOT / "ogl/modules/rtx/shaders/depth_guided_sun.comp").read_text()
        module = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        cmake = (ROOT / "ogl/modules/CMakeLists.txt").read_text()
        manifest = (ROOT / "indra/newview/viewer_manifest.py").read_text()
        camera = (ROOT / "ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        self.assertIn("ogl_gpu_probe_depth_run", gpu)
        self.assertIn("VK_DESCRIPTOR_TYPE_STORAGE_BUFFER", gpu)
        self.assertIn("cached_depth.size()==grid_pixels", gpu)
        self.assertIn("std::equal(depth_64,depth_64+grid_pixels",gpu)
        self.assertIn("requested_camera[16] = camera->near_clip_m", gpu)
        self.assertIn("requested_camera[17] = camera->far_clip_m", gpu)
        self.assertIn("layout(set=0,binding=2,std430) readonly buffer DeferredDepth", shader)
        self.assertIn("const vec3 worldSurface=", shader)
        self.assertIn("rayQueryInitializeEXT(shadow", shader)
        self.assertIn("OGL_RTX_DEPTH_GUIDED_PROBE", module)
        self.assertIn("OGL_RTX_DEPTH_PROBE", module)
        self.assertIn("!probe_depth_enabled || depth_debug_valid", module)
        self.assertIn("ogl_live_depth_guided_gpu_bridge", cmake)
        self.assertIn("ogl_gpu_depth_guided_sun_shadow", cmake)
        self.assertIn("depth_guided_sun.comp.spv", manifest)
        self.assertIn("near_clip_m", camera)
        self.assertIn("far_clip_m", camera)

    def test_rtx_debug_preview_is_separate_and_opt_in(self):
        ui = (ROOT / "indra/newview/llviewerdisplay.cpp").read_text()
        host = (ROOT / "indra/newview/oglrtxhost.cpp").read_text()
        api = (ROOT / "ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        module = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        self.assertIn('std::getenv("OGL_RTX_DEBUG_OVERLAY")', ui)
        self.assertIn('std::getenv("OGL_RTX_DEPTH_GUIDED_PROBE")', ui)
        self.assertIn('!isAgentAvatarValid()', ui)
        self.assertIn('ogl_rtx_copy_debug_shadow_sample(', ui)
        self.assertIn("sample_sequence == 0", ui)
        self.assertIn("now - sample_received > std::chrono::seconds(8)", ui)
        self.assertIn('"NOT WORLD SHADOWS"', ui)
        self.assertIn("LLViewerFetchedTexture::sWhiteImagep", ui)
        self.assertIn("ogl_draw_rtx_shadow_debug_preview();", ui)
        self.assertIn('ogl_resolve_module(rtx_module_handle, "ogl_rtx_received_triangles")', host)
        self.assertIn("rtx_received_triangles() == 0", host)
        self.assertIn("bool ogl_rtx_copy_debug_shadow_mask(", host)
        self.assertIn("bool ogl_rtx_copy_debug_shadow_sample(", host)
        self.assertIn("#define OGL_RTX_SHADOW_DEBUG_VERSION 3u", api)
        self.assertIn("uint64_t sample_sequence", api)
        self.assertIn("fresh.sample_sequence = outcome.revision;", module)
        self.assertIn("fresh.surface_pixels = outcome.surface_pixels", module)
        self.assertIn("fresh.background_pixels = outcome.background_pixels", module)
        self.assertIn("fresh.surface_mask", module)
        self.assertIn("surface_mask.data()", ui)
        self.assertIn("surface_count", ui)
        self.assertIn("background_count", ui)
        self.assertIn("Grey = sky/background", ui)
        self.assertIn("rendering_active=0", module)

    def test_live_rtx_depth_snapshot_survives_rotating_scene_scan(self):
        pipeline = (ROOT / "indra/newview/pipeline.cpp").read_text()
        plugin = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        test = (ROOT / "ogl/modules/rtx/ogl_live_gpu_probe_test.cpp").read_text()
        self.assertIn("ogl_rtx_last_region_id", pipeline)
        self.assertIn("ogl_rtx_region->getRegionID()", pipeline)
        self.assertIn("if (!ogl_rtx_last_region_id.isNull())", pipeline)
        self.assertNotIn("else\n                    ogl_rtx_submit_world_triangles(nullptr, 0);",pipeline)
        self.assertIn("(!probe_depth_enabled && outcome.geometry_revision != geometry_revision)",plugin)
        self.assertIn("if (!probe_depth_enabled) {",plugin)
        self.assertIn('OGL RTX depth-guided GPU snapshot queued',plugin)
        self.assertIn("assert(debug.geometry_revision==1); // captured before mesh scan", test)
        self.assertIn("assert(debug.geometry_revision==2);", test)
        self.assertIn("assert(submit(nullptr,0)==1)", test)

    def test_large_opt_in_world_occluder_capture(self):
        source = (ROOT / "indra/newview/pipeline.cpp").read_text()
        api = (ROOT / "ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        cmake = (ROOT / "ogl/modules/CMakeLists.txt").read_text()
        terrain = (ROOT / "ogl/modules/include/ogl/ogl_terrain_patch.h").read_text()
        self.assertIn("OGL_RTX_MAX_SCENE_TRIANGLES 8192u", api)
        self.assertIn('std::getenv("OGL_RTX_SCENE_PROBE")', source)
        self.assertIn("ogl::append_nearby_terrain_triangles(", source)
        self.assertIn("ogl_rtx_region->getLandHeightRegion(", source)
        self.assertIn("face_triangle_budget", source)
        self.assertIn("ogl_vulkan_module_scene_8192", cmake)
        self.assertIn("ogl_vulkan_module_scene_4096", cmake)
        self.assertIn("ogl_terrain_patch_geometry", cmake)
        self.assertIn("radius_cells = 10", terrain)
        self.assertIn("out.size() + 2 > triangle_budget", terrain)

    def test_depth_coverage_and_gl_projection_are_tested_portably(self):
        diag = (ROOT / "ogl/modules/include/ogl/ogl_depth_diagnostics.h").read_text()
        module = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        cmake = (ROOT / "ogl/modules/CMakeLists.txt").read_text()
        test = (ROOT / "ogl/tests/test_depth_diagnostics.cpp").read_text()
        self.assertIn("gl_depth_to_view_distance", diag)
        self.assertIn("summarize_depth_grid", diag)
        self.assertIn("fresh.background_pixels", diag)
        self.assertIn("OGL RTX depth coverage: surface=", module)
        self.assertIn("surface_samples=", module)
        self.assertIn("background_samples=", module)
        queue = (ROOT / "ogl/modules/include/ogl/ogl_async_gpu_queue.h").read_text()
        self.assertIn("uint32_t surface_pixels", queue)
        self.assertIn("uint32_t background_pixels", queue)
        self.assertIn("ogl_depth_reconstruction_and_coverage", cmake)
        self.assertIn("Perspective GL z encoding", test)

    def test_primary_rays_agree_with_opengl_depth_in_vulkan(self):
        shader=(ROOT / "ogl/modules/rtx/shaders/depth_guided_sun.comp").read_text()
        gpu=(ROOT / "ogl/modules/rtx/vulkan_blas_smoke.cpp").read_text()
        module=(ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        self.assertIn("uint primaryHit[64]",shader)
        self.assertIn("uint depthMatch[64]",shader)
        self.assertIn("rayQueryGetIntersectionTEXT(primary,true)",shader)
        self.assertIn("const uint32_t output_sections = depth_64 ? 4u : 1u",gpu)
        self.assertIn("primary_hits=",module)
        self.assertIn("depth_matches=",module)
        self.assertIn("primary_closer=",module)
        self.assertIn("primary_farther=",module)
        self.assertIn("primaryDepthRelation[64]",shader)

    def test_vulkan_as_inplace_refit_and_tlas_instance_transform(self):
        gpu=(ROOT / "ogl/modules/rtx/vulkan_blas_smoke.cpp").read_text()
        api=(ROOT / "ogl/modules/include/ogl/ogl_gpu_probe_api.h").read_text()
        cmake=(ROOT / "ogl/modules/CMakeLists.txt").read_text()
        self.assertIn("VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR",gpu)
        self.assertIn("VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR",gpu)
        self.assertIn("update.srcAccelerationStructure=c.tlas",gpu)
        self.assertIn("update_info.srcAccelerationStructure = c.blas",gpu)
        self.assertIn("ogl_gpu_probe_transform_run(",gpu)
        self.assertIn("tlas_refits",api)
        self.assertIn("blas_refits",api)
        self.assertIn("ogl_vulkan_tlas_instance_refit",cmake)

    def test_dynamic_object_identity_pose_and_vulkan_refit(self):
        cache=(ROOT / "ogl/modules/include/ogl/ogl_dynamic_scene.h").read_text()
        gpu=(ROOT / "ogl/modules/rtx/vulkan_blas_smoke.cpp").read_text()
        cmake=(ROOT / "ogl/modules/CMakeLists.txt").read_text()
        self.assertIn("using ObjectKey = std::array<uint8_t, 16>",cache)
        self.assertIn("SceneChange update_transform",cache)
        self.assertIn("m_geometry_generation",cache)
        self.assertIn("m_transform_generation",cache)
        self.assertIn("VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR",gpu)
        self.assertIn("ogl_dynamic_scene_updates",cmake)
        self.assertIn("ogl_live_uuid_object_registry",cmake)
        self.assertIn("ogl_rtx_track_local_object",(
            ROOT / "indra/newview/pipeline.cpp").read_text())
        self.assertIn("ogl_rtx_update_object_pose",(
            ROOT / "indra/newview/pipeline.cpp").read_text())
        self.assertIn("ogl_vulkan_tlas_instance_refit",cmake)

    def test_foreground_priority_and_stratified_mesh_sampling(self):
        src=(ROOT / "indra/newview/pipeline.cpp").read_text()
        helper=(ROOT / "ogl/modules/include/ogl/ogl_scene_sampling.h").read_text()
        cmake=(ROOT / "ogl/modules/CMakeLists.txt").read_text()
        self.assertIn("ogl::scene_priority(distance,cosine,extent)",src)
        self.assertIn("std::stable_sort(candidates.begin()",src)
        self.assertIn("ogl::spread_triangle_index(",src)
        self.assertIn("object_triangles < max_per_object",src)
        self.assertIn("128u : 512u",src)
        self.assertIn("ogl_scene_priority_and_mesh_sampling",cmake)
        self.assertIn("uint64_t(2) * sample_index",helper)

    def test_no_rt_hardware_preserves_opengl_fallback(self):
        module = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        self.assertIn('No suitable Vulkan ray tracing GPU found; OpenGL fallback.', module)
        host = (ROOT / "indra/newview/oglrtxhost.cpp").read_text()
        self.assertIn('OpenGL remains the active renderer', host)


if __name__ == "__main__":
    unittest.main()
