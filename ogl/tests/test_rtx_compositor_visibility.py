"""Confirm that the first in-world shadow path measures actual GL
fragments and does not confuse successful ray queries with visible pixels.
Optional GLSL parsing uses the developer's existing local glslang binary.
"""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
HOST=(ROOT/"indra/newview/oglrtxhost.cpp").read_text()
REPORT=(ROOT/"ogl/scripts/ogl-test-report.py").read_text()


class ShadowVisibilityTests(unittest.TestCase):
    def test_live_depth_not_just_vulkan_shadow_mask(self):
        self.assertIn("if (ray.r<0.5 || ray.g<0.5) discard;",HOST)
        self.assertIn("viewDistance(ray.b)",HOST)
        self.assertIn("viewDistance(live)",HOST)
        self.assertIn("clamp(reference*0.02,0.25,1.5)",HOST)
        self.assertIn("if (abs(now-reference)>tolerance) discard;",HOST)
        self.assertIn("if (live>=0.999999 || ray.b>=0.999999) discard;",HOST)

    def test_correct_lightmap_channels_and_actual_red_overlay(self):
        pipeline=(ROOT/"indra/newview/pipeline.cpp").read_text()
        sun=(ROOT/"indra/newview/app_settings/shaders/class2/deferred/sunLightF.glsl").read_text()
        soften=(ROOT/"indra/newview/app_settings/shaders/class3/deferred/softenLightF.glsl").read_text()
        self.assertIn("col.r = sampleDirectionalShadow",sun)
        self.assertIn("col.g = 1.0f",sun)
        self.assertIn("texture(lightMap, vary_fragcoord.xy).rg",soften)
        self.assertIn("glGetBooleanv(GL_COLOR_WRITEMASK,old_color_mask)",HOST)
        self.assertIn("glColorMask(GL_TRUE,",HOST)
        self.assertIn("diagnostic_overlay ? GL_TRUE : GL_FALSE",HOST)
        self.assertIn("glColorMask(old_color_mask[0],old_color_mask[1]",HOST)
        self.assertIn("if (!diagnostic_overlay &&",HOST)
        self.assertIn("OGL RTX shadow diagnostic color overlay:",HOST)
        self.assertIn("renderGeomPostDeferred(*LLViewerCamera::getInstance())",pipeline)
        self.assertIn("true /* diagnostic overlay into final 3D color */",pipeline)
        self.assertIn("OGL_RTX_SHADOW_VISUALIZE",pipeline)
        self.assertIn("RTX diagnostic RED overlay:",REPORT)
        self.assertIn("shadow diagnostic color overlay:",REPORT)

    def test_gl_query_counts_fragments_after_discard(self):
        self.assertIn("glBeginQuery(GL_SAMPLES_PASSED",HOST)
        self.assertIn("glEndQuery(GL_SAMPLES_PASSED)",HOST)
        self.assertIn("GL_QUERY_RESULT_AVAILABLE",HOST)
        self.assertIn("GL shadow pixel coverage: samples=",HOST)
        self.assertIn("OGL_RTX_SHADOW_VISUALIZE",HOST)
        self.assertIn("RTX OpenGL fragment coverage:",REPORT)
        self.assertIn("RTX visible-shadow blocker:",REPORT)

    def test_visual_mode_explicit_only(self):
        script=(ROOT/"ogl/scripts/run-rtx-dense-preview-test.sh").read_text()
        visual=(ROOT/"ogl/scripts/run-rtx-shadow-visual-test.sh").read_text()
        self.assertNotIn("OGL_RTX_SHADOW_VISUALIZE=1",script)
        self.assertIn("OGL_RTX_SHADOW_VISUALIZE=1",visual)
        self.assertIn("run-rtx-dense-preview-test.sh",visual)

    def test_fragment_shader_parses_if_glslang_available(self):
        compiler=shutil.which("glslangValidator")
        if not compiler:
            compiler=Path.home()/"Schreibtisch/OGL-Viewer/.ogl-tools/shader-compiler/extracted/usr/bin/glslangValidator"
        if not Path(compiler).is_file():
            self.skipTest("GLSL compiler not installed")
        match=re.search(
            r'static const char\* fragment_source=R"GLSL\((.*?)\)GLSL";',
            HOST,re.S)
        self.assertIsNotNone(match)
        with tempfile.TemporaryDirectory() as scratch:
            target=Path(scratch)/"ogl_rtx_visibility.frag"
            target.write_text(match.group(1))
            result=subprocess.run([str(compiler),"-S","frag",str(target)],
                                  capture_output=True,text=True,timeout=15)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
