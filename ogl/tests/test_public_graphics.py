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

    def test_no_rt_hardware_preserves_opengl_fallback(self):
        module = (ROOT / "ogl/modules/rtx/ogl_rtx_module.cpp").read_text()
        self.assertIn('No suitable Vulkan ray tracing GPU found; OpenGL fallback.', module)
        host = (ROOT / "indra/newview/oglrtxhost.cpp").read_text()
        self.assertIn('OpenGL remains the active renderer', host)


if __name__ == "__main__":
    unittest.main()
