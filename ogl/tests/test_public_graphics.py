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
        self.assertIn('label="Vulkan (P05 - not available yet)"',panel)

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
        self.assertNotIn("ogl_vulkan_world_present_exclusive_test_frame",
                         (ROOT/"indra/newview/oglrtxhost.cpp").read_text())
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
        self.assertNotIn("ogl_vulkan_world_present_exclusive_test_frame",
                         (ROOT/"indra/newview/oglrtxhost.cpp").read_text())
        self.assertIn("gl_released!=1",module)
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
        self.assertNotIn("ogl_vulkan_world_present_exclusive_test_frame",
                         (ROOT/"indra/newview/oglrtxhost.cpp").read_text())
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
        self.assertNotIn("ogl_vulkan_world_present_exclusive_test_frame",
                         (ROOT/"indra/newview/oglrtxhost.cpp").read_text())
        self.assertIn("gl_released!=1",module)

    def test_p05_offscreen_vulkan_gpu_frame_is_verified_not_presented(self):
        header=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        self.assertIn("ogl_vulkan_world_render_offscreen_probe",header)
        self.assertIn("ogl_vulkan_world_render_offscreen_probe",module)
        self.assertIn('std::getenv("OGL_VULKAN_OFFSCREEN_FRAME_PROBE")',module)
        self.assertIn('std::getenv("OGL_VULKAN_OFFSCREEN_FRAME_PROBE")',host)
        for fn in ("vkCreateImage","vkCreateRenderPass",
                   "vkCreateFramebuffer","vkCmdBeginRenderPass",
                   "vkCmdEndRenderPass","vkCmdCopyImageToBuffer",
                   "vkCmdPipelineBarrier","vkQueueSubmit","vkWaitForFences",
                   "vkMapMemory","vkUnmapMemory"):
            self.assertIn("OGL_OFFSCREEN_FN("+fn+")",module)
        self.assertIn("VK_FORMAT_R8G8B8A8_UNORM",module)
        self.assertIn("VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT",module)
        self.assertIn("VK_ACCESS_TRANSFER_READ_BIT",module)
        self.assertIn("VK_ACCESS_TRANSFER_WRITE_BIT",module)
        self.assertIn("VK_ACCESS_HOST_READ_BIT",module)
        self.assertIn("bool correct=matches(0)",module)
        self.assertIn("return correct ? 1u : 0u;",module)
        self.assertIn('unsetenv("OGL_VULKAN_OFFSCREEN_FRAME_PROBE")',native)
        self.assertIn("assert(offscreen_frame()==0);",native)
        self.assertIn("if (actual_frame) assert(offscreen_frame()==1);",native)
        self.assertIn('getenv("OGL_REQUIRE_VULKAN_OFFSCREEN_FRAME")',native)
        self.assertIn("(ready!=1 || actual_frame!=1)",native)
        self.assertIn("return 11;",native)
        self.assertIn("P05 real Vulkan offscreen GPU frame:",host)
        self.assertIn("no swapchain present, OpenGL unchanged.",host)
        self.assertNotIn("ogl_vulkan_world_present_exclusive_test_frame",
                         (ROOT/"indra/newview/oglrtxhost.cpp").read_text())
        self.assertIn("gl_released!=1",module)

    def test_p05_native_present_is_exclusive_x11_only_not_live_gl(self):
        header=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        cmake=(ROOT/"ogl/modules/CMakeLists.txt").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        self.assertIn("ogl_vulkan_world_present_exclusive_test_frame",header)
        self.assertIn("present_exclusive_native_test_frame(gl_released)",module)
        self.assertIn('std::getenv("OGL_VULKAN_EXCLUSIVE_PRESENT_PROBE")',module)
        self.assertIn("gl_released!=1",module)
        self.assertIn("world_device.exclusive_present_used",module)
        self.assertIn("world_device.swapchain_format=format.format;",module)
        self.assertIn("world_device.swapchain_extent=extent;",module)
        for item in ("vkAcquireNextImageKHR","vkQueuePresentKHR",
                     "vkCreateImageView","vkCreateRenderPass",
                     "vkCmdBeginRenderPass","vkCmdEndRenderPass",
                     "vkCreateSemaphore","vkWaitForFences",
                     "vkQueueWaitIdle"):
            self.assertIn("OGL_PRESENT_FN("+item+")",module)
        self.assertIn("VK_IMAGE_LAYOUT_PRESENT_SRC_KHR",module)
        self.assertIn("VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT",module)
        self.assertIn("OGL_VULKAN_PRESENT_TEST_X11",native)
        self.assertIn("XCreateSimpleWindow",native)
        self.assertIn("const uint32_t without_owner=exclusive_present(0)",native)
        self.assertIn("const uint32_t without_flag=exclusive_present(1)",native)
        self.assertIn("const uint32_t presented=exclusive_present(1)",native)
        self.assertIn("const uint32_t duplicate=exclusive_present(1)",native)
        self.assertIn("XResizeWindow(display,window,480,300)",native)
        self.assertIn("resized_presented=exclusive_present(1)",native)
        self.assertIn("resized_duplicate=exclusive_present(1)",native)
        self.assertIn("Native X11 present QA unavailable in this build",native)
        self.assertIn('CMAKE_SYSTEM_NAME STREQUAL "Linux"',cmake)
        self.assertNotIn("ogl_vulkan_world_present_exclusive_test_frame",host)
        self.assertIn('renderer->setEnabledByValue("vulkan", false)',
                      (ROOT/"indra/newview/llfloaterpreference.cpp").read_text())

    def test_p05_actual_sdl_window_pre_gl_vulkan_handoff_is_safe_and_opt_in(self):
        # This is a single developer-only Vulkan WSI present on the *actual*
        # SDL viewer window BEFORE it has an OpenGL context. GL is restored
        # normally, and this is not the full in-world Vulkan world backend.
        win=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        header=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        for token in ("OGL_VULKAN_SDL_PREGL_PRESENT_PROBE",
                      "OGL_VULKAN_WORLD_BOOTSTRAP",
                      "OGL_VULKAN_WINDOW_SURFACE",
                      "OGL_VULKAN_EXCLUSIVE_PRESENT_PROBE"):
            self.assertIn('ogl_pre_gl_flag("'+token+'")',win)
        self.assertIn("APP_NAME != \"OGL\"",win)
        self.assertIn("SDL_GL_GetCurrentContext()!=nullptr",win)
        self.assertIn("SDL_GetWindowWMInfo(window,&wm)",win)
        self.assertIn("wm.subsystem!=SDL_SYSWM_X11",win)
        self.assertIn("SDL_LoadObject(\"./lib/ogl_rtx.so\")",win)
        self.assertIn("SDL_LoadFunction(library,\"ogl_vulkan_world_present_exclusive_test_frame\")",win)
        self.assertIn("if (!good || selected_present(1)!=1 ||",win)
        self.assertIn("detach();\n            stop();",win)
        self.assertIn("SDL_UnloadObject(library);",win)
        self.assertIn("ogl_probe_real_sdl_window_before_gl(mWindow);",win)
        self.assertLess(win.index("ogl_probe_real_sdl_window_before_gl(mWindow);"),
                        win.index("mContext = SDL_GL_CreateContext( mWindow );"))
        self.assertIn("OpenGL follows.",win)
        self.assertIn("XGetImage(wm.info.x11.display,",win)
        self.assertIn("XGetPixel(image,0,0)",win)
        self.assertIn("std::abs(red-wanted_red)>5",win)
        self.assertIn('pixel_verified ? "verified" : "unverified"',win)
        self.assertIn("XDestroyImage(image);",win)
        self.assertLess(win.index("XDestroyImage(image);"),
                        win.index("SDL_GL_CreateContext( mWindow );"))
        self.assertIn("ogl_vulkan_world_present_exclusive_test_frame",header)
        self.assertIn("gl_released!=1",module)
        self.assertIn('renderer->setEnabledByValue("vulkan", false)',
            (ROOT/"indra/newview/llfloaterpreference.cpp").read_text())
        self.assertNotIn("OGL_VULKAN_SDL_PREGL_PRESENT_PROBE",
            (ROOT/"indra/newview/oglrtxhost.cpp").read_text())

    def test_p05_sdl_pre_gl_all_permission_pairs_are_checked_before_vulkan(self):
        win=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        begin=win.index("void ogl_probe_real_sdl_window_before_gl(")
        end=win.index("} // namespace",begin)
        fn=win[begin:end]
        names=(
            "OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE",
            "OGL_VULKAN_SDL_PREGL_DEPTH_PROBE",
            "OGL_VULKAN_EXCLUSIVE_PERSISTENT_CLEAR_PROBE",
            "OGL_VULKAN_SDL_PREGL_PERSISTENT_CLEAR_PROBE",
            "OGL_VULKAN_EXCLUSIVE_PERSISTENT_WORLD_PROBE",
            "OGL_VULKAN_SDL_PREGL_PERSISTENT_WORLD_PROBE",
            "OGL_VULKAN_PERSISTENT_MESH_ROTATION_PROBE",
            "OGL_VULKAN_SDL_PREGL_MESH_ROTATION_PROBE",
            "OGL_VULKAN_LIVE_REGION_BARRIER",
            "OGL_VULKAN_SDL_PREGL_REGION_BARRIER_PROBE")
        for flag in names:
            self.assertIn('"'+flag+'"',fn)
        self.assertIn("const char* paired_probe_flags[][2]",fn)
        self.assertIn("ogl_pre_gl_flag(pair[0])!=ogl_pre_gl_flag(pair[1])",fn)
        self.assertIn("P05 SDL pre-GL developer permission pair incomplete; OpenGL follows.",fn)
        self.assertIn('!ogl_pre_gl_flag("OGL_VULKAN_FRAME_SYNC_PROBE")',fn)
        self.assertIn("P05 SDL pre-GL dual-sync permission incomplete; OpenGL follows.",fn)
        self.assertLess(fn.index("for (const auto& pair : paired_probe_flags)"),
                        fn.index('SDL_LoadObject("./lib/ogl_rtx.so")'))
        self.assertLess(fn.index("P05 SDL pre-GL dual-sync permission incomplete"),
                        fn.index("SDL_GetWindowWMInfo(window,&wm)"))

    def test_p05_sdl_pre_gl_sequence_requires_matching_optin_flags(self):
        win=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        begin=win.index("void ogl_probe_real_sdl_window_before_gl(")
        end=win.index("} // namespace",begin)
        fn=win[begin:end]
        for marker in (
            'const bool sequence_requested=',
            'ogl_pre_gl_flag("OGL_VULKAN_SDL_PREGL_SEQUENCE_PROBE")',
            'const bool sequence_permitted=',
            'ogl_pre_gl_flag("OGL_VULKAN_EXCLUSIVE_FRAME_LOOP")',
            'if (sequence_requested!=sequence_permitted)',
            'P05 SDL pre-GL sequence permission incomplete; OpenGL follows.',
            'const bool sequence=sequence_requested && sequence_permitted;'):
            self.assertIn(marker,fn)
        self.assertLess(fn.index('if (sequence_requested!=sequence_permitted)'),
                        fn.index('SDL_GetWindowWMInfo(window,&wm)'))
        self.assertLess(fn.index('if (sequence_requested!=sequence_permitted)'),
                        fn.index('SDL_LoadObject("./lib/ogl_rtx.so")'))
        self.assertLess(fn.index('const bool sequence=sequence_requested'),
                        fn.index('if (sequence && count)'))

    def test_p05_actual_sdl_three_extent_multiframe_probe_contract(self):
        win=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        self.assertIn('const bool sequence=sequence_requested && sequence_permitted;',win)
        self.assertIn('SDL_LoadFunction(library,"ogl_vulkan_world_exclusive_present_count")',win)
        self.assertIn('original_width,original_height,8',win)
        self.assertIn('frame_group(actual_width,actual_height,8)',win)
        self.assertIn('restored_width,restored_height,4',win)
        self.assertIn('SDL_SetWindowSize(window,target_width,target_height);',win)
        self.assertIn('SDL_SetWindowSize(window,original_width,original_height);',win)
        self.assertIn('sequence_verified && presented==20',win)
        self.assertIn('(sequence ? "/20" : "/1")',win)
        self.assertIn('if (count && count()!=0) good=false;',win)
        self.assertIn('pixel_verified=(!raster || uploaded) &&',win)
        self.assertIn('projected && frame_group(',win)
        self.assertIn('mContext = SDL_GL_CreateContext( mWindow );',win)

    def test_p05_sdl_pre_gl_five_pixels_geometry_and_fault_recovery(self):
        win=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        self.assertIn('static bool startup_probe_attempted=false;',win)
        self.assertIn('if (startup_probe_attempted)',win)
        self.assertIn('SDL_GL_GetCurrentContext()!=nullptr',win)
        self.assertIn('OGL_VULKAN_SDL_PREGL_FAULT_STAGE',win)
        self.assertIn('std::strcmp(value,stage)==0',win)
        self.assertIn('group==1 ? "initial"',win)
        self.assertIn('group==2 ? "resized"',win)
        self.assertIn('"restored"',win)
        self.assertIn('if (frame==3 &&',win)
        self.assertIn('P05 frame stage fault injected:',win)
        self.assertIn('const int coords[5][2]',win)
        self.assertIn('++verified_pixels;',win)
        self.assertIn('verified_pixels==100',win)
        self.assertIn('XGetGeometry(wm.info.x11.display,wm.info.x11.window,',win)
        self.assertIn('x11_extent_matches(original_width,original_height)',win)
        self.assertIn('SDL_SetWindowSize(window,original_width,original_height);',win)
        self.assertIn('detach();\n            stop();',win)
        self.assertIn('mContext = SDL_GL_CreateContext( mWindow );',win)

    def test_p05_indexed_vulkan_graphics_pipeline_uses_validated_spirv(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        header=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        self.assertIn("ogl_dev_vert_spv[]",module)
        self.assertIn("ogl_dev_frag_spv[]",module)
        self.assertIn('OGL_VULKAN_EXCLUSIVE_RASTER_PROBE',module)
        self.assertIn('world_device.mesh_triangle_count==0',module)
        self.assertIn('mesh_triangle_count>OGL_RTX_MAX_SCENE_TRIANGLES',module)
        self.assertIn('OGL_PRESENT_FN(vkCreateGraphicsPipelines)',module)
        self.assertIn('OGL_PRESENT_FN(vkCmdBindVertexBuffers)',module)
        self.assertIn('OGL_PRESENT_FN(vkCmdBindIndexBuffer)',module)
        self.assertIn('OGL_PRESENT_FN(vkCmdDrawIndexed)',module)
        self.assertIn('VK_FORMAT_R32G32B32_SFLOAT',module)
        self.assertIn('VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST',module)
        self.assertIn('graphics_info.renderPass=pass;',module)
        self.assertIn('vkCreateGraphicsPipelines(world_device.device',module)
        self.assertIn('vkCmdDrawIndexed(cmd,world_device.mesh_triangle_count*3u',module)
        self.assertIn('if (graphics_pipeline) vkDestroyPipeline(',module)
        self.assertIn('if (vertex_shader) vkDestroyShaderModule(',module)
        self.assertIn('OGL_VULKAN_RASTER_TEST_X11',native)
        self.assertIn('P05 Vulkan indexed raster X11 pixel:',native)
        self.assertIn('raster_pixels=center && background',native)
        self.assertIn('raster_cleared=world_upload(nullptr,0)==1',native)
        self.assertIn('raster_without_mesh=exclusive_present(1)',native)
        self.assertIn('raster_without_mesh!=0',native)
        self.assertIn('OGL_VULKAN_EXCLUSIVE_RASTER_PROBE',header)
        self.assertIn('OpenSim agent coordinates still require a camera/world matrix',header)
        self.assertIn('ogl_vulkan_world_upload_triangles',header)

    def test_p05_actual_sdl_triangle_raster_is_strict_pre_gl_opt_in(self):
        win=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        for name in ('OGL_VULKAN_SDL_PREGL_GEOMETRY_PROBE',
                     'OGL_VULKAN_EXCLUSIVE_RASTER_PROBE',
                     'OGL_VULKAN_WORLD_MESH_UPLOAD'):
            self.assertIn('ogl_pre_gl_flag("'+name+'")',win)
        self.assertIn('(!sequence || persistent_world_both) &&',win)
        self.assertIn('upload && mesh_count;',win)
        self.assertIn('SDL_LoadFunction(library,"ogl_vulkan_world_upload_triangles")',win)
        self.assertIn('SDL_LoadFunction(library,"ogl_vulkan_world_gpu_triangle_count")',win)
        self.assertIn('const ogl_rtx_triangle clip_triangle',win)
        self.assertIn('mesh_count()==triangles',win)
        self.assertIn('const bool triangle=raster && sample==0',win)
        self.assertIn('std::abs(red-wanted_red)>5',win)
        self.assertIn('raster_verified=raster && uploaded && pixel_verified;',win)
        self.assertIn('if (raster && upload) upload(nullptr,0);',win)
        self.assertIn('raster_verified ? "verified" : "failed"',win)
        self.assertLess(win.index('if (raster && upload) upload(nullptr,0);'),
                        win.index('SDL_GL_CreateContext( mWindow );'))
        self.assertNotIn('OGL_VULKAN_EXCLUSIVE_RASTER_PROBE',
                         (ROOT/"indra/newview/oglrtxhost.cpp").read_text())

    def test_p05_vulkan_world_camera_projection_and_push_constant_contract(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        abi=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        win=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        self.assertIn('ogl_dev_world_vert_spv[]',module)
        self.assertIn('valid_camera_frame(*frame)',module)
        self.assertIn('OGL_VULKAN_WORLD_CAMERA_PROBE',module)
        self.assertIn('OGL_VULKAN_EXCLUSIVE_WORLD_PROBE',module)
        self.assertIn('world_camera_ready=false;',module)
        self.assertIn('std::fabs(actual_aspect-frame->aspect_ratio)>0.15f',module)
        self.assertIn('std::fabs(norm2-1.f)>0.05f',module)
        self.assertIn('handedness<0.9 || handedness>1.1',module)
        self.assertIn('const double fy=1.0/std::tan(',module)
        self.assertIn('matrix[axis*4+0]=float(double(right[axis])*fx);',module)
        self.assertIn('matrix[axis*4+1]=float(-double(up[axis])*fy);',module)
        self.assertIn('matrix[axis*4+2]=float(double(forward[axis])*depth);',module)
        self.assertIn('matrix[12]=float(-right_eye*fx);',module)
        self.assertIn('matrix[14]=float(-(forward_eye+double(frame->near_clip_m))*depth);',module)
        self.assertIn('world_device.world_to_clip=matrix;',module)
        self.assertIn('OGL_PRESENT_FN(vkCmdPushConstants)',module)
        self.assertIn('camera_range.size=sizeof(float)*16;',module)
        self.assertIn('vkCmdPushConstants(cmd,pipeline_layout,',module)
        self.assertIn('world_device.world_to_clip.data()',module)
        self.assertIn('ogl_vulkan_world_set_projection_camera(',abi)
        self.assertIn('ogl_vulkan_world_projection_camera_ready(void)',abi)
        self.assertIn('world_triangle',native)
        self.assertIn('OGL_VULKAN_WORLD_PROJECTION_TEST_X11',native)
        self.assertIn('world_positive=refused_bad_axis',native)
        self.assertIn('world_shifted=moved_draw && pixel_is(256,192,20,54,115)',native)
        self.assertIn('OGL_VULKAN_SDL_PREGL_WORLD_PROJECTION_PROBE',win)
        self.assertIn('set_camera(&camera)==1',win)
        self.assertIn('camera_ready()==1',win)
        self.assertIn('world_verified=world && projected && pixel_verified;',win)
        self.assertLess(win.index('world_verified=world && projected && pixel_verified;'),
                        win.index('SDL_GL_CreateContext( mWindow );'))
        self.assertNotIn('OGL_VULKAN_EXCLUSIVE_WORLD_PROBE',
                         (ROOT/"indra/newview/oglrtxhost.cpp").read_text())

    def test_p05_real_firestorm_camera_and_mesh_staging_pair_without_rtx(self):
        host=(ROOT/'indra/newview/oglrtxhost.cpp').read_text()
        header=(ROOT/'ogl/modules/include/ogl/ogl_rtx_api.h').read_text()
        module=(ROOT/'ogl/modules/rtx/ogl_rtx_module.cpp').read_text()
        native=(ROOT/'ogl/modules/rtx/frame_bridge_test.cpp').read_text()
        pipeline=(ROOT/'indra/newview/pipeline.cpp').read_text()
        for name in ('OGL_VULKAN_WORLD_BOOTSTRAP',
                     'OGL_VULKAN_WORLD_MESH_UPLOAD',
                     'OGL_VULKAN_WORLD_CAMERA_PROBE'):
            self.assertIn('std::getenv("'+name+'")',host)
        self.assertIn('ogl_vulkan_world_set_projection_camera',host)
        self.assertIn('ogl_vulkan_world_live_inputs_ready',host)
        self.assertIn('world_set_camera(&frame)==1',host)
        self.assertIn('world_device_ready()==1',host)
        self.assertIn('const bool rt_camera=rtx_submit_frame && rtx_hardware_ready;',host)
        self.assertIn('if ((!gpu_camera && !rt_camera)',host)
        self.assertIn('if (rt_camera) rtx_submit_frame(&frame);',host)
        self.assertIn('if (uploaded && count>0)',host)
        self.assertIn('if (count==0) {\n            world_inputs_live_logged=false;',host)
        self.assertIn('P05 live Firestorm camera accepted by Vulkan world-matrix ABI',host)
        self.assertIn('actual captured OpenSim mesh',host)
        self.assertLess(host.index('world_set_camera(&frame)==1'),
                        host.index('if (rt_camera) rtx_submit_frame(&frame);'))
        self.assertIn('camera->getOrigin().mV, camera->getAtAxis().mV',pipeline)
        self.assertIn('const LLVector3 right = -camera->getLeftAxis();',pipeline)
        self.assertIn('ogl_rtx_submit_camera_sun(viewport.getWidth()',pipeline)
        self.assertIn('ogl_rtx_submit_deferred_world_triangles(\n                        triangles.front().position',pipeline)
        self.assertIn('ogl_rtx_submit_world_triangles(nullptr, 0);',pipeline)
        self.assertIn('ogl_vulkan_world_live_inputs_ready(void)',header)
        self.assertIn('ogl_vulkan_world_camera_updates(void)',header)
        self.assertIn('ogl_vulkan_world_mesh_uploads(void)',header)
        self.assertIn('world_device.world_camera_accepts',module)
        self.assertIn('world_device.mesh_uploads',module)
        self.assertIn('world_device.world_to_clip.fill(0.f);',module)
        self.assertIn('world_device.mesh_triangle_count>0',module)
        self.assertIn('return world_device.world_camera_ready &&',module)
        self.assertIn('live_inputs()==1',native)
        self.assertIn('mesh_uploads()==3 && camera_updates()==3',native)
        self.assertIn('invalid input clears camera',native)
        self.assertIn('new scene invalidates pose',native)
        self.assertIn('region-clear frees GPU buffers',native)
        self.assertNotIn('vkQueuePresentKHR(',host)

    def test_p05_vulkan_depth_buffer_real_occlusion_and_sdl_fallback(self):
        module=(ROOT/'ogl/modules/rtx/ogl_rtx_module.cpp').read_text()
        native=(ROOT/'ogl/modules/rtx/frame_bridge_test.cpp').read_text()
        win=(ROOT/'indra/llwindow/llwindowsdl2.cpp').read_text()
        self.assertIn('ogl_dev_depth_frag_spv[]',module)
        self.assertIn('OGL_VULKAN_EXCLUSIVE_DEPTH_PROBE',module)
        self.assertIn('if ((depth_test || depth_color) && !raster) return 0;',module)
        self.assertIn('VK_FORMAT_D32_SFLOAT',module)
        self.assertIn('VK_FORMAT_D16_UNORM',module)
        self.assertIn('VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT',module)
        self.assertIn('VK_IMAGE_ASPECT_DEPTH_BIT',module)
        self.assertIn('VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT',module)
        self.assertIn('depth_attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;',module)
        self.assertIn('VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL',module)
        self.assertIn('subpass.pDepthStencilAttachment=depth_test ? &depth_ref : nullptr;',module)
        self.assertIn('fbi.attachmentCount=depth_test ? 2u : 1u;',module)
        self.assertIn('depth_state.depthTestEnable=VK_TRUE;',module)
        self.assertIn('depth_state.depthWriteEnable=VK_TRUE;',module)
        self.assertIn('depth_state.depthCompareOp=VK_COMPARE_OP_LESS;',module)
        self.assertIn('clears[1].depthStencil.depth=1.f;',module)
        self.assertIn('if (depth_image) vkDestroyImage(',module)
        self.assertIn('if (depth_memory) vkFreeMemory(',module)
        self.assertIn('OGL_VULKAN_DEPTH_TEST_X11',native)
        self.assertIn('const ogl_rtx_triangle triangles[2]{near_triangle,far_triangle};',native)
        self.assertIn('depth_sample(256,192,237,48,38)',native)
        self.assertIn('depth_sample(320,200,237,48,38)',native)
        self.assertIn('depth_cleared=world_upload(nullptr,0)==1',native)
        self.assertIn('OGL_VULKAN_SDL_PREGL_DEPTH_PROBE',win)
        self.assertIn('(depth_requested && !depth)',win)
        self.assertIn('const uint32_t triangles=depth ? 2u : 1u;',win)
        self.assertIn('depth_verified=depth && uploaded && pixel_verified;',win)
        self.assertIn('depth_verified ? "verified" : "failed"',win)
        self.assertLess(win.index('depth_verified=depth && uploaded && pixel_verified;'),
                        win.index('SDL_GL_CreateContext( mWindow );'))
        self.assertNotIn('OGL_VULKAN_SDL_PREGL_DEPTH_PROBE',
                         (ROOT/'indra/newview/oglrtxhost.cpp').read_text())

    def test_p05_projected_world_depth_and_negative_gpu_control(self):
        module=(ROOT/'ogl/modules/rtx/ogl_rtx_module.cpp').read_text()
        native=(ROOT/'ogl/modules/rtx/frame_bridge_test.cpp').read_text()
        win=(ROOT/'indra/llwindow/llwindowsdl2.cpp').read_text()
        header=(ROOT/'ogl/modules/include/ogl/ogl_rtx_api.h').read_text()
        self.assertIn('OGL_VULKAN_EXCLUSIVE_DEPTH_COLOR_PROBE',module)
        self.assertIn('if ((depth_test || depth_color) && !raster) return 0;',module)
        self.assertIn('if (depth_test && depth_color) return 0;',module)
        self.assertIn('fragment_info.codeSize=(depth_test || depth_color)',module)
        self.assertIn('fragment_info.pCode=(depth_test || depth_color)',module)
        self.assertIn('OGL_VULKAN_EXCLUSIVE_DEPTH_COLOR_PROBE=1',header)
        self.assertIn('OGL_VULKAN_WORLD_DEPTH_TEST_X11',native)
        self.assertIn('const ogl_rtx_triangle paired[2]',native)
        self.assertIn('12.5f,18.f,28.4f',native)
        self.assertIn('15.f,16.f,26.8f',native)
        self.assertIn('projection.near_clip_m=2.f;',native)
        self.assertIn('projection.far_clip_m=10.f;',native)
        self.assertIn('world_depth_near=mesh_ok',native)
        self.assertIn('read_rgb(256,192,237,48,38)',native)
        self.assertIn('read_rgb(256,192,20,199,41)',native)
        self.assertIn('world_depth_moved=moved_camera',native)
        self.assertIn('read_rgb(256,192,20,54,115)',native)
        self.assertIn('world_depth_resized=resized_camera',native)
        self.assertIn('read_rgb(320,240,237,48,38)',native)
        self.assertIn('world_depth_invalid=world_camera(&projection)==0',native)
        self.assertIn('world_depth_cleared=world_upload(nullptr,0)==1',native)
        self.assertIn('world_depth_control',native)
        self.assertIn('const bool depth=raster && depth_requested && depth_sdl_permission;',win)
        self.assertIn('depth_requested!=depth_sdl_permission',win)
        self.assertIn('const ogl_rtx_triangle world_front_and_back[2]',win)
        self.assertIn('const ogl_rtx_triangle* geometry=world',win)
        self.assertIn('camera.near_clip_m=depth ? 2.f : 0.1f;',win)
        self.assertIn('camera.far_clip_m=depth ? 10.f : 100.f;',win)
        self.assertIn('depth_verified=depth && uploaded && pixel_verified;',win)
        self.assertIn('world_verified=world && projected && pixel_verified;',win)
        self.assertLess(win.index('depth_verified=depth && uploaded && pixel_verified;'),
                        win.index('SDL_GL_CreateContext( mWindow );'))
        self.assertNotIn('OGL_VULKAN_EXCLUSIVE_DEPTH_COLOR_PROBE',win)
        self.assertNotIn('OGL_VULKAN_EXCLUSIVE_DEPTH_COLOR_PROBE',
                         (ROOT/'indra/newview/oglrtxhost.cpp').read_text())

    def test_p05_dual_outstanding_gpu_fences_and_resize_camera_gate(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        start=module.index("uint32_t submit_world_sync_probe() {")
        end=module.index("// P05/3: verified Vulkan offscreen color frame",start)
        sync=module[start:end]
        self.assertIn("std::array<VkCommandBuffer,2> commands",sync)
        self.assertIn("std::array<VkFence,2> fences",sync)
        self.assertIn("++submitted;",sync)
        self.assertIn("world_device.sync_last_submitted=submitted;",sync)
        self.assertIn("world_device.sync_last_completed=submitted;",sync)
        self.assertIn("sync_max_outstanding=(std::max)(",sync)
        self.assertIn("fences.data(),VK_TRUE,2000000000ULL",sync)
        self.assertLess(sync.index("++submitted;"),
                        sync.index("world_device.sync_last_submitted=submitted;"))
        self.assertLess(sync.index("world_device.sync_last_submitted=submitted;"),
                        sync.index("const auto completed=wait("))
        self.assertNotIn("vkQueueWaitIdle",sync[sync.index("uint32_t submitted=0;"):])
        self.assertIn("disabled_after_gpu_timeout=true;",sync)
        self.assertIn("world_device.sync_last_completed=submitted;",sync)
        for suffix in ("max_outstanding","last_submitted","last_completed"):
            self.assertIn("ogl_vulkan_world_sync_"+suffix+"(void)",api)
            self.assertIn("ogl_vulkan_world_sync_"+suffix+"()",module)
        self.assertIn("sync_last_submitted()==(ready ? 2u : 0u)",native)
        self.assertIn("sync_last_completed()==(ready ? 2u : 0u)",native)
        self.assertIn("sync_max_outstanding()==(ready ? 2u : 0u)",native)
        self.assertIn("aspect_matches()==0",native)
        self.assertIn("world_camera_aspect = 0.f;",module)
        self.assertIn("world_device.world_camera_aspect=frame->aspect_ratio;",module)
        self.assertIn("world_device.world_camera_aspect-swap_aspect",module)
        self.assertIn("world_device.world_camera_ready=false;",module)
        self.assertIn("ogl_vulkan_world_camera_matches_swapchain(void)",api)
        self.assertIn("ogl_vulkan_world_camera_matches_swapchain()",module)
        self.assertIn("world_depth_aspect=rejected_stale && corrected_present;",native)
        self.assertIn("projection.aspect_ratio=800.f/450.f;",native)
        self.assertIn("read_rgb(400,225,237,48,38)",native)
        self.assertIn("world_depth_aspect?1:0",native)
        self.assertIn("!world_depth_aspect ||",native)
        self.assertIn("world_depth_invalid=world_camera(&projection)==0",native)
        self.assertIn("OGL_VULKAN_SDL_PREGL_DUAL_SYNC_PROBE",
                      (ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text())
        win=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        self.assertIn('OGL_VULKAN_FRAME_SYNC_PROBE',win)
        self.assertIn('dual_sync_verified=sync && highwater && queued && completed &&',win)
        self.assertIn('sync()==2 && highwater()==2 && queued()==2 && completed()==2',win)
        self.assertIn('(!dual_sync_requested || dual_sync_verified)',win)
        self.assertIn('dual_sync_verified ? "verified" : "failed"',win)
        self.assertLess(win.index('dual_sync_verified=sync && highwater'),
                        win.index('const int coords[5][2]'))
        self.assertLess(win.index('dual_sync_verified=sync && highwater'),
                        win.index('SDL_GL_CreateContext( mWindow );'))

        self.assertIn("aspect_matches()==0 &&",native)
        # Rendering is still clearly experimental: the visible world is GL.
        self.assertIn("vkQueueWaitIdle(world_device.queue)",module)
        self.assertNotIn("vkQueuePresentKHR(",
                         (ROOT/"indra/newview/oglrtxhost.cpp").read_text())

    def test_p05_persistent_wsi_gpu_frames_and_windows_msvc_portability(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        header=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        win=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        self.assertIn("sync_max_outstanding=(std::max)(",module)
        self.assertNotIn("sync_max_outstanding=std::max(",module)
        self.assertIn("struct OglPersistentClearFrames {",module)
        self.assertIn("std::vector<VkSemaphore> presented_by_image;",module)
        self.assertIn("std::vector<VkFence> image_in_flight;",module)
        self.assertIn("std::array<VkCommandBuffer,2> commands{};",module)
        self.assertIn("std::array<VkSemaphore,2> image_acquired{};",module)
        self.assertIn("std::array<VkFence,2> submit_fences{};",module)
        self.assertIn("VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT",module)
        self.assertIn("VK_FENCE_CREATE_SIGNALED_BIT",module)
        self.assertIn("persistent.framebuffers",module)
        self.assertIn("void release_persistent_frames()",module)
        self.assertIn("release_persistent_frames();",module)
        self.assertIn("persistent_teardown_idles;",module)
        self.assertIn("uint32_t present_persistent_clear_frame(uint32_t gl_released)",module)
        a=module.index("uint32_t present_persistent_clear_frame(uint32_t gl_released)")
        b=module.index("// P05/3 foundation: exercise the REAL Vulkan graphics queue",a)
        fn=module[a:b]
        self.assertIn('OGL_VULKAN_EXCLUSIVE_PERSISTENT_CLEAR_PROBE',fn)
        self.assertIn("p.frame_count>=p.max_frames",fn)
        self.assertIn('OGL_VULKAN_PERSISTENT_LONG_RUN_PROBE',fn)
        self.assertIn('OGL_VULKAN_PRESENT_TEST_X11',fn)
        self.assertIn('? 32u : 16u;',fn)
        self.assertIn("p.presented_by_image[image_index]",fn)
        self.assertIn("p.image_in_flight[image_index]",fn)
        self.assertIn("vkWaitForFences(world_device.device,1,&slot_fence",fn)
        self.assertIn("vkAcquireNextImageKHR(world_device.device",fn)
        self.assertIn("vkResetFences(world_device.device,1,&slot_fence)",fn)
        self.assertIn("vkQueuePresentKHR(world_device.queue,&present)",fn)
        self.assertNotIn("vkQueueWaitIdle(",fn)
        self.assertIn("world_device.release_persistent_frames();",module)
        self.assertIn("idle(queue)!=VK_SUCCESS",module)
        self.assertIn("++persistent_teardown_idles;",module)
        self.assertIn("OGL_VULKAN_PERSISTENT_WSI_TEST_X11",native)
        self.assertIn("persistent_clear(1)==1",native)
        self.assertIn("persistent_count()==persistent_frames",native)
        self.assertIn("persistent_builds()==2",native)
        self.assertIn("persistent_idles()==2",native)
        self.assertIn("OGL_VULKAN_SDL_PREGL_PERSISTENT_CLEAR_PROBE",win)
        self.assertIn("persistent_requested!=persistent_permission",win)
        self.assertIn("selected_present=persistent ? persistent_present : present",win)
        self.assertIn("const uint32_t before=verified_pixels;",win)
        self.assertIn("persistent_builds() == group",win)
        self.assertIn("persistent_releases() == group",win)
        self.assertIn("persistent_idles() == group",win)
        self.assertLess(win.index("persistent_verified=lifecycle"),
                        win.index("SDL_GL_CreateContext( mWindow );"))
        for suffix in ("clear_frame","frame_count","build_count",
                       "release_count","teardown_idles"):
            self.assertIn("ogl_vulkan_world_persistent_"+suffix,header)
        self.assertNotIn("ogl_vulkan_world_persistent_clear_frame",
                         (ROOT/"indra/newview/oglrtxhost.cpp").read_text())

    def test_p05_persistent_world_depth_pipeline_and_mesh_gpu_lifetime(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        win=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        self.assertIn("std::vector<VkImage> depth_images;",module)
        self.assertIn("std::vector<VkDeviceMemory> depth_memories;",module)
        self.assertIn("std::vector<VkImageView> depth_views;",module)
        self.assertIn("std::vector<VkFramebuffer> framebuffers;",module)
        self.assertIn("bool world_depth = false;",module)
        self.assertIn("OGL_VULKAN_EXCLUSIVE_PERSISTENT_WORLD_PROBE",module)
        self.assertIn("p.depth_images.resize(count,VK_NULL_HANDLE)",module)
        self.assertIn("p.depth_memories.resize(count,VK_NULL_HANDLE)",module)
        self.assertIn("p.depth_views.resize(count,VK_NULL_HANDLE)",module)
        self.assertIn("VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT",module)
        self.assertIn("VK_IMAGE_ASPECT_DEPTH_BIT",module)
        self.assertIn("subpass.pDepthStencilAttachment=world ? &depth_ref : nullptr;",module)
        self.assertIn("fi.attachmentCount=world ? 2u : 1u;",module)
        self.assertIn("p.pipeline_layout",module)
        self.assertIn("p.pipeline",module)
        self.assertIn("vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,p.pipeline)",module)
        self.assertIn("vkCmdPushConstants(cmd,p.pipeline_layout",module)
        self.assertIn("vkCmdBindVertexBuffers(cmd,0,1,&world_device.mesh_vertices,&offset)",module)
        self.assertIn("vkCmdBindIndexBuffer(cmd,world_device.mesh_indices,0,",module)
        self.assertIn("vkCmdDrawIndexed(cmd,world_device.mesh_triangle_count*3u,1,0,0,0)",module)
        self.assertIn("p.world_depth!=world",module)
        self.assertIn("if (frames.initialized && frames.submitted_any)",module)
        self.assertIn("OGL_VULKAN_PERSISTENT_WORLD_TEST_X11",native)
        self.assertIn("persistent_world_immutable=world_upload(mesh,2)==0",native)
        self.assertIn("persistent_world_moved=world_camera(&camera)==1",native)
        self.assertIn("persistent_world_resize=cleared",native)
        self.assertIn("persistent_world_ok=frames && persistent_clear(1)==0",native)
        self.assertIn("OGL_VULKAN_SDL_PREGL_PERSISTENT_WORLD_PROBE",win)
        self.assertIn("(!sequence || persistent_world_both)",win)
        self.assertIn("persistent_world=persistent_world_both",win)
        self.assertIn("upload(mesh,2)==1 && mesh_count()==2",win)
        self.assertIn("if (good && persistent_world)",win)
        self.assertIn("camera.near_clip_m=2.f;camera.far_clip_m=10.f;",win)
        self.assertIn("persistent-world=",win)
        a=module.index("uint32_t present_persistent_clear_frame(")
        b=module.index("// P05/3 foundation: exercise the REAL Vulkan graphics queue",a)
        self.assertNotIn("vkQueueWaitIdle(",module[a:b])
        self.assertLess(win.index("upload(mesh,2)==1 && mesh_count()==2"),
                        win.index("SDL_GL_CreateContext( mWindow );"))

    def test_p05_persistent_mesh_generation_bounded_fence_retirement(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        win=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        assert_names=("ogl_vulkan_world_mesh_generation",
                      "ogl_vulkan_world_mesh_retire_waits",
                      "ogl_vulkan_world_mesh_retire_failures")
        for name in assert_names:
            self.assertIn(name,api)
            self.assertIn(name,module)
            self.assertIn(name,native)
        a=module.index("int upload_world_geometry(")
        b=module.index("int ",a+10) if "int " in module[a+10:] else len(module)
        fn=module[a:b]
        self.assertIn("OGL_VULKAN_PERSISTENT_MESH_ROTATION_PROBE",fn)
        self.assertIn("if (frames.initialized && frames.submitted_any &&",fn)
        self.assertIn("frames.submitted_mesh_generations[i]==world_device.mesh_generation",fn)
        self.assertIn("VkFence unfinished[2]",fn)
        self.assertIn("2000000000ULL",fn)
        self.assertIn("frames.blocked=true;",fn)
        self.assertIn("++world_device.mesh_retire_failures;",fn)
        self.assertIn("++world_device.mesh_retire_waits;",fn)
        self.assertIn("++world_device.mesh_generation;",fn)
        self.assertLess(fn.index("ogl::is_valid_triangle"),
                        fn.index("VkFence unfinished[2]"))
        self.assertLess(fn.index("wait(world_device.device,pending,unfinished"),
                        fn.index("world_device.clear_mesh();"))
        self.assertNotIn("vkQueueWaitIdle(",fn)
        self.assertIn("p.submitted_mesh_generations[slot]=world_device.mesh_generation;",module)
        self.assertIn("OGL_VULKAN_PERSISTENT_MESH_ROTATION_TEST_X11",native)
        self.assertIn("mesh_retire_waits()==wait0+3",native)
        self.assertIn("rotation_invalid=world_upload(&invalid,1)==0",native)
        self.assertIn("world_upload(nullptr,0)==1",native)
        self.assertIn("mesh_generation()==gen0+6",native)
        self.assertIn("OGL_VULKAN_SDL_PREGL_MESH_ROTATION_PROBE",win)
        self.assertIn("persistent_world && sequence",win)
        self.assertIn("mesh_rotation_requested!=mesh_rotation_permission",win)
        self.assertIn("mesh_rotation_verified=good &&",win)
        self.assertIn("far_triangle=attempted_groups==1",win)
        self.assertIn("mesh-rotation=",win)
        self.assertLess(win.index("mesh_rotation_verified=good &&"),
                        win.index("SDL_GL_CreateContext( mWindow );"))
        self.assertNotIn("OGL_VULKAN_SDL_PREGL_MESH_ROTATION_PROBE",
                         (ROOT/"indra/newview/oglrtxhost.cpp").read_text())

    def test_p05_real_opensim_deferred_mesh_camera_generation_handoff(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        # The camera is submitted before a sometimes-later deferred scene
        # capture. The region callback MUST invalidate stale cached poses.
        self.assertIn("bool ogl_rtx_on_region_transition()",header)
        self.assertIn("bool ogl_rtx_on_region_transition() {",host)
        self.assertIn("last_live_world_camera_valid=false;",host)
        self.assertIn("++live_world_region_epoch;",host)
        self.assertIn("ogl_rtx_on_region_transition();",pipeline)
        i=pipeline.index("if (current_region_id != ogl_rtx_last_region_id)")
        j=pipeline.index("ogl_rtx_submit_world_triangles(nullptr, 0);",i)
        self.assertLess(i,pipeline.index("ogl_rtx_on_region_transition();",i))
        self.assertLess(pipeline.index("ogl_rtx_on_region_transition();",i),j)
        self.assertLess(pipeline.index("ogl_rtx_submit_camera_sun(viewport.getWidth()"),
                        pipeline.index("ogl_rtx_submit_deferred_world_triangles(\n                        triangles.front().position"))
        self.assertIn("ogl_rtx_frame_state last_live_world_camera{};",host)
        self.assertIn("last_live_world_camera=frame;",host)
        self.assertIn("last_live_world_camera_valid=true;",host)
        self.assertIn("OGL_VULKAN_LIVE_CAMERA_REBIND",host)
        self.assertIn("world_set_camera(&last_live_world_camera)==1",host)
        self.assertIn("world_camera_mesh_generation()==world_mesh_generation()",host)
        self.assertIn("world_input_ready()==1",host)
        self.assertIn("NO Vulkan world present, OpenGL remains active.",host)
        self.assertIn("live_world_rebinds%64==0",host)
        self.assertIn("world_camera_mesh_generation = nullptr;",host)
        self.assertIn("last_live_world_camera_valid=false;",host)
        self.assertNotIn("vkQueuePresentKHR(",host)
        # The module cannot report a ready scene until the camera has
        # been validated AGAIN for the actual accepted mesh generation.
        self.assertIn("uint64_t camera_mesh_generation = 0;",module)
        self.assertIn("world_device.camera_mesh_generation=world_device.mesh_generation;",module)
        self.assertIn("world_device.camera_mesh_generation!=world_device.mesh_generation",module)
        self.assertIn("world_device.camera_mesh_generation==world_device.mesh_generation",module)
        self.assertIn("ogl_vulkan_world_camera_mesh_generation(void)",api)
        self.assertIn("camera_mesh_generation()==mesh_generation()",native)
        self.assertIn("camera_mesh_generation()==0",native)
        # A repeated exact real OpenSim snapshot does not touch VkBuffers
        # or invalidate a correctly paired camera when explicit opt-in set.
        self.assertIn("OGL_VULKAN_WORLD_MESH_DEDUP",module)
        self.assertIn("std::vector<ogl_rtx_triangle> mesh_cpu_snapshot;",module)
        self.assertIn("std::memcmp(world_device.mesh_cpu_snapshot.data(),triangles,",module)
        self.assertIn("++world_device.mesh_dedup_skips;",module)
        self.assertIn("ogl_vulkan_world_mesh_dedup_skips(void)",api)
        a=module.index("int upload_world_geometry(")
        b=module.index("int ",a+10)
        fn=module[a:b]
        self.assertLess(fn.index("std::memcmp("),fn.index("VkFence unfinished[2]"))
        self.assertLess(fn.index("std::memcmp("),fn.index("world_device.clear_mesh();"))
        self.assertIn("accepted_snapshot.assign(triangles,triangles+count)",fn)
        self.assertIn("world_device.mesh_cpu_snapshot.swap(accepted_snapshot)",fn)
        self.assertLess(fn.index("accepted_snapshot.assign(triangles,triangles+count)"),
                        fn.index("const auto get_device="))
        self.assertLess(fn.index("world_device.clear_mesh();"),
                        fn.index("world_device.mesh_cpu_snapshot.swap(accepted_snapshot)"))
        self.assertIn("mesh_dedup_skips()==dedup0+1",native)
        self.assertIn("camera_mesh_generation()==mesh_generation()",native)
        self.assertIn("const bool changed=!world_mesh_generation ||",host)
        self.assertIn("if (changed) world_inputs_live_logged=false;",host)
        self.assertIn("if (enabled && tint_success && !world_inputs_live_logged &&",host)
        self.assertIn("if (count==0) {\n            world_inputs_live_logged=false;",host)
        self.assertIn("if (!rtx_submit_geometry || !rtx_hardware_ready) return;",host)
        # It remains GL-owned after actual Firestorm scene capture.
        self.assertNotIn("ogl_vulkan_world_persistent_clear_frame",host)

    def test_p05_genuine_region_barrier_two_frame_slots_and_opengl_fallback(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        sdl=(ROOT/"indra/llwindow/llwindowsdl2.cpp").read_text()
        symbols=("ogl_vulkan_world_invalidate_region",
                 "ogl_vulkan_world_region_barrier_accepts",
                 "ogl_vulkan_world_region_barrier_failures")
        for symbol in symbols:
            self.assertIn(symbol,module)
            self.assertIn(symbol,api)
            self.assertIn(symbol,native)
        a=module.index("int invalidate_live_world_region()")
        b=module.index("// P05/4 Vulkan vertex/index buffer staging",a)
        fun=module[a:b]
        for token in ("OGL_VULKAN_LIVE_REGION_BARRIER",
                      "OGL_VULKAN_WORLD_MESH_UPLOAD",
                      "world_device.world_camera_ready=false;",
                      "world_device.camera_mesh_generation=0;",
                      "frames.submit_pending[i]",
                      "frames.submitted_mesh_generations[i]==world_device.mesh_generation",
                      "2000000000ULL","VK_TRUE","frames.blocked=true;",
                      "++world_device.region_barrier_failures;",
                      "++world_device.mesh_retire_waits;",
                      "world_device.clear_mesh();",
                      "++world_device.mesh_generation;",
                      "++world_device.region_barrier_accepts;"):
            self.assertIn(token,fun)
        self.assertNotIn("vkQueueWaitIdle",fun)
        self.assertLess(fun.index("world_device.world_camera_ready=false;"),
                        fun.index("wait(world_device.device,pending,unfinished"))
        self.assertLess(fun.index("wait(world_device.device,pending,unfinished"),
                        fun.index("world_device.clear_mesh();"))
        a=module.index("int upload_world_geometry(")
        b=module.index("int ",a+10)
        upload=module[a:b]
        self.assertIn("if (frames.blocked) return 0;",upload)
        self.assertIn("(world_device.mesh_vertices || world_device.mesh_indices)",upload)
        self.assertIn("OGL_VULKAN_PERSISTENT_MESH_ROTATION_PROBE",upload)
        self.assertIn("OGL_VULKAN_LIVE_REGION_BARRIER_TEST_X11",native)
        self.assertIn("region_barrier_accepts()==accepts0+2",native)
        self.assertIn("world_upload(&far,1)==1",native)
        self.assertIn("world_upload(&far,1)==0",native)
        self.assertIn("persistent_idles()==idle0",native)
        self.assertIn("bool ogl_rtx_on_region_transition()",header)
        self.assertIn("bool ogl_rtx_on_region_transition() {",host)
        self.assertIn("world_invalidate_region()==1",host)
        self.assertIn("OGL_VULKAN_LIVE_REGION_BARRIER",host)
        self.assertIn("return done;",host)
        self.assertIn("const bool gpu_region_retired=ogl_rtx_on_region_transition();",pipeline)
        self.assertIn("if (!ogl_rtx_last_region_id.isNull() && !gpu_region_retired)",pipeline)
        self.assertLess(pipeline.index("const bool gpu_region_retired=ogl_rtx_on_region_transition();"),
                        pipeline.index("ogl_rtx_submit_world_triangles(nullptr, 0);"))
        self.assertIn("OGL_VULKAN_SDL_PREGL_REGION_BARRIER_PROBE",sdl)
        self.assertIn("region_barrier_requested!=region_barrier_permission",sdl)
        self.assertIn("!mesh_rotation;",sdl)
        self.assertIn("good=invalidate_region()==1",sdl)
        self.assertIn("region_barrier_verified=good &&",sdl)
        self.assertIn("region-barrier=",sdl)
        self.assertLess(sdl.index("region_barrier_verified=good &&"),
                        sdl.index("SDL_GL_CreateContext( mWindow );"))
        self.assertNotIn("ogl_vulkan_world_persistent_clear_frame",host)

    def test_p05_real_world_vulkan_offscreen_gpu_raster_without_gl_window_ownership(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        for symbol in ("ogl_vulkan_world_render_offscreen_scene",
                       "ogl_vulkan_world_offscreen_scene_frames",
                       "ogl_vulkan_world_offscreen_scene_pixels"):
            self.assertIn(symbol,module)
            self.assertIn(symbol,api)
            self.assertIn(symbol,native)
            self.assertIn(symbol,host)
        a=module.index("uint32_t render_offscreen_color_frame(")
        b=module.index("// Independent opt-in camera projection",a)
        f=module[a:b]
        for flag in ("OGL_VULKAN_OFFSCREEN_FRAME_PROBE",
                     "OGL_VULKAN_WORLD_OFFSCREEN_RASTER",
                     "OGL_VULKAN_WORLD_MESH_UPLOAD",
                     "OGL_VULKAN_WORLD_CAMERA_PROBE"):
            self.assertIn(flag,f)
        self.assertIn("world_device.camera_mesh_generation!=world_device.mesh_generation",f)
        self.assertIn("world_device.mesh_vertices",f)
        self.assertIn("world_device.mesh_indices",f)
        self.assertIn("OglOffscreenWorldCache transient;",f)
        self.assertIn("auto& resource=scene ? world_device.offscreen_world_cache : transient;",f)
        self.assertIn("auto& color=resource.color;",f)
        self.assertIn("auto& depth_mem=resource.depth_mem;",f)
        self.assertIn("VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT",f)
        self.assertIn("VK_IMAGE_ASPECT_DEPTH_BIT",f)
        self.assertIn("subpass.pDepthStencilAttachment=scene ? &depth_ref : nullptr;",f)
        self.assertIn("VK_COMPARE_OP_LESS",f)
        self.assertIn("sizeof(ogl_dev_world_vert_spv)",f)
        self.assertIn("sizeof(ogl_dev_depth_frag_spv)",f)
        self.assertIn("vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,",f)
        self.assertIn("vkCmdPushConstants(command,pipeline_state.pipeline_layout,",f)
        self.assertIn("auto offscreen_projection=world_device.world_to_clip;",f)
        self.assertIn("world_light_push.data()",f)
        self.assertIn("vkCmdBindVertexBuffers(command,0,2,buffers,offsets)",f)
        self.assertIn("vkCmdBindIndexBuffer(command,world_device.mesh_indices,0,",f)
        self.assertIn("vkCmdDrawIndexed(command,world_device.mesh_triangle_count*3u,",f)
        self.assertIn("vkCmdCopyImageToBuffer(command,color,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,",f)
        self.assertIn("vkWaitForFences(world_device.device,1,&fence,",f)
        self.assertIn("2000000000ULL",f)
        self.assertIn("world_device.offscreen_world_covered_pixels=changed;",f)
        self.assertIn("++world_device.offscreen_world_frames;",f)
        self.assertIn("return correct ? 1u : 0u;",f)
        self.assertNotIn("vkQueuePresentKHR(",f)
        self.assertNotIn("SDL_",f)
        self.assertNotIn("vkCreateSwapchainKHR(",f)
        self.assertIn("offscreen_world_frames()==offscreen_start+3",native)
        self.assertIn("offscreen_world_pixels()==0",native)
        self.assertIn("offscreen_world_pixels()>0",native)
        self.assertIn("denied_no_mesh==0",native)
        self.assertIn("OGL_VULKAN_LIVE_WORLD_OFFSCREEN",host)
        self.assertIn("OGL_VULKAN_WORLD_OFFSCREEN_RASTER",host)
        self.assertIn("OGL_VULKAN_OFFSCREEN_FRAME_PROBE",host)
        self.assertIn("world_render_offscreen_scene()==1",host)
        self.assertIn("live_world_rebinds<=3 || live_world_rebinds%64==0",host)
        self.assertIn("bounded private VkImage, NO Vulkan present",host)
        self.assertNotIn("ogl_vulkan_world_attach_x11_window(",host)
        self.assertNotIn("vkQueuePresentKHR(",host)

    def test_p05_offscreen_world_cache_reuses_gpu_objects_and_tears_down(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        begin=module.index("struct OglOffscreenWorldCache {")
        end=module.index("// P05/1 developer-only Vulkan graphics device bootstrap",begin)
        struct=module[begin:end]
        for token in ("VkImage color=VK_NULL_HANDLE, depth=VK_NULL_HANDLE;",
                      "VkDeviceMemory color_mem=VK_NULL_HANDLE, depth_mem=VK_NULL_HANDLE,",
                      "VkRenderPass render_pass=VK_NULL_HANDLE;",
                      "VkFramebuffer framebuffer=VK_NULL_HANDLE;",
                      "VkBuffer readback=VK_NULL_HANDLE;",
                      "VkCommandPool command_pool=VK_NULL_HANDLE;",
                      "VkCommandBuffer command=VK_NULL_HANDLE;",
                      "VkFence fence=VK_NULL_HANDLE;",
                      "VkPipeline pipeline=VK_NULL_HANDLE;",
                      "bool initialized=false;","bool submitted=false;"):
            self.assertIn(token,struct)
        close=module.index("void close() {")
        self.assertLess(module.index("release_offscreen_world_cache();",close),
                        module.index("clear_mesh();",close))
        self.assertLess(module.index("clear_mesh();",close),
                        module.index("destroy_device(device, nullptr)",close))
        a=module.index("void release_offscreen_world_cache() {")
        b=module.index("void clear_mesh() {",a)
        rel=module[a:b]
        self.assertIn("if (c.submitted)",rel)
        self.assertIn("wait(device,1,&c.fence,VK_TRUE,2000000000ULL)",rel)
        self.assertIn("vkDestroyPipeline(device,c.pipeline,nullptr);",rel)
        self.assertIn("vkDestroyImage(device,c.depth,nullptr);",rel)
        self.assertIn("vkDestroyImage(device,c.color,nullptr);",rel)
        self.assertIn("vkDestroyBuffer(device,c.readback,nullptr);",rel)
        self.assertIn("vkFreeMemory(device,c.readback_mem,nullptr);",rel)
        self.assertNotIn("vkQueueWaitIdle(",rel)
        a=module.index("uint32_t render_offscreen_color_frame(")
        b=module.index("// Independent opt-in camera projection",a)
        fn=module[a:b]
        self.assertIn("const bool reuse=scene && resource.initialized;",fn)
        self.assertIn("if (!reuse) {",fn)
        self.assertIn("if (reuse && vkResetCommandBuffer(command,0)!=VK_SUCCESS)",fn)
        self.assertIn("vkResetFences(world_device.device,1,&fence)",fn)
        self.assertIn("VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT",fn)
        self.assertIn("resource.submitted=true;",fn)
        self.assertIn("resource.submitted=false;",fn)
        self.assertIn("++world_device.offscreen_world_builds;",fn)
        self.assertIn("++world_device.offscreen_world_reuses;",fn)
        self.assertIn("if (result!=2 && (!scene || result!=1)) cleanup();",fn)
        self.assertIn("if (scene && resource.initialized)",fn)
        for name in ("ogl_vulkan_world_offscreen_cache_builds",
                     "ogl_vulkan_world_offscreen_cache_reuses",
                     "ogl_vulkan_world_offscreen_cache_releases",
                     "ogl_vulkan_world_offscreen_cache_release"):
            self.assertIn(name,module)
            self.assertIn(name,api)
            self.assertIn(name,native)
        self.assertIn("offscreen_cache_builds()==offscreen_build0+1",native)
        self.assertIn("offscreen_cache_reuses()==offscreen_reuse0+2",native)
        self.assertIn("offscreen_cache_releases()==offscreen_release0+1",native)
        self.assertIn("offscreen_cache_release()==0",native)
        self.assertNotIn("vkQueuePresentKHR(",fn)
        self.assertNotIn("vkCreateSwapchainKHR(",fn)

    def test_p05_vulkan_private_world_render_dimensions_resize_and_aspect(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        report=(ROOT/"ogl/scripts/ogl-test-report.py").read_text()
        a=module.index("VkExtent2D ogl_private_world_offscreen_extent()")
        b=module.index("// P05/3: verified Vulkan offscreen color frame",a)
        select=module[a:b]
        for size in ("32x32","128x96","256x144","512x288"):
            self.assertIn(size,select)
            self.assertIn(size,api)
            w,h=size.split("x")
            self.assertIn(f"({w},{h})",report)
            self.assertIn(size,native)
        self.assertIn('return {0,0};',select)
        a=module.index("uint32_t render_offscreen_color_frame(")
        b=module.index("// Independent opt-in camera projection",a)
        fn=module[a:b]
        for contract in (
            "ogl_private_world_offscreen_extent()",
            "width>512 || height>288",
            "uint64_t(width)*height>512u*288u",
            "VkDeviceSize(width)*height*4",
            "resource.extent.width!=width",
            "resource.extent.height!=height",
            "world_device.release_offscreen_world_cache();",
            "if (resource.initialized || resource.submitted) return 0;",
            "image.extent={width,height,1};",
            "di.format=depth_format;di.extent={width,height,1};",
            "fb.width=width;",
            "fb.height=height;",
            "viewport.width=static_cast<float>(width);",
            "viewport.height=static_cast<float>(height);",
            "scissor.extent={width,height};",
            "copy.imageExtent={width,height,1};",
            "offscreen_projection=world_device.world_to_clip;",
            "world_device.world_camera_aspect /",
            "(float(width)/float(height))",
            "offscreen_projection[col*4]*=aspect_correction;",
            "world_light_push.data()",
            "world_device.offscreen_world_width=width;",
            "world_device.offscreen_world_height=height;",
            "resource.extent={width,height};",
        ):
            self.assertIn(contract,fn)
        for symbol in ("ogl_vulkan_world_offscreen_scene_width",
                       "ogl_vulkan_world_offscreen_scene_height"):
            self.assertIn(symbol,api)
            self.assertIn(symbol,module)
            self.assertIn(symbol,host)
            self.assertIn(symbol,native)
        self.assertIn('" width=" << (world_offscreen_scene_width ?',host)
        self.assertIn('" height=" << (world_offscreen_scene_height ?',host)
        self.assertIn("pixel<width*height;",fn)
        self.assertIn('"8192x8192"',native)
        self.assertIn('offscreen_world_frames()==offscreen_start+12',native)
        self.assertIn('offscreen_cache_reuses()==offscreen_reuse0+6',native)
        self.assertIn('offscreen_cache_builds()==offscreen_build0+6',native)
        self.assertIn('offscreen_cache_releases()==offscreen_release0+6',native)
        self.assertIn('mesh_uploads()==static_mesh_uploads',native)
        self.assertNotIn('vkQueuePresentKHR(',fn)
        self.assertNotIn('vkCreateSwapchainKHR(',fn)
        self.assertNotIn('SDL_',fn)

    def test_p05_vulkan_world_face_normal_lambert_gpu_shader_is_optin(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        report=(ROOT/"ogl/scripts/ogl-test-report.py").read_text()
        for name in ("ogl_dev_world_lambert_vert_spv",
                     "ogl_dev_world_lambert_frag_spv"):
            self.assertIn("static constexpr uint32_t "+name+"[]",module)
            self.assertIn("sizeof("+name+")",module)
        for name in ("ogl_vulkan_world_offscreen_scene_lit_pixels",
                     "ogl_vulkan_world_offscreen_scene_mean_luminance"):
            for source in (api,module,native,host):
                self.assertIn(name,source)
        begin=module.index("uint32_t render_offscreen_color_frame(")
        end=module.index("// Independent opt-in camera projection",begin)
        frame=module[begin:end]
        for required in (
            'std::getenv("OGL_VULKAN_WORLD_LAMBERT_PROBE")',
            'std::strcmp(lambert_flag,"0")!=0',
            'std::strcmp(lambert_flag,"1")!=0',
            'resource.shading_mode!=shading_mode',
            'shading_mode==1u',
            'ogl_dev_world_lambert_vert_spv',
            'ogl_dev_world_lambert_frag_spv',
            'ogl_dev_depth_frag_spv',
            'vec3', # absent in C++ shader words; skip?
            'world_device.offscreen_world_lit_pixels=lit;',
            'world_device.offscreen_world_mean_luminance=changed ?',
            'resource.shading_mode=shading_mode;',
            'vkCmdDrawIndexed(command,world_device.mesh_triangle_count*3u,',
            'VK_COMPARE_OP_LESS'):
            if required!='vec3':
                self.assertIn(required,frame)
        self.assertIn("offscreen_world_lit_pixels = offscreen_world_mean_luminance = 0;",module)
        self.assertIn("offscreen_lit_pixels()>0",native)
        self.assertIn("tilted_luminance!=offscreen_mean_luminance()",native)
        self.assertIn("offscreen_world_frames()==offscreen_start+18",native)
        self.assertIn("offscreen_cache_builds()==offscreen_build0+10",native)
        self.assertIn("offscreen_cache_reuses()==offscreen_reuse0+8",native)
        self.assertIn("offscreen_cache_releases()==offscreen_release0+10",native)
        self.assertIn("OGL_VULKAN_WORLD_LAMBERT_PROBE",api)
        self.assertIn('lit_pixels=',host)
        self.assertIn('mean_luminance=',host)
        self.assertIn("lit>pixels or mean>765",report)
        self.assertNotIn("vkQueuePresentKHR(",frame)
        self.assertNotIn("SDL_",frame)

    def test_p05_world_sun_direction_gpu_push_80_bytes_and_dynamic_caching(self):
        plugin=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        a=plugin.index("int set_world_projection_camera(")
        b=plugin.index("// A genuine Firestorm region transition",a)
        camera=plugin[a:b]
        for phrase in ('OGL_VULKAN_WORLD_SUN_LIGHT_PROBE',
                       'sun_length2<0.25 || sun_length2>4.0',
                       'world_device.world_sun_direction[i]=frame->sun_direction[i]*inv_sun;',
                       'world_device.world_sun_ready=true;',
                       'world_device.world_sun_ready=false;'):
            self.assertIn(phrase,camera)
        a=plugin.index("uint32_t render_offscreen_color_frame(")
        b=plugin.index("// Independent opt-in camera projection",a)
        draw=plugin[a:b]
        for phrase in ('OGL_VULKAN_WORLD_SUN_LIGHT_PROBE',
                       'sun_enabled && (!lambert_enabled || !world_device.world_sun_ready)',
                       'const uint32_t shading_mode=atlas_hq ? 8u :',
                       'resource.shading_mode!=shading_mode',
                       'ogl_dev_world_sunlight_frag_spv',
                       'sizeof(float)*20',
                       'VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT',
                       'world_device.world_sun_direction.begin()',
                       'world_light_push.data()',
                       'vkCmdDrawIndexed(command,world_device.mesh_triangle_count*3u,'):
            self.assertIn(phrase,draw)
        self.assertIn("static constexpr uint32_t ogl_dev_world_sunlight_frag_spv[]",plugin)
        self.assertIn("OGL_VULKAN_WORLD_SUN_LIGHT_PROBE",host)
        self.assertIn("std::memcmp(a.sun_direction,b.sun_direction,",host)
        self.assertIn('P05 Vulkan real-sun GPU Lambert:',native)
        self.assertIn("sun_facing_luminance!=sun_side_luminance",native)
        self.assertIn("world_camera(&camera)==0",native)
        self.assertIn("mesh_uploads()==sun_mesh_uploads",native)
        self.assertIn("offscreen_cache_reuses()==offscreen_reuse0+10",native)
        self.assertIn("offscreen_world_frames()==offscreen_start+22",native)
        self.assertIn('"OGL_VULKAN_WORLD_SUN_LIGHT_PROBE",',native)
        self.assertNotIn("vkQueuePresentKHR(",draw)
        self.assertNotIn("SDL_",draw)

    def test_p05_real_opensim_face_tint_shader_gpu_and_generation(self):
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        gpu=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        for symbol in ("ogl_rtx_face_tint",
                       "ogl_vulkan_world_upload_face_tints",
                       "ogl_vulkan_world_face_tint_mesh_generation",
                       "ogl_vulkan_world_offscreen_scene_mean_red",
                       "ogl_vulkan_world_offscreen_scene_mean_blue"):
            self.assertIn(symbol,api)
            self.assertIn(symbol,module)
        for marker in ('OGL_VULKAN_WORLD_FACE_TINT_PROBE',
                       'face_tints.resize(triangles.size(),neutral_tint)',
                       'volume_object->getTE(f)',
                       'te->getColor()',
                       'rgba.mV[3]>=0.999f',
                       'if (tint_scan) face_tints.push_back(face_tint)',
                       '(!tint_scan || face_tints.size()==triangles.size())',
                       'tint_scan ? face_tints.front().rgb : nullptr'):
            self.assertIn(marker,pipeline)
        self.assertIn("const float *face_tints_rgb,",header)
        self.assertIn("const float *face_uvs,const float *face_material_slots,",header)
        for marker in ("live_deferred_face_tints=face_tints_rgb ?",
                       "world_upload_face_tints(live_deferred_face_tints,count)==1",
                       "world_tint_mesh_generation()==world_mesh_generation()",
                       "world_upload_face_tints(nullptr,0)",
                       "if (live_deferred_capture_call && tint_success &&"):
            self.assertIn(marker,host)
        start=module.index("int upload_world_face_tints(")
        end=module.index("void gpu_probe_cleanup()",start)
        tinted=module[start:end]
        for marker in ('OGL_VULKAN_WORLD_FACE_TINT_PROBE',
                       'OGL_VULKAN_WORLD_LAMBERT_PROBE',
                       'OGL_VULKAN_WORLD_SUN_LIGHT_PROBE',
                       'count!=world_device.mesh_triangle_count',
                       'world_device.offscreen_world_cache.submitted',
                       'world_device.clear_face_tints()',
                       'std::isfinite(rgb)',
                       'std::vector<float> expanded(size_t(count)*9u)',
                       'upload_world_attribute_bytes(expanded.data(),bytes,',
                       'world_device.tint_mesh_generation=world_device.mesh_generation',
                       '++world_device.tint_uploads'):
            self.assertIn(marker,tinted)
        for marker in ('VK_BUFFER_USAGE_VERTEX_BUFFER_BIT',
                       'VK_MEMORY_PROPERTY_HOST_COHERENT_BIT',
                       'VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};'):
            self.assertIn(marker,module)
        start=module.index("uint32_t render_offscreen_color_frame(")
        end=module.index("// Independent opt-in camera projection",start)
        draw=module[start:end]
        for marker in ('(tinted ? 3u :',
                       'ogl_dev_world_face_tint_vert_spv',
                       'ogl_dev_world_face_tint_frag_spv',
                       'resource.shading_mode!=shading_mode',
                       'world_device.tint_mesh_generation!=world_device.mesh_generation',
                       'world_device.tint_vertex_count!=world_device.mesh_triangle_count*3u',
                       'vkCmdBindVertexBuffers(command,0,2,buffers,offsets)',
                       'world_device.offscreen_world_mean_red=changed ?',
                       'world_device.offscreen_world_mean_blue=changed ?'):
            self.assertIn(marker,draw)
        self.assertIn("static constexpr uint32_t ogl_dev_world_face_tint_vert_spv[]",module)
        self.assertIn("static constexpr uint32_t ogl_dev_world_face_tint_frag_spv[]",module)
        for marker in ('P05 Vulkan face tint GPU:',
                       'tint_upload(&invalid_tint,1)==0',
                       'tint_upload(&red_tint,2)==0',
                       'tint_upload(&red_tint,1)==1',
                       'tint_upload(&blue_tint,1)==1',
                       'tint_mesh_generation()==0',
                       'offscreen_world_frames()==offscreen_start+26'):
            self.assertIn(marker,gpu)
        self.assertNotIn('vkQueuePresentKHR(',draw)
        self.assertNotIn('SDL_',draw)

    def test_p05_live_opensim_face_uv_provenance_gpu_and_checker(self):
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        for name in ("ogl_rtx_face_uv",
                     "ogl_vulkan_world_upload_face_uvs",
                     "ogl_vulkan_world_face_uv_mesh_generation",
                     "ogl_vulkan_world_face_uv_uploads",
                     "ogl_vulkan_world_offscreen_scene_mean_green"):
            self.assertIn(name,api)
            self.assertIn(name,module)
        for marker in (
            "OGL_VULKAN_WORLD_UV_CHECKER_PROBE",
            "if (uv_scan) face_uvs.reserve(ogl_triangle_budget);",
            "face_uvs.resize(triangles.size(),neutral_uv)",
            "face.mTexCoords",
            "te->getTexGen()==0",
            "te->getScale(&uv_scale_s,&uv_scale_t)",
            "te->getOffset(&uv_offset_s,&uv_offset_t)",
            "te->getRotation(&uv_angle)",
            "const LLVector2 tc(face.mTexCoords[index]);",
            "uv_cos+tex_v*uv_sin",
            "if (uv_scan)",
            "face_uvs.push_back(uv_triangle_valid ?",
            "(!uv_scan || face_uvs.size()==triangles.size())",
            "uv_scan ? face_uvs.front().uv : nullptr",
        ):
            self.assertIn(marker,pipeline)
        self.assertIn("const float *face_uvs,const float *face_material_slots,",header)
        for marker in (
            "live_deferred_face_uvs=face_uvs ?",
            "world_upload_face_uvs(live_deferred_face_uvs,count)==1",
            "world_uv_mesh_generation()==world_mesh_generation()",
            "world_upload_face_uvs(nullptr,0)",
            "tint_success=tint_success && tint_needed &&",
        ):
            self.assertIn(marker,host)
        start=module.index("int upload_world_face_uvs(")
        end=module.index("void gpu_probe_cleanup()",start)
        upload=module[start:end]
        for marker in (
            "OGL_VULKAN_WORLD_UV_CHECKER_PROBE",
            "OGL_VULKAN_WORLD_FACE_TINT_PROBE",
            "OGL_VULKAN_WORLD_LAMBERT_PROBE",
            "OGL_VULKAN_WORLD_SUN_LIGHT_PROBE",
            "count!=world_device.mesh_triangle_count",
            "world_device.tint_mesh_generation!=world_device.mesh_generation",
            "std::isfinite(coord)",
            "coord<-4096.f",
            "sizeof(float)*6",
            "world_device.clear_face_uvs()",
            "upload_world_attribute_bytes(uv_triangles,",
            "world_device.uv_mesh_generation=world_device.mesh_generation",
            "++world_device.uv_uploads",
        ):
            self.assertIn(marker,upload)
        self.assertIn("void clear_face_uvs()",module)
        self.assertIn("clear_face_uvs();\n        clear_face_tints();",module)
        self.assertIn("bool upload_world_attribute_bytes(",module)
        draw=module[module.index("uint32_t render_offscreen_color_frame("):
                    module.index("// Independent opt-in camera projection")]
        for marker in (
            "const uint32_t shading_mode=atlas_hq ? 8u :",
            "ogl_dev_world_uv_checker_vert_spv",
            "ogl_dev_world_uv_checker_frag_spv",
            "VK_FORMAT_R32G32_SFLOAT",
            "shading_mode==4u",
            "world_device.uv_mesh_generation!=world_device.mesh_generation",
            "world_device.uv_vertex_count!=world_device.mesh_triangle_count*3u",
            "vkCmdBindVertexBuffers(command,0,3,buffers,offsets)",
            "world_device.offscreen_world_mean_green=changed ?",
        ):
            self.assertIn(marker,draw)
        for symbol in ("ogl_dev_world_uv_checker_vert_spv",
                       "ogl_dev_world_uv_checker_frag_spv"):
            self.assertIn("static constexpr uint32_t "+symbol+"[]",module)
        for marker in (
            'P05 Vulkan UV checker GPU:',
            'uv_upload(&checker_invalid,1)==0',
            'uv_upload(&checker_red,2)==0',
            'uv_upload(&checker_green,1)==1',
            'uv_upload(&checker_wrapped,1)==1',
            'uv_mesh_generation()==0',
            'offscreen_world_frames()==offscreen_start+30',
        ):
            self.assertIn(marker,native)
        self.assertNotIn("vkQueuePresentKHR(",draw)
        self.assertNotIn("SDL_",draw)

    def test_p05_actual_firestorm_decoded_opaque_texture_source_fail_closed(self):
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        for marker in (
            "inline bool ogl_rtx_thumbnail_opaque_rgba8(",
            "result.clear();",
            "out_width=out_height=0;",
            "width>1024 || height>1024",
            "components!=3 && components!=4",
            "data_size<needed",
            "pixels[px]!=255u",
            "std::max(1u,(width*64u)/maximum)",
            "converted[output]=pixels[input];",
            "converted[output+1]=pixels[input+1];",
            "converted[output+2]=pixels[input+2];",
            "result.swap(converted);",
            "const std::uint8_t *decoded_rgba,",
            "unsigned image_width,unsigned image_height,unsigned count"):
            self.assertIn(marker,header)
        for marker in (
            '"OGL_VULKAN_WORLD_RGBA_IMAGE_PROBE"',
            '"OGL_VULKAN_LIVE_RGBA_SOURCE"',
            "const bool rgba_scan=uv_scan && rgba_pref &&",
            "bool homogeneous_rgba=rgba_scan && !atlas_scan &&",
            "LLPointer<LLViewerFetchedTexture> homogeneous_image;",
            "dynamic_cast<LLViewerFetchedTexture*>(",
            "volume_object->getTEImage(static_cast<U8>(f))",
            "face_image->isMissingAsset()",
            "LLViewerFetchedTexture::sDefaultImagep.get()",
            "!volume_object->mTextureAnimp",
            "te->getMaterialParams().isNull()",
            "!te->getGLTFRenderMaterial()",
            "diffuse.mV[3]>=0.999f",
            "if (!uv_triangle_valid || !face_image)",
            "homogeneous_image.get()!=face_image",
            "homogeneous_image->isRawImageValid()",
            "LLPointer<LLImageRaw> raw=homogeneous_image->getRawImage();",
            "LLImageDataSharedLock lock(raw.get());",
            "ogl_rtx_thumbnail_opaque_rgba8(",
            "decoded_rgba.empty() ? nullptr : decoded_rgba.data()",
        ):
            self.assertIn(marker,pipeline)
        # No GL pixel readback or texture-forcing in the scanner.
        self.assertNotIn("readbackRawImage()",pipeline)
        self.assertNotIn("forceToSaveRawImage(",pipeline)
        for marker in (
            "ogl_vulkan_world_upload_rgba_image",
            "ogl_vulkan_world_rgba_mesh_generation",
            "const bool rgba_needed=rgba_requested &&",
            "std::strcmp(rgba_live_flag,",
            "live_deferred_capture_call && live_deferred_rgba_pixels &&",
            "live_deferred_rgba_width<=(atlas_hq_allowed ? 256u : 64u)",
            "live_deferred_rgba_height<=(atlas_hq_allowed ? 256u : 64u)",
            "world_rgba_mesh_generation()==world_mesh_generation()",
            "world_upload_rgba_image(nullptr,0,0)",
            "live_deferred_capture_valid=false;",
            "live_deferred_rgba_pixels=nullptr;",
            "live_deferred_rgba_width=live_deferred_rgba_height=0;",
        ):
            self.assertIn(marker,host)
        for marker in (
            '#include "../../../indra/newview/oglrtxhost.h"',
            "test_opensim_raw_thumbnail_policy();",
            "assert(!ogl_rtx_thumbnail_opaque_rgba8(",
            "const std::uint8_t transparent[8]",
            "large_rgb.data(),large_rgb.size(),256,128,3",
            "w==64 && h==32",
            "w==1 && h==64",
            "P05 real decoded OpenSim raw thumbnail policy:",
        ):
            self.assertIn(marker,native)
        self.assertIn("rgba_cpu_texels.swap(accepted)",module)
        self.assertIn("world_device.rgba_mesh_generation=world_device.mesh_generation",module)
        self.assertIn("VK_IMAGE_USAGE_SAMPLED_BIT",module)
        self.assertIn("vkCmdBindDescriptorSets(command,",module)
        self.assertNotIn("vkQueuePresentKHR(",host[host.index("void ogl_rtx_submit_deferred_world_triangles("):
                                                   host.index("bool ogl_rtx_track_local_object(",host.index("void ogl_rtx_submit_deferred_world_triangles("))])

    def test_p05_decoded_rgba8_vkimage_sampler_descriptor_gpu_probe(self):
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        for symbol in ("ogl_vulkan_world_upload_rgba_image",
                       "ogl_vulkan_world_rgba_mesh_generation",
                       "ogl_vulkan_world_rgba_uploads"):
            self.assertIn(symbol,api)
            self.assertIn(symbol,module)
        start=module.index("int upload_world_rgba_image(")
        end=module.index("void gpu_probe_cleanup()",start)
        upload=module[start:end]
        for marker in (
            "OGL_VULKAN_WORLD_RGBA_IMAGE_PROBE",
            "OGL_VULKAN_WORLD_UV_CHECKER_PROBE",
            "OGL_VULKAN_WORLD_FACE_TINT_PROBE",
            "OGL_VULKAN_WORLD_SUN_LIGHT_PROBE",
            "world_device.offscreen_world_cache.submitted",
            "width>max_source_edge || height>max_source_edge",
            "rgba[i]!=255u",
            "world_device.uv_mesh_generation!=world_device.mesh_generation",
            "world_device.tint_mesh_generation!=world_device.mesh_generation",
            "world_device.release_offscreen_world_cache();",
            "world_device.rgba_cpu_texels.swap(accepted)",
            "world_device.rgba_mesh_generation=world_device.mesh_generation",
            "++world_device.rgba_uploads"):
            self.assertIn(marker,upload)
        draw=module[module.index("uint32_t render_offscreen_color_frame("):
                    module.index("// Independent opt-in camera projection")]
        for marker in (
            "const bool sampled=scene && rgba_flag",
            "world_device.rgba_mesh_generation!=world_device.mesh_generation",
            "VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT",
            "VK_IMAGE_TILING_LINEAR",
            "VK_IMAGE_USAGE_SAMPLED_BIT",
            "VK_MEMORY_PROPERTY_HOST_COHERENT_BIT",
            "vkGetImageSubresourceLayout(world_device.device,",
            "row_layout.rowPitch",
            "VK_IMAGE_LAYOUT_PREINITIALIZED",
            "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL",
            "VK_ACCESS_HOST_WRITE_BIT",
            "VK_ACCESS_SHADER_READ_BIT",
            "VK_SAMPLER_ADDRESS_MODE_REPEAT",
            "VK_FILTER_NEAREST",
            "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER",
            "vkUpdateDescriptorSets(world_device.device,1,&write,0,nullptr)",
            "vkCmdBindDescriptorSets(command,",
            "ogl_dev_world_rgba_sample_frag_spv",
            "const uint32_t shading_mode=atlas_hq ? 8u :",
            "VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT",
            "vkCmdDrawIndexed(command,world_device.mesh_triangle_count*3u,",
        ):
            self.assertIn(marker,draw)
        self.assertIn("static constexpr uint32_t ogl_dev_world_rgba_sample_frag_spv[]",module)
        self.assertIn("VkImage texture_image=VK_NULL_HANDLE;",module)
        self.assertIn("VkDescriptorSetLayout texture_set_layout=VK_NULL_HANDLE;",module)
        self.assertIn("VkSampler texture_sampler=VK_NULL_HANDLE;",module)
        self.assertIn("void clear_rgba_image()",module)
        for marker in (
            "P05 Vulkan real VkImage sampler:",
            "rgba_upload(red_green_rgba,2,2)==1",
            "rgba_upload(green_red_rgba,2,2)==1",
            "rgba_upload(max_rgba.data(),64,64)==1",
            "rgba_upload(translucent_rgba,1,1)==0",
            "rgba_upload(nullptr,0,0)==1",
            "rgba_mesh_generation()==tex_mesh",
        ):
            self.assertIn(marker,native)
        self.assertNotIn("vkQueuePresentKHR(",draw)
        self.assertNotIn("SDL_",draw)

    def test_p05_four_material_atlas_gpu_and_real_opensim_source(self):
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        for key in ("ogl_rtx_face_material_slot",
                    "ogl_vulkan_world_upload_material_slots",
                    "ogl_vulkan_world_material_slot_mesh_generation",
                    "ogl_vulkan_world_material_slot_uploads"):
            self.assertIn(key,api)
            self.assertIn(key,module)
        for key in ("struct OGLRgbaTileView",
                    "inline bool ogl_rtx_pack_rgba_atlas_2x2(",
                    "atlas.clear();",
                    "tile_count==0 || tile_count>4",
                    "tile.width>64 || tile.height>64",
                    "tile.rgba[pixel]!=255u",
                    "std::vector<std::uint8_t> packed(64u*64u*4u,255u)",
                    "atlas.swap(packed)",
                    "const float *face_material_slots",
                    "unsigned atlas_tile_count"):
            self.assertIn(key,header)
        for key in ("OGL_VULKAN_WORLD_MATERIAL_ATLAS_PROBE",
                    "OGL_VULKAN_LIVE_MATERIAL_ATLAS_SOURCE",
                    "const bool atlas_scan=rgba_scan",
                    "bool atlas_valid=atlas_scan && triangles.empty()",
                    "std::array<LLPointer<LLViewerFetchedTexture>,16> atlas_images",
                    "face_material_slots.reserve(ogl_triangle_budget)",
                    "if (!triangles.empty()) {",
                    "homogeneous_rgba=false;",
                    "atlas_valid=false;",
                    "while (slot<atlas_tile_count",
                    "atlas_images[atlas_tile_count]=face_image",
                    "if (atlas_tile_count<atlas_capacity)",
                    "face_material_slots.push_back(",
                    "std::array<std::vector<std::uint8_t>,16> rgba_tiles;",
                    "LLImageDataSharedLock lock(raw.get());",
                    "ogl_rtx_thumbnail_opaque_rgba8(",
                    "ogl_rtx_pack_rgba_atlas_2x2(",
                    "image_w=atlas_hq_scan ? 256u : 64u;",
                    "image_h=atlas_hq_scan ? 256u : 64u;",
                    "face_material_slots.front().slot : nullptr"):
            self.assertIn(key,pipeline)
        for key in ("ogl_vulkan_world_upload_material_slots",
                    "world_material_slot_generation()==world_mesh_generation()",
                    "live_deferred_material_slot_count==count",
                    "live_deferred_material_tile_count<=\n                    (atlas16_allowed ? 16u : 4u)",
                    "world_upload_material_slots(nullptr,0,0)",
                    "live_deferred_material_slots=face_material_slots ?",
                    "live_deferred_material_slot_count=0;",
                    "live_deferred_material_tile_count=0;"):
            self.assertIn(key,host)
        upload=module[module.index("int upload_world_material_slots("):
                      module.index("// First real VkImage+VkSampler source ABI:")]
        for key in ("OGL_VULKAN_WORLD_MATERIAL_ATLAS_PROBE",
                    "count!=world_device.mesh_triangle_count",
                    "tile_count==0 || tile_count>",
                    "std::isfinite(value)",
                    "value>=float(tile_count)",
                    "std::floor(value)!=value",
                    "slots[t].slot[v]!=value",
                    "upload_world_attribute_bytes(slots,",
                    "world_device.material_slot_mesh_generation=world_device.mesh_generation",
                    "++world_device.material_slot_uploads"):
            self.assertIn(key,upload)
        draw=module[module.index("uint32_t render_offscreen_color_frame("):
                    module.index("// Independent opt-in camera projection")]
        for key in ("const uint32_t shading_mode=atlas_hq ? 8u :",
                    "ogl_dev_world_material_atlas_vert_spv",
                    "ogl_dev_world_material_atlas_frag_spv",
                    "VK_FORMAT_R32_SFLOAT",
                    "world_device.material_slot_vertex_count!=world_device.mesh_triangle_count*3u",
                    "world_device.rgba_width!=(atlas_hq ? 256u : 64u)",
                    "vkCmdBindVertexBuffers(command,0,4,buffers,offsets)",
                    "world_device.mesh_material_slot_vertices"):
            self.assertIn(key,draw)
        for key in ("P05 4-material CPU atlas:",
                    "P05 Vulkan 4-material real VkImage atlas GPU:",
                    "slot_upload(&bad_fraction,1,4)==0",
                    "slot_upload(&bad_mixed,1,4)==0",
                    "slot_upload(&slot3,1,3)==0",
                    "slot_upload(&slot0,2,4)==0",
                    "offscreen_world_frames()==offscreen_start+40",
                    "P05 simultaneous two-material atlas GPU:",
                    "P05 per-face atlas repeat in one GPU frame:",
                    "offscreen_world_frames()==offscreen_start+42",
                    "slot_mesh_generation()==0",
                    "rgba_mesh_generation()==0",
                    "offscreen_mean_red()>10u",
                    "offscreen_mean_green()>10u",
                    "mesh_uploads()==atlas_xyz_uploads"):
            self.assertIn(key,native)
        self.assertNotIn("readbackRawImage()",pipeline)
        self.assertNotIn("forceToSaveRawImage(",pipeline)
        self.assertNotIn("vkQueuePresentKHR(",draw)

    def test_p05_sixteen_material_vulkan_atlas_and_legacy_fallback(self):
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        self.assertIn("OGL_VULKAN_WORLD_MATERIAL_ATLAS_16_PROBE=1",api)
        for marker in (
            "inline bool ogl_rtx_pack_rgba_atlas_2x2(",
            "inline bool ogl_rtx_pack_rgba_atlas_4x4(",
            "tile_count==0 || tile_count>16u",
            "tile.width>64u || tile.height>64u",
            "tile.rgba[i]!=255u",
            "std::vector<std::uint8_t> packed(64u*64u*4u,255u)",
            "const auto dx=(slot%4u)*16u,dy=(slot/4u)*16u;",
            "atlas.swap(packed);"):
            self.assertIn(marker,header)
        for marker in (
            '"OGL_VULKAN_WORLD_MATERIAL_ATLAS_16_PROBE"',
            '"OGL_VULKAN_LIVE_MATERIAL_ATLAS_16_SOURCE"',
            "const bool atlas16_scan=atlas_scan && atlas16_flag &&",
            "const std::uint32_t atlas_capacity=atlas16_scan ? 16u : 4u;",
            "std::array<LLPointer<LLViewerFetchedTexture>,16> atlas_images",
            "if (atlas_tile_count<atlas_capacity)",
            "std::array<OGLRgbaTileView,16> tile_views{};",
            "ogl_rtx_pack_rgba_atlas_4x4(",
            "ogl_rtx_pack_rgba_atlas_2x2(",
            "atlas16_scan ?",
            "LLImageDataSharedLock lock(raw.get());"):
            self.assertIn(marker,pipeline)
        for marker in (
            "const bool atlas16_allowed=atlas16_requested && atlas_needed &&",
            "if (uploaded && count>0 && atlas16_requested && !atlas16_allowed)",
            "(atlas16_allowed ? 16u : 4u)",
            "world_upload_material_slots(nullptr,0,0)",
            "world_rgba_mesh_generation()==world_mesh_generation()"):
            self.assertIn(marker,host)
        for marker in (
            "ogl_dev_world_material_atlas_16_frag_spv",
            "const bool atlas16=scene && atlas16_flag &&",
            "if (atlas16 && !atlas) return 0;",
            "material_slot_count>(atlas16 ? 16u : 4u)",
            "const uint32_t shading_mode=atlas_hq ? 8u :",
            "shading_mode>=6u",
            "enabled(\"OGL_VULKAN_WORLD_MATERIAL_ATLAS_16_PROBE\") ? 16u : 4u",
            "world_device.material_slot_mesh_generation=world_device.mesh_generation",
            "world_device.rgba_mesh_generation=world_device.mesh_generation"):
            self.assertIn(marker,module)
        for marker in (
            "P05 16-material CPU atlas:",
            "P05 16-material real Vulkan atlas GPU:",
            "samples16[slot]={red_channel,green_channel};",
            "slot_upload(first16_pair,2,17)==0",
            "slot_upload(&bad16_slot,1,16)==0",
            "slot_upload(slot16_pair,2,16)==1",
            "atlas16_frames_before+17u",
            "offscreen_cache_reuses()==atlas16_reuses_before+16u",
            "slot_upload(slot16_pair,2,16)==0",
            "const bool legacy16_restored=legacy16_denied &&",
            "offscreen_world_frames()==atlas16_frames_before+18u"):
            self.assertIn(marker,native)
        self.assertNotIn("readbackRawImage()",pipeline)
        self.assertNotIn("forceToSaveRawImage(",pipeline)
        self.assertNotIn("vkQueuePresentKHR(",
            module[module.index("uint32_t render_offscreen_color_frame("):
                   module.index("// Independent opt-in camera projection")])

    def test_p05_xlib_wsi_rejects_mixed_extent_sentinals_and_image_caps(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        begin=module.index("bool attach_world_xlib_window(")
        end=module.index("// Passive WSI attach never acquires or presents",begin)
        attach=module[begin:end]
        for marker in (
            "!caps.minImageCount || caps.minImageCount>16",
            "!caps.maxImageArrayLayers",
            "((caps.currentExtent.width==UINT32_MAX) !=",
            "(caps.currentExtent.height==UINT32_MAX))",
            "world_device.detach_window();",
            "VkSwapchainCreateInfoKHR swap{"):
            self.assertIn(marker,attach)
        self.assertLess(attach.index("!caps.minImageCount ||"),
                        attach.index("VkSwapchainCreateInfoKHR swap{"))
        self.assertLess(attach.index("caps.currentExtent.height==UINT32_MAX"),
                        attach.index("VkExtent2D extent=caps.currentExtent;"))

    def test_p05_xlib_wsi_rejects_undefined_in_multi_format_surface(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        begin=module.index("bool attach_world_xlib_window(")
        end=module.index("// Passive WSI attach never acquires or presents",begin)
        attach=module[begin:end]
        for expected in (
            "[expected_formats](const VkSurfaceFormatKHR& entry)",
            "entry.format==VK_FORMAT_UNDEFINED &&",
            "(expected_formats!=1 ||",
            "entry.colorSpace!=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR",
            "if (expected_formats==1 && format.format==VK_FORMAT_UNDEFINED)",
            "format={VK_FORMAT_B8G8R8A8_UNORM,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};"):
            self.assertIn(expected,attach)
        self.assertLess(attach.index("expected_formats!=1"),
                        attach.index("VkSurfaceFormatKHR format=formats.front();"))
        self.assertLess(attach.index("VkSurfaceFormatKHR format=formats.front();"),
                        attach.index("VkSwapchainCreateInfoKHR swap{"))

    def test_p05_xlib_wsi_rejects_invalid_surface_extents_and_transforms(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        begin=module.index("bool attach_world_xlib_window(")
        end=module.index("// Passive WSI attach never acquires or presents",begin)
        attach=module[begin:end]
        for marker in (
            "!caps.minImageExtent.width || !caps.minImageExtent.height",
            "caps.minImageExtent.width>caps.maxImageExtent.width",
            "caps.minImageExtent.height>caps.maxImageExtent.height",
            "!(caps.supportedTransforms&caps.currentTransform)",
            "caps.maxImageCount<caps.minImageCount",
            "extent.width<caps.minImageExtent.width",
            "extent.height<caps.minImageExtent.height",
            "extent.width>caps.maxImageExtent.width",
            "extent.height>caps.maxImageExtent.height",
            "world_device.detach_window();",
        ):
            self.assertIn(marker,attach)
        self.assertLess(attach.index("!(caps.supportedTransforms&caps.currentTransform)"),
                        attach.index("VkSwapchainCreateInfoKHR swap{"))
        self.assertLess(attach.index("extent.width<caps.minImageExtent.width"),
                        attach.index("VkSwapchainCreateInfoKHR swap{"))

    def test_p05_xlib_wsi_validates_second_surface_capability_enumerations(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        begin=module.index("bool attach_world_xlib_window(")
        end=module.index("// Passive WSI attach never acquires or presents",begin)
        attach=module[begin:end]
        for marker in (
            "const uint32_t expected_formats=count;",
            "std::vector<VkSurfaceFormatKHR> formats(expected_formats);",
            "&count,formats.data())!=VK_SUCCESS",
            "count!=expected_formats",
            "const uint32_t expected_modes=count;",
            "std::vector<VkPresentModeKHR> modes(expected_modes);",
            "&count,modes.data())!=VK_SUCCESS",
            "count!=expected_modes",
            "VK_PRESENT_MODE_FIFO_KHR",
            "world_device.detach_window();",
            "world_device.swapchain_probe_ready=true;"):
            self.assertIn(marker,attach)
        self.assertLess(attach.index("count!=expected_formats"),
                        attach.index("VkSurfaceFormatKHR format=formats.front()"))
        self.assertLess(attach.index("count!=expected_modes"),
                        attach.index("world_device.swapchain_probe_ready=true;"))
        self.assertNotIn("vkQueuePresentKHR(",attach)

    def test_p05_native_frame_limit_survives_mid_swapchain_flag_changes(self):
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        self.assertIn('for (uint32_t i=0;i<persistent_frames && frames_ok;++i) {',native)
        self.assertIn('for (uint32_t frame=0;frame<persistent_frames && frames;++frame) {',native)
        self.assertIn('if (i==8) {',native)
        self.assertIn('if (frame==8) {',native)
        self.assertIn('setenv("OGL_VULKAN_PERSISTENT_LONG_RUN_PROBE","1",1)',native)
        self.assertIn('unsetenv("OGL_VULKAN_PERSISTENT_LONG_RUN_PROBE")',native)
        self.assertIn('persistent_count()==persistent_frames',native)
        self.assertIn('persistent_clear(1)==0',native)

    def test_p05_xlib_geometry_module_requires_linked_x11(self):
        cmake=(ROOT/"ogl/modules/CMakeLists.txt").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        self.assertIn('target_link_libraries(ogl_rtx_module PRIVATE ${X11_LIBRARIES})',cmake)
        self.assertIn('target_compile_definitions(ogl_rtx_module PRIVATE OGL_XLIB_LINKED=1)',cmake)
        self.assertIn('defined(OGL_XLIB_LINKED) && __has_include(<X11/Xlib.h>)',module)

    def test_p05_renderer_ui_describes_actual_world_pipeline_limit(self):
        de=(ROOT/"indra/newview/skins/default/xui/de/panel_preferences_graphics1.xml").read_text()
        en=(ROOT/"indra/newview/skins/default/xui/en/panel_preferences_graphics1.xml").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        self.assertIn('Test-Swapchains funktionieren',de)
        self.assertIn('3D-Weltdarstellung nach dem Login fehlt',de)
        self.assertIn('persistent 3D world rendering after login is not implemented',en)
        self.assertIn('caps.vulkan_world_pipeline_complete = false;',host)
        self.assertIn('name="vulkan" label="Vulkan (P05 – noch nicht verfügbar)"',de)

    def test_p05_linux_system_font_directory_is_detected(self):
        font=(ROOT/"indra/llrender/llfontgl.cpp").read_text()
        self.assertIn('#include "llfile.h"',font)
        self.assertIn('#elif LL_LINUX',font)
        self.assertIn('"/usr/share/fonts/"',font)
        self.assertIn('"/usr/local/share/fonts/"',font)
        self.assertIn('LLFile::isdir(path)',font)
        self.assertIn('LL_WARNS_ONCE("Font")',font)

    def test_p05_persistent_live_present_queue_and_fifo_capabilities(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        a=module.index('uint32_t present_persistent_clear_frame(')
        b=module.index('// P05/3 foundation: exercise the REAL Vulkan graphics queue',a)
        frame=module[a:b]
        for marker in (
            'PFN_vkGetPhysicalDeviceSurfaceSupportKHR',
            'PFN_vkGetPhysicalDeviceSurfacePresentModesKHR',
            'queue_still_supported!=VK_TRUE',
            'live_mode_count>256',
            'live_mode_count!=expected_modes',
            'VK_PRESENT_MODE_FIFO_KHR)==active_modes.end()',
            'OGL_VULKAN_PERSISTENT_PRESENT_SUPPORT_LOST_TEST_X11',
            'std::strcmp(lost_native_qa,"1")==0'):
            self.assertIn(marker,frame)
        self.assertLess(frame.index('live_present_support(world_device.physical_device,'),
                        frame.index('vkAcquireNextImageKHR(world_device.device,'))
        for marker in (
            'setenv("OGL_VULKAN_PERSISTENT_PRESENT_SUPPORT_LOST_TEST_X11","1",1)',
            'unsetenv("OGL_VULKAN_PERSISTENT_PRESENT_SUPPORT_LOST_TEST_X11")',
            'persistent_lost_support=before_support_loss &&',
            'persistent_support_recovered=support_recovered_attach &&',
            '!persistent_lost_support || !persistent_support_recovered'):
            self.assertIn(marker,native)

    def test_p05_persistent_live_surface_format_and_composite_alpha(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        self.assertIn('VkColorSpaceKHR swapchain_color_space',module)
        self.assertIn('VkCompositeAlphaFlagBitsKHR swapchain_composite_alpha',module)
        self.assertIn('world_device.swapchain_color_space=format.colorSpace;',module)
        self.assertIn('world_device.swapchain_composite_alpha=composite;',module)
        a=module.index('uint32_t present_persistent_clear_frame(')
        b=module.index('// P05/3 foundation: exercise the REAL Vulkan graphics queue',a)
        fn=module[a:b]
        for marker in ('live_caps.supportedCompositeAlpha&world_device.swapchain_composite_alpha',
                       'PFN_vkGetPhysicalDeviceSurfaceFormatsKHR',
                       'live_format_count>256',
                       'live_format_count!=expected_live_formats',
                       'available.format==world_device.swapchain_format',
                       'available.format==VK_FORMAT_UNDEFINED',
                       'available.colorSpace==world_device.swapchain_color_space'):
            self.assertIn(marker,fn)
        self.assertLess(fn.index('get_live_formats(world_device.physical_device'),
                        fn.index('vkAcquireNextImageKHR(world_device.device,'))

    def test_p05_suboptimal_present_requires_new_swapchain_attachment(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        a=module.index('uint32_t present_persistent_clear_frame(')
        b=module.index('// P05/3 foundation: exercise the REAL Vulkan graphics queue',a)
        frame=module[a:b]
        for marker in ('const bool acquire_suboptimal=acquired==VK_SUBOPTIMAL_KHR;',
                       'const VkResult presented=vkQueuePresentKHR(world_device.queue,&present);',
                       'OGL_VULKAN_PERSISTENT_SUBOPTIMAL_TEST_X11',
                       'std::strcmp(native_qa,"1")==0',
                       'acquire_suboptimal || presented==VK_SUBOPTIMAL_KHR ||',
                       'injected_suboptimal) p.blocked=true;'):
            self.assertIn(marker,frame)
        self.assertLess(frame.index('++world_device.persistent_total_presents;'),
                        frame.index('injected_suboptimal) p.blocked=true;'))
        for marker in ('setenv("OGL_VULKAN_PERSISTENT_SUBOPTIMAL_TEST_X11","1",1)',
                       'unsetenv("OGL_VULKAN_PERSISTENT_SUBOPTIMAL_TEST_X11")',
                       'persistent_suboptimal=suboptimal_frame &&',
                       'persistent_recovered=recovered_attach &&',
                       '!persistent_suboptimal || !persistent_recovered'):
            self.assertIn(marker,native)

    def test_p05_live_surface_capabilities_remain_compatible_with_swapchain(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        self.assertIn('VkSurfaceTransformFlagBitsKHR swapchain_transform',module)
        self.assertIn('world_device.swapchain_transform=caps.currentTransform;',module)
        self.assertIn('swapchain_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;',module)
        a=module.index('uint32_t present_persistent_clear_frame(')
        b=module.index('// P05/3 foundation: exercise the REAL Vulkan graphics queue',a)
        fn=module[a:b]
        for marker in (
            'p.image_count<live_caps.minImageCount',
            'p.image_count>live_caps.maxImageCount',
            'live_caps.maxImageArrayLayers',
            'live_caps.supportedTransforms&world_device.swapchain_transform',
            'live_caps.currentTransform!=world_device.swapchain_transform',
            'world_device.swapchain_extent.width<live_caps.minImageExtent.width',
            'world_device.swapchain_extent.height<live_caps.minImageExtent.height',
            'world_device.swapchain_extent.width>live_caps.maxImageExtent.width',
            'world_device.swapchain_extent.height>live_caps.maxImageExtent.height'):
            self.assertIn(marker,fn)
        self.assertLess(fn.index('p.image_count<live_caps.minImageCount'),
                        fn.index('vkAcquireNextImageKHR(world_device.device,'))

    def test_p05_unmapped_x11_drawable_blocks_present_until_reattach(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        a=module.index('uint32_t present_persistent_clear_frame(')
        b=module.index('// P05/3 foundation: exercise the REAL Vulkan graphics queue',a)
        frame=module[a:b]
        self.assertIn('drawable.map_state!=IsViewable',frame)
        self.assertLess(frame.index('drawable.map_state!=IsViewable'),
                        frame.index('vkAcquireNextImageKHR(world_device.device,'))
        for marker in ('XUnmapWindow(display,window);',
                       'unmap_attrs.map_state==IsUnmapped',
                       'const bool unmap_attach=window_attach(',
                       'persistent_unmapped=before_unmap && unmap_confirmed',
                       'XMapWindow(display,window);',
                       'const bool remap_attach=window_attach(',
                       'persistent_remapped=remap_attach &&',
                       '!persistent_unmapped || !persistent_remapped'):
            self.assertIn(marker,native)

    def test_p05_variable_extent_uses_borrowed_x11_drawable_geometry(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        self.assertIn('Display* borrowed_xlib_display = nullptr;',module)
        self.assertIn('Window borrowed_xlib_window = 0;',module)
        self.assertIn('borrowed_xlib_display = nullptr;',module)
        self.assertIn('borrowed_xlib_window = 0;',module)
        self.assertIn('world_device.borrowed_xlib_display=static_cast<Display*>(native_display);',module)
        self.assertIn('world_device.borrowed_xlib_window=static_cast<Window>(native_window);',module)
        begin=module.index('uint32_t present_persistent_clear_frame(')
        end=module.index('// P05/3 foundation: exercise the REAL Vulkan graphics queue',begin)
        frame=module[begin:end]
        for marker in ('XWindowAttributes drawable{};',
                       'XGetWindowAttributes(world_device.borrowed_xlib_display,',
                       'world_device.borrowed_xlib_window,&drawable)',
                       'static_cast<uint32_t>(drawable.width)!=world_device.swapchain_extent.width',
                       'static_cast<uint32_t>(drawable.height)!=world_device.swapchain_extent.height',
                       'p.blocked=true;'):
            self.assertIn(marker,frame)
        self.assertLess(frame.index('XGetWindowAttributes(world_device.borrowed_xlib_display,'),
                        frame.index('vkAcquireNextImageKHR(world_device.device,'))
        self.assertLess(frame.index('XGetWindowAttributes(world_device.borrowed_xlib_display,'),
                        frame.index('vkQueueSubmit(world_device.queue,'))

    def test_p05_x11_gpu_resize_before_detach_is_rejected(self):
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        self.assertIn('XResizeWindow(display,window,544,336);',native)
        self.assertIn('const bool stale_clear_rejected=resized_draw &&',native)
        self.assertIn('persistent_resized=resized_draw && stale_clear_rejected &&',native)
        self.assertIn('XResizeWindow(display,window,704,528);',native)
        self.assertIn('const bool stale_world_rejected=persistent_world_resize &&',native)
        self.assertIn('stale_world_rejected &&',native)
        self.assertIn('persistent_clear(1)==0 && persistent_count()==1;',native)

    def test_p05_persistent_present_rejects_stale_fixed_surface_extent(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        a=module.index('uint32_t present_persistent_clear_frame(')
        b=module.index('// P05/3 foundation: exercise the REAL Vulkan graphics queue',a)
        fn=module[a:b]
        for marker in (
            'PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR',
            '"vkGetPhysicalDeviceSurfaceCapabilitiesKHR"',
            'VkSurfaceCapabilitiesKHR live_caps{};',
            'live_caps.currentExtent.width==UINT32_MAX',
            'live_caps.currentExtent.height==UINT32_MAX',
            'live_caps.currentExtent.width!=world_device.swapchain_extent.width',
            'live_caps.currentExtent.height!=world_device.swapchain_extent.height',
            'p.blocked=true;'):
            self.assertIn(marker,fn)
        self.assertLess(fn.index('get_live_caps(world_device.physical_device'),
                        fn.index('vkAcquireNextImageKHR(world_device.device,'))
        self.assertLess(fn.index('get_live_caps(world_device.physical_device'),
                        fn.index('vkQueueSubmit(world_device.queue,'))

    def test_p05_persistent_frame_ceiling_is_frozen_per_swapchain(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        structure=module[module.index("struct OglPersistentClearFrames {"):
                         module.index("struct OglOffscreenWorldCache {")]
        self.assertIn('uint32_t max_frames = 16;',structure)
        begin=module.index('uint32_t present_persistent_clear_frame(')
        end=module.index('// P05/3 foundation: exercise the REAL Vulkan graphics queue',begin)
        frame=module[begin:end]
        self.assertIn('p.initialized && p.frame_count>=p.max_frames',frame)
        self.assertIn('p.frame_count>=p.max_frames',frame)
        self.assertIn('p.max_frames=long_run',frame)
        self.assertIn('native_test && std::strcmp(native_test,"1")==0 ? 32u : 16u;',frame)
        self.assertLess(frame.index('p.max_frames=long_run'),frame.index('p.initialized=true;'))
        self.assertLess(frame.index('p.frame_count>=p.max_frames) return 0;'),
                        frame.index('vkAcquireNextImageKHR(world_device.device,'))

    def test_p05_persistent_long_run_is_separately_opted_in_and_bounded(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        a=module.index("uint32_t present_persistent_clear_frame(")
        b=module.index("// P05/3 foundation: exercise the REAL Vulkan graphics queue",a)
        frame=module[a:b]
        self.assertIn('std::getenv("OGL_VULKAN_PERSISTENT_LONG_RUN_PROBE")',frame)
        self.assertIn('std::strcmp(long_run,"1")==0',frame)
        self.assertIn('? 32u : 16u;',frame)
        self.assertIn('p.frame_count>=p.max_frames',frame)
        self.assertIn('p.max_frames=long_run',frame)
        self.assertIn('std::getenv("OGL_VULKAN_PRESENT_TEST_X11")',frame)
        self.assertIn('p.initialized=true;',frame)
        self.assertLess(frame.index('p.max_frames=long_run'),
                        frame.index('p.initialized=true;'))
        self.assertIn('vkQueuePresentKHR(world_device.queue,&present)',frame)
        self.assertNotIn('vkQueueWaitIdle(',frame)
        self.assertIn('OGL_VULKAN_PERSISTENT_LONG_RUN_TEST_X11',native)
        self.assertIn('const uint32_t persistent_frames=long_run ? 32u : 16u;',native)
        self.assertIn('i<persistent_frames && frames_ok',native)
        self.assertIn('frame<persistent_frames && frames',native)
        self.assertIn('persistent_count()==persistent_frames',native)
        self.assertIn('if (long_run) unsetenv("OGL_VULKAN_PERSISTENT_LONG_RUN_PROBE")',native)

    def test_p05_persistent_wsi_rejects_incomplete_image_enumeration(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        begin=module.index("uint32_t present_persistent_clear_frame(")
        end=module.index("// P05/3 foundation: exercise the REAL Vulkan graphics queue",begin)
        frame=module[begin:end]
        required=(
            "const uint32_t expected_count=count;",
            "std::vector<VkImage> images(expected_count,VK_NULL_HANDLE);",
            "&count,images.data())!=VK_SUCCESS",
            "count!=expected_count",
            "std::any_of(images.begin(),images.end(),",
            "return image==VK_NULL_HANDLE;",
            "p.image_count=expected_count;",
            "world_device.release_persistent_frames();",
            "vkAcquireNextImageKHR(world_device.device,",
            "vkQueuePresentKHR(world_device.queue,&present)"
        )
        for marker in required:
            self.assertIn(marker,frame)
        self.assertLess(frame.index("count!=expected_count"),
                        frame.index("p.image_count=expected_count;"))
        self.assertLess(frame.index("p.image_count=expected_count;"),
                        frame.index("vkAcquireNextImageKHR(world_device.device,"))
        self.assertIn("if (!init()) {",frame)
        self.assertIn("return 0;",frame)

    def test_p05_wsi_requires_complete_second_swapchain_image_enumeration(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        start=module.index("bool attach_world_xlib_window(")
        end=module.index("// Passive WSI attach never acquires or presents",start)
        attach=module[start:end]
        for marker in (
            "const uint32_t expected_count=count;",
            "std::vector<VkImage> swapchain_images(expected_count,VK_NULL_HANDLE);",
            "&count,swapchain_images.data())!=VK_SUCCESS",
            "count!=expected_count",
            "std::any_of(swapchain_images.begin(),swapchain_images.end(),",
            "return image==VK_NULL_HANDLE;",
            "world_device.detach_window();",
            "world_device.swapchain_probe_ready=true;"):
            self.assertIn(marker,attach)
        self.assertLess(attach.index("count!=expected_count"),
                        attach.index("world_device.swapchain_probe_ready=true;"))
        # The GL viewer never transfers window/present ownership here.
        self.assertNotIn("vkQueuePresentKHR(",attach)
        self.assertNotIn("vkAcquireNextImageKHR(",attach)

    def test_p05_hq_16_material_vulkan_detail_and_exact_source_gates(self):
        api=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        for marker in (
            "OGL_VULKAN_WORLD_MATERIAL_ATLAS_HQ_PROBE=1",
            "OGL_VULKAN_LIVE_MATERIAL_ATLAS_HQ_SOURCE=1"):
            self.assertIn(marker,api)
        for marker in (
            "inline bool ogl_rtx_pack_rgba_atlas_4x4_hq(",
            "atlas.clear();",
            "tile_count>16u",
            "tile.width>64u || tile.height>64u",
            "tile.rgba[pixel]!=255u",
            "std::vector<std::uint8_t> packed(256u*256u*4u,255u)",
            "const auto dst_x=(slot%4u)*64u;",
            "const auto dst_y=(slot/4u)*64u;",
            "std::size_t(dst_y+y)*256u+dst_x+x",
            "atlas.swap(packed);"):
            self.assertIn(marker,header)
        for marker in (
            "const bool atlas_hq_scan=atlas16_scan && atlas_hq_flag &&",
            '"OGL_VULKAN_WORLD_MATERIAL_ATLAS_HQ_PROBE"',
            '"OGL_VULKAN_LIVE_MATERIAL_ATLAS_HQ_SOURCE"',
            "ogl_rtx_pack_rgba_atlas_4x4_hq(",
            "ogl_rtx_pack_rgba_atlas_4x4(",
            "ogl_rtx_pack_rgba_atlas_2x2(",
            "image_w=atlas_hq_scan ? 256u : 64u;",
            "image_h=atlas_hq_scan ? 256u : 64u;",
            "LLImageDataSharedLock lock(raw.get());"):
            self.assertIn(marker,pipeline)
        for marker in (
            "const bool atlas_hq_requested=atlas_hq_flag &&",
            "const bool atlas_hq_allowed=atlas_hq_requested && atlas16_allowed &&",
            "if (uploaded && count>0 && atlas_hq_requested && !atlas_hq_allowed)",
            "live_deferred_rgba_width<=(atlas_hq_allowed ? 256u : 64u)",
            "live_deferred_rgba_height<=(atlas_hq_allowed ? 256u : 64u)",
            "live_deferred_rgba_width==256u",
            "live_deferred_rgba_height==256u",
            "world_rgba_mesh_generation()==world_mesh_generation()"):
            self.assertIn(marker,host)
        for marker in (
            "ogl_dev_world_material_atlas_hq_frag_spv[]",
            "const bool atlas_hq=scene && hq_flag &&",
            "if (atlas_hq && !atlas16) return 0;",
            "material_slot_count>(atlas16 ? 16u : 4u)",
            "world_device.rgba_width!=(atlas_hq ? 256u : 64u)",
            "const uint32_t shading_mode=atlas_hq ? 8u :",
            "sizeof(ogl_dev_world_material_atlas_hq_frag_spv)",
            "ogl_dev_world_material_atlas_hq_frag_spv",
            "shading_mode>=6u",
            "const uint32_t max_source_edge=hq_requested ? 256u : 64u;",
            "width>max_source_edge || height>max_source_edge",
            "const size_t bytes=size_t(width)*height*4u; // bounded <=262144",
            "hq_requested && (!atlas_requested || !atlas16_requested)",
            "world_device.release_offscreen_world_cache();",
            "VK_IMAGE_TILING_LINEAR",
            "row_layout.rowPitch",
            "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER",
            "VK_FILTER_NEAREST"):
            self.assertIn(marker,module)
        for marker in (
            "P05 HQ 16-material CPU atlas:",
            "P05 Vulkan HQ 16-material sampled VkImage:",
            "hq_rgba.size()==256u*256u*4u",
            "rgba_upload(hq_rgba.data(),64,64)==0",
            "rgba_upload(hq_rgba.data(),257,256)==0",
            "rgba_upload(hq_rgba.data(),256,256)==1",
            "slot_upload(hq_two_faces,2,16)==1",
            "hq_stripes[at]=x%2u==0u ? 255u : 0u;",
            "const float u_red=8.5f/64.f, u_green=9.5f/64.f;",
            "offscreen_mean_red()>45u",
            "offscreen_mean_green()>45u",
            "const bool hq_no_leak=offscreen_world()==0;",
            "const bool hq_legacy16=hq_no_leak &&",
            "offscreen_world_frames()==hq_frame_start+21u"):
            self.assertIn(marker,native)
        self.assertNotIn("readbackRawImage()",pipeline)
        self.assertNotIn("forceToSaveRawImage(",pipeline)
        draw=module[module.index("uint32_t render_offscreen_color_frame("):
                    module.index("// Independent opt-in camera projection")]
        self.assertNotIn("vkQueuePresentKHR(",draw)

    def test_p05_deferred_source_provenance_receipts_fail_closed(self):
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        report=(ROOT/"ogl/scripts/ogl-test-report.py").read_text()
        self.assertIn("void ogl_rtx_submit_deferred_world_triangles(",header)
        self.assertIn("void ogl_rtx_submit_deferred_world_triangles(",host)
        self.assertIn("ogl_rtx_submit_deferred_world_triangles(",pipeline)
        self.assertIn("if (publish_geometry && !triangles.empty() &&",pipeline)
        self.assertIn("ogl_rtx_region && !current_region_id.isNull()",pipeline)
        a=host.index("void ogl_rtx_submit_deferred_world_triangles(")
        b=host.index("bool ogl_rtx_track_local_object(",a)
        wrapper=host[a:b]
        self.assertIn("live_deferred_capture_call=true;",wrapper)
        self.assertIn("ogl_rtx_submit_world_triangles(triangle_xyz,count);",wrapper)
        self.assertIn("live_deferred_capture_call=false;",wrapper)
        self.assertIn("!triangle_xyz || count==0",wrapper)
        a=host.index("bool ogl_firestorm_offscreen_gpu_sample(")
        b=host.index("bool ogl_same_world_camera_pose(",a)
        gpu=host[a:b]
        for token in ("!live_deferred_capture_valid",
                      "live_deferred_receipt_epoch!=live_world_region_epoch",
                      "live_deferred_mesh_generation!=world_mesh_generation()",
                      "world_input_ready()!=1",
                      "world_camera_mesh_generation()!=world_mesh_generation()",
                      '<< "P05 deferred GPU receipt: epoch="',
                      "++live_deferred_gpu_frames;",
                      "++live_deferred_camera_gpu_frames;"):
            self.assertIn(token,gpu)
        self.assertNotIn("SDL_",gpu)
        self.assertNotIn("vkQueuePresentKHR(",gpu)
        a=host.index("void ogl_rtx_submit_world_triangles(")
        b=host.index("void ogl_rtx_submit_deferred_world_triangles(",a)
        upload=host[a:b]
        self.assertIn("if (live_deferred_capture_call && tint_success &&",upload)
        self.assertIn("if (uploaded && count>0) {",upload)
        self.assertIn("live_deferred_capture_valid=true;",upload)
        self.assertIn("++live_deferred_receipts;",upload)
        self.assertIn('"P05 deferred capture receipt: epoch="',upload)
        self.assertIn("live_deferred_capture_valid=false;",upload)
        a=host.index("bool ogl_rtx_on_region_transition()")
        b=host.index("void ogl_rtx_submit_camera_sun(",a)
        region=host[a:b]
        self.assertIn("live_deferred_capture_valid=false;",region)
        self.assertIn("live_deferred_receipt_epoch=0;",region)
        self.assertIn("live_deferred_mesh_generation=0;",region)
        self.assertIn('"P05 deferred region boundary: epoch="',region)
        self.assertIn('"P05 deferred evidence session: begin=1;',host)
        self.assertNotIn("user_id",gpu)
        self.assertNotIn("region_uuid",gpu)
        self.assertIn("def p05_live_gpu_evidence(log_text: str):",report)
        self.assertIn("P05_CAPTURE = re.compile(",report)
        self.assertIn("P05_GPU = re.compile(",report)
        self.assertIn("P05 deferred Vulkan GPU evidence:",report)

    def test_p05_real_opensim_camera_only_gpu_ticks_after_region_validation(self):
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        header=(ROOT/"indra/newview/oglrtxhost.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        self.assertIn("void ogl_rtx_after_region_camera_tick();",header)
        self.assertIn("constexpr bool ogl_vulkan_camera_tick_due(std::uint64_t frame)",header)
        for boundary in ("!ogl_vulkan_camera_tick_due(0)",
                         "ogl_vulkan_camera_tick_due(1)",
                         "!ogl_vulkan_camera_tick_due(2)",
                         "!ogl_vulkan_camera_tick_due(120)",
                         "ogl_vulkan_camera_tick_due(121)",
                         "!ogl_vulkan_camera_tick_due(240)",
                         "ogl_vulkan_camera_tick_due(241)"):
            self.assertIn(boundary,header)
        self.assertIn("void ogl_rtx_after_region_camera_tick() {",host)
        self.assertIn("OGL_VULKAN_LIVE_CAMERA_TICK",host)
        self.assertIn("OGL_VULKAN_LIVE_WORLD_OFFSCREEN",host)
        self.assertIn("OGL_VULKAN_WORLD_OFFSCREEN_RASTER",host)
        self.assertIn("OGL_VULKAN_OFFSCREEN_FRAME_PROBE",host)
        self.assertIn("const bool accepted_gpu_camera=gpu_camera &&",host)
        self.assertIn("if (gpu_camera && !accepted_gpu_camera)",host)
        self.assertIn("world_inputs_live_logged=false; // stale camera",host)
        a=pipeline.index("ogl_rtx_submit_camera_sun(viewport.getWidth()")
        b=pipeline.index("if (current_region_id != ogl_rtx_last_region_id)")
        c=pipeline.index("ogl_rtx_after_region_camera_tick();")
        d=pipeline.index("ogl_rtx_submit_deferred_world_triangles(\n                        triangles.front().position")
        self.assertLess(a,b)
        self.assertLess(b,c)
        self.assertLess(c,d)
        self.assertLess(pipeline.index("ogl_rtx_on_region_transition();",b),c)
        self.assertIn("if (ogl_rtx_region && !current_region_id.isNull())",pipeline)
        start=host.index("void ogl_rtx_after_region_camera_tick() {")
        end=host.index("// The region UUID is compared",start)
        fn=host[start:end]
        for token in ("std::getenv(\"OGL_VULKAN_LIVE_CAMERA_TICK\")",
                      "ogl_live_offscreen_developer_enabled()",
                      "last_live_world_camera_valid",
                      "++live_world_camera_tick_calls;",
                      "constexpr std::uint64_t camera_interval=120;",
                      "ogl_vulkan_camera_tick_due(live_world_camera_tick_calls)",
                      "world_input_ready()",
                      "world_mesh_generation()==0",
                      "world_camera_mesh_generation()!=world_mesh_generation()",
                      "live_world_last_offscreen_mesh_generation==revision",
                      "live_world_last_offscreen_region_epoch==live_world_region_epoch",
                      "ogl_same_world_camera_pose(",
                      "++live_world_camera_tick_unchanged;",
                      "++live_world_camera_tick_attempts;",
                      'ogl_firestorm_offscreen_gpu_sample("camera-tick")',
                      "live_world_last_offscreen_pose=last_live_world_camera;"):
            self.assertIn(token,fn)
        self.assertNotIn("world_upload_geometry(",fn)
        self.assertNotIn("vkQueuePresentKHR(",fn)
        self.assertNotIn("SDL_",fn)
        cstart=host.index("bool ogl_same_world_camera_pose(")
        cend=host.index("// Must run AFTER",cstart)
        comparator=host[cstart:cend]
        for part in ("a.width==b.width","a.height==b.height",
                     "a.vertical_fov_radians==b.vertical_fov_radians",
                     "a.aspect_ratio==b.aspect_ratio",
                     "a.near_clip_m==b.near_clip_m",
                     "a.far_clip_m==b.far_clip_m",
                     "a.camera_position,b.camera_position",
                     "a.camera_forward,b.camera_forward",
                     "a.camera_right,b.camera_right",
                     "a.camera_up,b.camera_up"):
            self.assertIn(part,comparator)
        self.assertIn("OGL_VULKAN_WORLD_SUN_LIGHT_PROBE",comparator)
        self.assertIn("if (dynamic_sun &&",comparator)
        self.assertIn("std::memcmp(a.sun_direction,b.sun_direction,",comparator)
        self.assertIn("sizeof(a.sun_direction))!=0)",comparator)
        region=host.index("bool ogl_rtx_on_region_transition()")
        self.assertIn("live_world_camera_tick_calls=0;",host[region:region+900])
        self.assertIn("live_world_last_offscreen_pose_valid=false;",host[region:region+900])
        self.assertIn('ogl_firestorm_offscreen_gpu_sample("mesh-rebind")',host)
        self.assertIn("world_render_offscreen_scene()==1",host)
        self.assertIn("world_offscreen_scene_frames()==old_count+1",host)
        self.assertIn("camera.camera_position[1]=50.f;",native)
        self.assertIn("camera.camera_position[1]=20.f;",native)
        self.assertIn("static_mesh_generation=mesh_generation()",native)
        self.assertIn("mesh_uploads()==static_mesh_uploads",native)
        self.assertIn("mesh_retire_waits()==static_mesh_waits",native)
        self.assertIn("offscreen_cache_reuses()==offscreen_reuse0+4",native)
        # Inactive Vulkan never selects the product backend.
        self.assertIn("caps.vulkan_world_pipeline_complete = false;",host)

    def test_p05_native_multiframe_gpu_present_is_bounded_and_pixel_verified(self):
        module=(ROOT/"ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        header=(ROOT/"ogl/modules/include/ogl/ogl_rtx_api.h").read_text()
        native=(ROOT/"ogl/modules/rtx/frame_bridge_test.cpp").read_text()
        host=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
        self.assertIn("ogl_vulkan_world_exclusive_present_count",header)
        self.assertIn("return world_device.exclusive_present_count;",module)
        self.assertIn("exclusive_present_count = 0;",module)
        self.assertIn("const uint32_t sequence=world_device.exclusive_present_count",module)
        self.assertIn('std::getenv("OGL_VULKAN_EXCLUSIVE_FRAME_LOOP")',module)
        self.assertIn("constexpr uint32_t max_native_frames=16;",module)
        self.assertIn("world_device.exclusive_present_count>=max_native_frames",module)
        self.assertIn("if (success) ++world_device.exclusive_present_count;",module)
        self.assertIn("sequence%2",module)
        self.assertIn("vkQueueWaitIdle(world_device.queue)",module)
        self.assertIn("in_flight=true;",module)
        self.assertIn("const uint32_t overflow=exclusive_present(1)",native)
        self.assertIn("uint32_t completed=0;",native)
        self.assertIn("i<16",native)
        self.assertIn("exclusive_count()!=i+1",native)
        self.assertIn("XGetImage(display,window,320,180,1,1",native)
        self.assertIn("XGetPixel(image,0,0)",native)
        self.assertIn("image->red_mask",native)
        self.assertIn("image->green_mask",native)
        self.assertIn("image->blue_mask",native)
        self.assertIn("if (!valid) frame_sequence_valid=false;",native)
        self.assertIn("restored_frame=exclusive_present(1)",native)
        self.assertIn("restored_repeat=exclusive_present(1)",native)
        self.assertIn("restored_count!=1",native)
        self.assertIn("stopped_count!=0",native)
        self.assertNotIn("ogl_vulkan_world_present_exclusive_test_frame",host)
        self.assertNotIn("OGL_VULKAN_EXCLUSIVE_FRAME_LOOP",host)

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
        self.assertIn("if (publish_geometry && !triangles.empty() &&",pipeline)
        self.assertIn("bool ogl_vulkan_world_mesh_staging_enabled()",host)
        self.assertNotIn("ogl_vulkan_world_present_exclusive_test_frame",
                         (ROOT/"indra/newview/oglrtxhost.cpp").read_text())

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
        self.assertIn("if (!ogl_rtx_last_region_id.isNull() && !gpu_region_retired)", pipeline)
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
