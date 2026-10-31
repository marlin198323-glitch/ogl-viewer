"""Contracts for the pinned Stadt NexVerse single-grid OGL build.

Source contract tests complement (but cannot replace) a live viewer run.
No real credentials are used.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class StadtNexVerseGridTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.grid = (ROOT / "indra/newview/fsgridhandler.cpp").read_text()
        cls.panel = (ROOT / "indra/newview/fspanellogin.cpp").read_text()
        cls.startup = (ROOT / "indra/newview/llstartup.cpp").read_text()
        cls.values = (ROOT / "indra/newview/oglnexversegrid.h").read_text()

    def test_exact_grid_entry_and_welcome(self):
        for value in (
            '"Stadt NexVerse"', '"hg.stadt-nexverse.de:80"',
            '"http://hg.stadt-nexverse.de:80/"',
            '"https://stadt-nexverse.de/service/welcome"',
            '"hop://hg.stadt-nexverse.de:80/"',
        ):
            self.assertIn(value, self.values)

    def test_grid_is_pinned_and_remote_lists_skipped(self):
        self.assertIn('if (APP_NAME != "OGL")', self.grid)
        self.assertIn('setGridChoice(OGLNexVerseGrid::kKey)', self.grid)
        self.assertIn('Rejected unrelated grid registration', self.grid)
        self.assertIn('Rejected grid switch outside Stadt NexVerse', self.grid)
        self.assertIn('OGLNexVerseGrid::kLoginURI', self.grid)
        self.assertIn('if (APP_NAME == "OGL") server_choice_combo->setEnabled(false)', self.panel)
        self.assertIn('grid_builder_text->setVisible(APP_NAME != "OGL")', self.panel)

    def test_secure_login_on_all_paths(self):
        security = self.startup
        self.assertIn('STATE_LOGIN_AUTH_INIT', security)
        self.assertIn('Blocked all login methods: secure HTTPS endpoint', security)
        self.assertIn('transition_back_to_login_panel("")', security)
        self.assertIn('gSavedSettings.setBOOL("AutoLogin", false)', security)
        self.assertIn('parsed.scheme() != "https"', security)
        self.assertLess(security.index('Blocked all login methods: secure HTTPS endpoint'),
                        security.index('login->connect(gUserCredential)'))
        self.assertIn('OGLInsecureGridLogin', security)


if __name__ == "__main__":
    unittest.main()
