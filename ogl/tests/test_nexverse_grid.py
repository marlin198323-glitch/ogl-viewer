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
        self.assertIn('Rejected login: HTTPS or explicit fixed-grid HTTP consent required', security)
        self.assertIn('consented_pinned_http', security)
        self.assertIn('gHTTPLoginApprovedThisSession', security)
        self.assertIn('isExactPinnedHTTPLogin(', security)
        self.assertIn('transition_back_to_login_panel("")', security)
        self.assertIn('gSavedSettings.setBOOL("AutoLogin", false)', security)
        self.assertIn('parsed.scheme() != "https"', security)
        self.assertLess(security.index('Rejected login: HTTPS or explicit fixed-grid HTTP consent required'),
                        security.index('login->connect(gUserCredential)'))
        self.assertIn('OGLInsecureGridLogin', security)
        self.assertIn('OGLHTTPLoginWarning', self.panel)
        self.assertIn('gHTTPLoginApprovedThisSession = true', self.panel)
        self.assertIn('getSelectedOption(notification, response) == 0', self.panel)
        self.assertIn('gHTTPLoginApprovedThisSession &&', security)

    def test_http_warning_has_explicit_translated_accept_cancel_buttons(self):
        import xml.etree.ElementTree as ET
        for language in ('en', 'de'):
            path = ROOT / f'indra/newview/skins/default/xui/{language}/notifications.xml'
            xml = ET.parse(path).getroot()
            matches = [e for e in xml.iter('notification')
                       if e.get('name') == 'OGLHTTPLoginWarning']
            self.assertEqual(len(matches), 1)
            buttons = matches[0].find('usetemplate')
            self.assertEqual(buttons.get('name'), 'okcancelbuttons')
            self.assertTrue(buttons.get('yestext'))
            self.assertTrue(buttons.get('notext'))
            self.assertIn('HTTP', matches[0].text or '')


if __name__ == "__main__":
    unittest.main()
