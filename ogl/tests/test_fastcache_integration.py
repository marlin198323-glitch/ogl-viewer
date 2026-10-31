"""Opt-in fast-cache safety invariants, cross-platform and default off."""
from pathlib import Path
import unittest
R=Path(__file__).resolve().parents[2]
V=(R/"indra/newview/lltexturecache.cpp").read_text()
C=(R/"indra/newview/CMakeLists.txt").read_text()
M=(R/"ogl/modules/CMakeLists.txt").read_text()

class CacheIntegrationTests(unittest.TestCase):
    def test_is_opt_in_and_uses_valid_data_only(self):
        self.assertIn('std::getenv("OGL_FASTCACHE_SKIP_IDENTICAL")',V)
        self.assertIn("value[0]=='1'",V)
        self.assertIn("if (skip_identical && copy_size>0",V)
        self.assertIn("bytes==TEXTURE_FAST_CACHE_ENTRY_SIZE",V)
        self.assertIn("ogl::same_fastcache_entry(",V)
        self.assertIn("comparison=libc_memcmp",V)
        helper=(R/"ogl/modules/include/ogl/ogl_fastcache_compare.h").read_text()
        self.assertIn("std::memcmp(old_entry+header_size",helper)
        self.assertIn("static_cast<size_t>(copy_size)",V)
        self.assertIn("if (!already_present)",V)
    def test_buffer_preparation_is_within_lock(self):
        section=V[V.index("bool LLTextureCache::writeToFastCache("):
                  V.index("void LLTextureCache::openFastCache(")]
        self.assertLess(section.index("LLMutexLock cache_lock(&mFastCacheMutex)"),
                        section.index("memcpy(mFastCachePadBuffer, &w"))
        self.assertLess(section.index("memcpy(mFastCachePadBuffer, &w"),
                        section.index("mFastCachep->write("))
    def test_same_backend_in_viewer_and_standalone(self):
        build=(R/"ogl/modules/cpu/ogl_cpu_buffer_targets.cmake").read_text()
        self.assertIn("function(ogl_add_cpu_buffer_target target)",build)
        self.assertIn("ogl_add_cpu_buffer_target(ogl_viewer_cpu_buffer)",C)
        self.assertIn("ogl_add_cpu_buffer_target(ogl_cpu_buffer)",M)
        self.assertIn("ogl_viewer_cpu_buffer",C)
        self.assertIn("OGL_CPU_SINGLE_ARCH",build)
        self.assertNotIn("-march=native",build)
    def test_report_never_claims_speedup_from_skip_count(self):
        source=(R/"ogl/scripts/ogl-test-report.py").read_text()
        self.assertIn("OGL fastcache identical-write check:",source)
        self.assertIn("not a measured speedup",source)

    def test_defaults_and_cache_format_unchanged(self):
        self.assertIn("TEXTURE_FAST_CACHE_ENTRY_SIZE",V)
        self.assertNotIn("OGL_FASTCACHE_SKIP_IDENTICAL",
                         (R/"indra/newview/app_settings/settings.xml").read_text())
        self.assertIn("ogl_fastcache_microbenchmark",M)
if __name__=="__main__":unittest.main()
