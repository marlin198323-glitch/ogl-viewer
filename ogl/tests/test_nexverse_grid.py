"""Contracts for the two hard-coded NexVerse login-grid selections.

No credentials or network calls. The Beta entry is selectable but MUST NOT
be treated as an OpenSim/XML-RPC authentication endpoint until verified.
"""
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]


class StadtNexVerseGridTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.grid = (ROOT / "indra/newview/fsgridhandler.cpp").read_text()
        cls.panel = (ROOT / "indra/newview/fspanellogin.cpp").read_text()
        cls.startup = (ROOT / "indra/newview/llstartup.cpp").read_text()
        cls.values = (ROOT / "indra/newview/oglnexversegrid.h").read_text()

    def test_both_fixed_grid_keys_and_addresses(self):
        for value in (
            '"Stadt NexVerse"', '"hg.stadt-nexverse.de:80"',
            '"http://hg.stadt-nexverse.de:80/"',
            '"https://stadt-nexverse.de/service/welcome"',
            '"hop://hg.stadt-nexverse.de:80/"',
            '"Stadt NexVerse Beta"', '"beta.stadt-nexverse.de"',
            '"https://beta.stadt-nexverse.de/"',
        ):
            self.assertIn(value, self.values)
        self.assertIn("isFixedGrid(", self.values)
        self.assertIn("isBetaGrid(", self.values)

    def test_both_are_registered_as_pinned_system_grids(self):
        self.assertIn('if (APP_NAME != "OGL")', self.grid)
        self.assertIn('setGridChoice(selected);', self.grid)
        self.assertIn('OGLNexVerseGrid::kKey', self.grid)
        self.assertIn('OGLNexVerseGrid::kBetaKey', self.grid)
        self.assertIn('OGLNexVerseGrid::kBetaName', self.grid)
        self.assertIn('OGLNexVerseGrid::kBetaLoginURI', self.grid)
        self.assertIn('addSystemGrid(OGLNexVerseGrid::kBetaName', self.grid)
        self.assertIn("state != SYSTEM", self.grid)
        self.assertIn("Rejected unrelated grid registration or override", self.grid)
        self.assertIn('OGLNexVerseGrid::isFixedGrid(grid)', self.grid)
        self.assertIn("CurrentGrid", self.grid)
        # Only a previously selected BETA is restored; unrelated saved values
        # cannot redirect credentials to another host.
        self.assertIn('OGLNexVerseGrid::isBetaGrid(last_grid)', self.grid)
        self.assertIn('OGLNexVerseGrid::kKey;', self.grid)

    def test_grid_dropdown_enabled_and_always_visible(self):
        self.assertIn('server_choice_combo->setEnabled(true)', self.panel)
        self.assertIn('(APP_NAME == "OGL") || gSavedSettings.getBOOL("ForceShowGrid")', self.panel)
        self.assertIn('server_choice_combo->add(grid_choice->second, grid_choice->first)', self.panel)
        self.assertIn('server_choice_combo->selectFirstItem()', self.panel)
        self.assertIn("Rejected unlisted grid selection.", self.panel)
        self.assertIn("OGLNexVerseGrid::isFixedGrid(server_combo_val.asString())", self.panel)
        self.assertIn('grid_builder_text->setVisible(APP_NAME != "OGL")', self.panel)

    def test_ogl_dropdown_contains_exactly_two_nonduplicated_grids(self):
        panel = self.panel
        marker = panel.index('// Exactly two pinned OGL worlds.')
        branch_end = panel.index('#if OPENSIM && !SINGLEGRID', marker)
        branch = panel[marker:branch_end]
        self.assertIn('if (APP_NAME == "OGL")', branch)
        self.assertEqual(branch.count('server_choice_combo->add('), 2)
        self.assertIn('server_choice_combo->add(OGLNexVerseGrid::kName, OGLNexVerseGrid::kKey)', branch)
        self.assertIn('server_choice_combo->add(OGLNexVerseGrid::kBetaName, OGLNexVerseGrid::kBetaKey)', branch)
        self.assertIn('OGLNexVerseGrid::isBetaGrid(', branch)
        self.assertIn('server_choice_combo->setSelectedByValue(', branch)
        self.assertIn('update_grid_help();', branch)
        self.assertIn('return;', branch)

    def test_beta_credentials_never_sent_from_login_button(self):
        blocked = self.panel.index('Blocked beta authentication: protocol and TLS not verified.')
        credentials = self.panel.index('std::string username = sInstance->getChild<LLUICtrl>("username_combo")', blocked)
        self.assertLess(blocked, credentials)
        self.assertIn('OGLNexVerseGrid::isBetaGrid(LLGridManager::getInstance()->getGrid())', self.panel)
        self.assertIn('LLTrans::getString("OGLBetaLoginUnavailable")', self.panel)

    def test_beta_credentials_never_sent_from_auto_login(self):
        guard = self.startup.index('Blocked beta authentication: no verified protocol/TLS adapter.')
        call = self.startup.index('login->connect(gUserCredential)')
        self.assertLess(guard, call)
        self.assertIn('OGLNexVerseGrid::isBetaGrid(LLGridManager::getInstance()->getGrid())', self.startup)
        self.assertIn('gSavedSettings.setBOOL("AutoLogin", false)', self.startup)
        self.assertIn('transition_back_to_login_panel("")', self.startup)

    def test_public_http_exception_remains_exact_and_opt_in(self):
        self.assertIn('gHTTPLoginApprovedThisSession', self.values)
        self.assertIn('isExactPinnedHTTPLogin(', self.values)
        self.assertIn('rememberedURI == kLoginURI', self.values)
        self.assertIn('isApprovedPinnedHTTPLogin(', self.startup)
        self.assertIn('consented_pinned_http', self.startup)
        self.assertIn('OGLHTTPLoginWarning', self.panel)
        self.assertIn('OGLRememberPinnedHTTPLoginURI', self.panel)
        self.assertIn('option == 0 || option == 1', self.panel)
        self.assertIn('if (option == 1)', self.panel)
        self.assertIn('Rejected login: HTTPS or explicit fixed-grid HTTP consent required', self.startup)
        self.assertLess(self.startup.index('Rejected login: HTTPS or explicit fixed-grid HTTP consent required'),
                        self.startup.index('login->connect(gUserCredential)'))

    def test_beta_warning_and_http_consent_are_localized(self):
        for language in ("en", "de"):
            strings = ET.parse(ROOT / f"indra/newview/skins/default/xui/{language}/strings.xml")
            entries = [e for e in strings.iter('string')
                       if e.get('name') == 'OGLBetaLoginUnavailable']
            self.assertEqual(len(entries), 1)
            self.assertIn('Beta', entries[0].text or '')
            notifications = ET.parse(
                ROOT / f"indra/newview/skins/default/xui/{language}/notifications.xml")
            matches = [e for e in notifications.iter('notification')
                       if e.get('name') == 'OGLHTTPLoginWarning']
            self.assertEqual(len(matches), 1)
            self.assertEqual(matches[0].find('form').get('name'), 'form')
            buttons = list(matches[0].find('form').findall('button'))
            self.assertEqual([int(b.get('index')) for b in buttons], [0, 1, 2])
            # Enter/Escape must not implicitly consent to sending credentials
            # over plaintext HTTP. The only default is explicit cancellation.
            self.assertEqual([b.get('name') for b in buttons
                              if b.get('default') == 'true'], ['OGLHTTPCancel'])
            self.assertIn('HTTP', matches[0].text or '')
            self.assertIn('OGLRememberPinnedHTTPLoginURI', matches[0].text or '')
            self.assertTrue(buttons[0].get('text') and buttons[1].get('text'))

    def test_consent_is_persistent_only_on_explicit_user_action(self):
        root = ET.parse(ROOT / 'indra/newview/app_settings/settings.xml').getroot()
        entries = {}
        children = list(root.find('map'))
        for i in range(0, len(children)-1, 2):
            if children[i].tag == 'key':
                entries[children[i].text] = children[i+1]
        fields = list(entries['OGLRememberPinnedHTTPLoginURI'])
        record = {fields[i].text: fields[i+1] for i in range(0, len(fields)-1, 2)}
        self.assertEqual(record['Persist'].text, '1')
        self.assertEqual(record['Type'].text, 'String')
        self.assertIsNone(record['Value'].text)
        self.assertNotIn('gSavedSettings.setString("OGLRememberPinnedHTTPLoginURI",',
                         self.startup)


if __name__ == "__main__":
    unittest.main()
