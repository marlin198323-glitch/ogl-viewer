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
        self.assertIn('gHTTPLoginApprovedThisSession', self.values)
        self.assertIn('isExactPinnedHTTPLogin(', self.values)
        self.assertIn('transition_back_to_login_panel("")', security)
        self.assertIn('gSavedSettings.setBOOL("AutoLogin", false)', security)
        self.assertIn('parsed.scheme() != "https"', security)
        self.assertLess(security.index('Rejected login: HTTPS or explicit fixed-grid HTTP consent required'),
                        security.index('login->connect(gUserCredential)'))
        self.assertIn('OGLInsecureGridLogin', security)
        self.assertIn('OGLHTTPLoginWarning', self.panel)
        self.assertIn('gHTTPLoginApprovedThisSession = true', self.panel)
        self.assertIn('option == 0 || option == 1', self.panel)
        self.assertIn('if (option == 1)', self.panel)
        self.assertIn('gHTTPLoginApprovedThisSession = true', self.panel)
        self.assertIn('isApprovedPinnedHTTPLogin(', security)
        self.assertIn('OGLRememberPinnedHTTPLoginURI', security)
        self.assertIn('OGLRememberPinnedHTTPLoginURI', self.panel)

    def test_http_warning_has_explicit_translated_accept_cancel_buttons(self):
        import xml.etree.ElementTree as ET
        for language in ('en', 'de'):
            path = ROOT / f'indra/newview/skins/default/xui/{language}/notifications.xml'
            xml = ET.parse(path).getroot()
            matches = [e for e in xml.iter('notification')
                       if e.get('name') == 'OGLHTTPLoginWarning']
            self.assertEqual(len(matches), 1)
            form = matches[0].find('form')
            self.assertIsNotNone(form)
            buttons = list(form.findall('button'))
            self.assertEqual([int(b.get('index')) for b in buttons], [0, 1, 2])
            self.assertEqual([b.get('name') for b in buttons],
                             ['OGLHTTPConnectOnce', 'OGLHTTPRemember', 'OGLHTTPCancel'])
            self.assertTrue(all(b.get('text') for b in buttons))
            self.assertIn('HTTP', matches[0].text or '')

    def test_remembered_consent_is_opt_in_scoped_and_reversible(self):
        import xml.etree.ElementTree as ET
        src = ROOT / 'indra/newview/app_settings/settings.xml'
        root = ET.parse(src).getroot()
        control = root.find('map')
        entries = {}
        children = list(control)
        for i in range(0, len(children)-1, 2):
            if children[i].tag == 'key':
                entries[children[i].text] = children[i+1]
        stored = entries['OGLRememberPinnedHTTPLoginURI']
        settings = list(stored)
        fields = {settings[i].text: settings[i+1]
                  for i in range(0,len(settings)-1,2)}
        self.assertEqual(fields['Persist'].text, '1')
        self.assertEqual(fields['Type'].text, 'String')
        self.assertEqual(fields['Value'].text, None)
        self.assertIn('rememberedURI == kLoginURI', self.values)
        self.assertIn('isExactPinnedHTTPLogin(selectedGrid, loginURIs)', self.values)
        self.assertNotIn('gSavedSettings.setString("OGLRememberPinnedHTTPLoginURI",',
                         self.startup)



if __name__ == "__main__":
    unittest.main()
