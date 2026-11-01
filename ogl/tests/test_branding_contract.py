#!/usr/bin/env python3
"""Static OGL fork smoke tests; runtime acceptance still requires a live GUI."""
import unittest
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]


class OGLBrandingContract(unittest.TestCase):
    def source(self, path):
        return (ROOT / path).read_text()

    def test_ogl_preferences_tab_brand_without_changing_registered_callbacks(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        files = sorted(base.glob('*/floater_preferences.xml'))
        self.assertEqual(len(files), 13)
        changed = 0
        for path in files:
            root = ET.parse(path).getroot()
            for tab in root.iter('panel'):
                if tab.get('name') == 'firestorm':
                    changed += 1
                    self.assertEqual(tab.get('label'),
                                     'OGL Funktionen' if path.parent.name == 'de'
                                     else 'OGL Features')
                    if path.parent.name == 'en':
                        self.assertEqual(tab.get('class'),'panel_preference_firestorm')
                        self.assertEqual(tab.get('filename'),
                                         'panel_preferences_firestorm.xml')
        self.assertGreaterEqual(changed, 9)
        prefs = ET.parse(base / 'en/panel_preferences_firestorm.xml').getroot()
        self.assertEqual(prefs.get('label'),'OGL Features')
        tabs = next(e for e in prefs.iter('tab_container') if e.get('name')=='tabs')
        self.assertEqual(tabs.get('label'),'OGL Preferences')
        self.assertTrue(any(e.get('name')=='tab-extras' for e in prefs.iter()))
        self.assertTrue(any(e.get('control_name')=='RestrainedLove' for e in prefs.iter()))
        self.assertNotIn('If enabled, Firestorm will use a progressive draw distance',
                         (base / 'en/panel_preferences_firestorm.xml').read_text())

    def test_nexverse_search_title_all_locales_preserves_existing_tabs(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        files = sorted(base.glob('*/floater_fs_search.xml'))
        self.assertEqual(len(files),11)
        for path in files:
            root = ET.parse(path).getroot()
            title = root.get('title','')
            self.assertIn('NexVerse',title,str(path))
            self.assertNotIn('Firestorm',title,str(path))
            self.assertEqual(root.get('name'),'floater_fs_search')
        search = ET.parse(base / 'en/floater_fs_search.xml').getroot()
        self.assertTrue(any(e.get('name')=='ls_tabs' for e in search.iter('tab_container')))
        self.assertTrue(any(e.get('name')=='panel_ls_people' for e in search.iter()))
        self.assertTrue(any(e.get('name')=='panel_ls_groups' for e in search.iter()))

    def test_source_brand_and_translations(self):
        self.assertIn('const std::string APP_NAME = "OGL";',
                      self.source("indra/llcommon/indra_constants.h"))
        for language in ("en", "de"):
            tree = ET.parse(ROOT / f"indra/newview/skins/default/xui/{language}/strings.xml")
            keys = {s.get("name"): (s.text or "").strip() for s in tree.getroot()}
            self.assertIn("OGLInsecureGridLogin", keys)
            self.assertIn("HTTP", keys["OGLInsecureGridLogin"])
        base = ET.parse(ROOT / "indra/newview/skins/default/xui/en/strings.xml")
        labels = {x.get("name"): (x.text or "").strip() for x in base.getroot()}
        self.assertEqual(labels.get("APP_NAME"), "OGL Viewer")

    def test_login_logo_and_asset(self):
        textures = ET.parse(ROOT / "indra/newview/skins/default/textures/textures.xml")
        mapping = {x.get("name"): x.get("file_name") for x in textures.getroot()}
        self.assertEqual(mapping["login_ogl_logo"], "windows/login_ogl_logo.png")
        png = ROOT / "indra/newview/skins/default/textures/windows/login_ogl_logo.png"
        self.assertTrue(png.is_file())
        self.assertEqual(png.read_bytes()[:8], b"\x89PNG\r\n\x1a\n")
        panel = self.source("indra/newview/skins/default/xui/en/panel_fs_nui_login.xml")
        self.assertIn('image_name="login_ogl_logo"', panel)

    def test_ogl_welcome_url_omits_firestorm_telemetry(self):
        cpp = self.source("indra/newview/fspanellogin.cpp")
        start = cpp.index('void FSPanelLogin::loadLoginPage()')
        stop = cpp.index('void FSPanelLogin::handleMediaEvent(', start)
        body = cpp[start:stop]
        self.assertIn('OGLNexVerseGrid::kWelcomeURI', body)
        self.assertIn('params["lang"] = LLUI::getLanguage()', body)
        self.assertIn('params["channel"] = LLVersionInfo::getInstance()->getChannel()', body)
        self.assertIn('params["grid"] = LLGridManager::getInstance()->getGridId()', body)
        self.assertIn('if (APP_NAME != "OGL")', body)
        self.assertLess(body.index('if (APP_NAME != "OGL")', body.index('// The OGL welcome page')),
                        body.index('params["os"]'))
        self.assertIn('params["sourceid"]', body)
        self.assertIn('params["skin"]', body)
        self.assertIn('params["login_content_version"]', body)
        self.assertIn('params[m.second]', body)
        self.assertLess(body.index('params[m.second]'),
                        body.index('// Make an LLURI with this augmented info'))
        self.assertNotIn('params["password"]', body)
        self.assertNotIn('params["username"]', body)

    def test_all_locale_support_strings_use_nexverse_identity(self):
        root = ROOT / 'indra/newview/skins/default/xui'
        strings_files = sorted(root.glob('*/strings.xml'))
        self.assertGreaterEqual(len(strings_files), 13)
        for filename in strings_files:
            nodes = [x for x in ET.parse(filename).iter('string')
                     if x.get('name') == 'SUPPORT_SITE']
            self.assertEqual(len(nodes), 1, str(filename))
            self.assertEqual((nodes[0].text or '').strip(), 'NexVerse Portal',
                             str(filename))

    def test_ogl_download_and_login_denial_use_our_github_source(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        releases = 'https://github.com/marlin198323-glitch/ogl-viewer/releases'
        for path in sorted(base.glob('*/strings.xml')):
            tree = ET.parse(path)
            data = {x.get('name'): (x.text or '').strip()
                    for x in tree.iter('string') if x.get('name')}
            if 'ViewerDownloadURL' in data:
                self.assertEqual(data['ViewerDownloadURL'], releases, str(path))
            if 'create_account_url' in data:
                self.assertEqual(data['create_account_url'], 'https://stadt-nexverse.de/',
                                 str(path))
            if 'LoginFailedViewerNotPermitted' in data:
                self.assertNotIn('firestormviewer.org',
                                 data['LoginFailedViewerNotPermitted'], str(path))
                self.assertNotIn('viewer-access-faq',
                                 data['LoginFailedViewerNotPermitted'], str(path))
        for lang in ('en', 'de'):
            tree = ET.parse(base / lang / 'strings.xml')
            labels = {x.get('name'): (x.text or '').strip()
                      for x in tree.iter('string') if x.get('name')}
            self.assertEqual(labels['ViewerDownloadURL'], releases)
            self.assertEqual(labels['create_account_url'], 'https://stadt-nexverse.de/')
            self.assertIn(releases, labels['LoginFailedViewerNotPermitted'])
            self.assertNotIn('firestormviewer.org', labels['LoginFailedViewerNotPermitted'])
            self.assertNotIn('secondlife.com', labels['LoginFailedViewerNotPermitted'])
            if lang == 'en':
                self.assertEqual(labels['DOWNLOAD_URL'], releases)
                self.assertEqual(labels['RELEASE_NOTES_BASE_URL'],
                                 releases + '?version=')
                self.assertNotIn('firestormviewer.org', labels['MBFreezeDetected'])
                self.assertIn('external website', labels['TooltipSLIcon'])

    def test_ogl_login_failure_log_excludes_raw_server_response(self):
        cpp = self.source('indra/newview/llstartup.cpp')
        start = cpp.index('if(LLLoginInstance::getInstance()->authFailure())')
        stop = cpp.index('LLSD response = LLLoginInstance::getInstance()->getResponse();',start)
        diagnostic = cpp[start:stop]
        self.assertIn('APP_NAME == "OGL"', diagnostic)
        self.assertIn('Authentication failed; server response redacted.', diagnostic)
        self.assertIn('else', diagnostic)
        self.assertIn('LLLoginInstance::getInstance()->getResponse() << LL_ENDL', diagnostic)
        self.assertLess(diagnostic.index('APP_NAME == "OGL"'),
                        diagnostic.index('LLLoginInstance::getInstance()->getResponse() << LL_ENDL'))

    def test_prelogin_help_uses_official_nexverse_portal(self):
        source = self.source('indra/newview/fspanellogin.cpp')
        start = source.index('void FSPanelLogin::onClickHelp(')
        end = source.index('void FSPanelLogin::onPassKey(', start)
        method = source[start:end]
        self.assertIn('if (APP_NAME == "OGL")', method)
        self.assertIn('LLWeb::loadURLExternal(OGLNexVerseGrid::kWebsite)', method)
        self.assertLess(method.index('LLWeb::loadURLExternal(OGLNexVerseGrid::kWebsite)'),
                        method.index('LLViewerHelp::getInstance()'))
        self.assertIn('return;', method[:method.index('LLViewerHelp::getInstance()')])

    def test_nexverse_background_and_https_auth_guard(self):
        source = self.source("indra/newview/fspanellogin.cpp")
        self.assertIn("OGLNexVerseGrid::kWelcomeURI", source)
        self.assertIn('Blocked unpinned grid login endpoint.', source)
        self.assertIn("getLoginURIs(login_uris)", source)
        self.assertIn("OGLUnexpectedGridEndpoint", source)
        self.assertIn("OGLHTTPLoginWarning", source)
        self.assertIn("OGLNexVerseGrid::isExactPinnedHTTPLogin", source)
        self.assertIn("User approved HTTP login for pinned Stadt NexVerse", source)

    def test_nexverse_login_bar_without_rtx_debug(self):
        from struct import unpack
        path = ROOT / "indra/newview/skins/default/xui/en/panel_fs_nui_login.xml"
        root = ET.parse(path).getroot()
        elements = {x.get("name"): x for x in root.iter() if x.get("name")}
        for name in ("ogl_brand_caption", "ogl_username_label",
                     "ogl_password_label", "ogl_start_label"):
            self.assertIn(name, elements)
        self.assertEqual(elements["connect_btn"].get("label"), "Enter world")
        german = ET.parse(ROOT / "indra/newview/skins/default/xui/de/panel_fs_nui_login.xml")
        glabels = [x for x in german.getroot().iter() if x.get("name") == "connect_btn"]
        self.assertEqual(len(glabels), 1)
        self.assertEqual(glabels[0].get("label"), "Welt betreten")
        self.assertEqual(elements["connect_btn"].get("image_unselected"), "OGL_Login")
        self.assertEqual(elements["panel_login"].get("bg_opaque_color"), "OGLLoginBackground")
        textures = ET.parse(ROOT / "indra/newview/skins/default/textures/textures.xml")
        names = {x.get("name") for x in textures.getroot()}
        for name in ("OGL_Login", "OGL_Login_Over", "OGL_Login_Pressed"):
            self.assertIn(name, names)
            data = (ROOT / f"indra/newview/skins/default/textures/widgets/{name}.png").read_bytes()
            self.assertEqual(data[:8], bytes.fromhex("89504e470d0a1a0a"))
            self.assertEqual(unpack("!II", data[16:24]), (299, 64))
        login_cpp = self.source("indra/newview/fspanellogin.cpp")
        self.assertNotIn('ogl_rtx_rendering_active()', login_cpp)
        self.assertNotIn('ogl_rtx_hardware_supported()', login_cpp)
        self.assertNotIn('Vulkan RT bereit', login_cpp)
        self.assertNotIn('Effekte aus', login_cpp)
        self.assertNotIn('ogl_graphics_status', elements)
        self.assertNotIn('ogl_graphics_status', login_cpp)
        self.assertIn('"ui_stack" : "login_widgets"', login_cpp)
        self.assertIn('LLWeb::loadURLInternal(OGLNexVerseGrid::kWebsite)', login_cpp)
        self.assertIn('LLWeb::loadURLExternal(OGLNexVerseGrid::kWebsite)', login_cpp)
        self.assertIn('APP_NAME == "OGL" || !gSavedSettings.getBOOL', login_cpp)

    def test_responsive_ogl_login_palette_and_locale_contract(self):
        xui = ROOT / "indra/newview/skins/default/xui"
        english = ET.parse(xui / "en/panel_fs_nui_login.xml").getroot()
        german = ET.parse(xui / "de/panel_fs_nui_login.xml").getroot()
        by_name = {node.get("name"): node for node in english.iter()
                   if node.get("name")}
        localized = {node.get("name"): node for node in german.iter()
                     if node.get("name")}
        colors = ET.parse(ROOT / "indra/newview/skins/default/colors.xml")
        named = {c.get("name") for c in colors.getroot()}
        self.assertTrue({"OGLLoginBackground", "OGLLoginSurface",
                         "OGLLoginAccent", "OGLLoginLink"} <= named)
        for node in english.iter():
            for attr in ("bg_opaque_color", "text_color"):
                token = node.get(attr)
                if token and token.startswith("OGLLogin"):
                    self.assertIn(token, named)
        self.assertEqual(by_name["ui_stack"].get("height"), "140")
        self.assertEqual(by_name["1st_row"].get("height"), "82")
        self.assertEqual(by_name["2nd_row"].get("height"), "58")
        self.assertEqual(by_name["login_html"].get("height"), "610")
        self.assertEqual(by_name["login_html"].get("follows"), "all")
        self.assertNotIn("mode_selection", by_name)
        self.assertEqual(by_name["grid_panel"].get("visible"), "true")
        for name in ("username_combo", "password_edit", "server_combo",
                     "start_location_combo", "remember_username_check",
                     "remember_check"):
            self.assertIn(name, by_name)
        for name in ("remember_username_check", "remember_check"):
            self.assertNotEqual(by_name[name].get("tab_stop"), "false")
        self.assertEqual(by_name["ogl_password_label"].text, "PASSWORD")
        self.assertEqual(localized["ogl_password_label"].text, "PASSWORT")
        self.assertEqual(localized["ogl_username_label"].text, "NEXVERSE-NAME")
        self.assertEqual(localized["ogl_start_label"].text, "STARTORT")
        self.assertEqual(localized["connect_btn"].get("label"), "Welt betreten")
        self.assertNotIn("bg_image_always_focused", self.source(
            "indra/newview/skins/default/xui/en/panel_fs_nui_login.xml"))
        self.assertNotIn("ogl_graphics_status", self.source(
            "indra/newview/skins/default/xui/en/panel_fs_nui_login.xml"))

    def test_ogl_main_menu_portal_and_marketplace_use_verified_nexverse_urls(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        targets = {'Manage Account': 'https://stadt-nexverse.de/',
                   'MarketplaceListings': 'https://marketplace.stadt-nexverse.de/'}
        for language in ('en','de'):
            root = ET.parse(base / language / 'menu_viewer.xml').getroot()
            for name,url in targets.items():
                entries = [e for e in root.iter('menu_item_call') if e.get('name')==name]
                self.assertEqual(len(entries), 1, f'{language}:{name}')
                callback = entries[0].find('menu_item_call.on_click')
                self.assertIsNotNone(callback,f'{language}:{name}')
                self.assertEqual(callback.get('function'), 'Advanced.ShowURL')
                self.assertEqual(callback.get('parameter'), url)
                self.assertNotIn('secondlife.com',ET.tostring(entries[0],encoding='unicode'))
            self.assertTrue(root.find('.//menu[@name="Me"]') is not None)

    def test_ogl_main_menu_removes_unavailable_secondlife_commerce_entries(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        obsolete = {'Buy and Sell L$', 'Membership', 'SL Marketplace',
                    'LindenXchange', 'SL Community Pages'}
        for language in ('en','de'):
            root = ET.parse(base / language / 'menu_viewer.xml').getroot()
            for element in root.iter():
                self.assertNotIn(element.get('name'),obsolete,
                                 f'{language}:{element.get("name")}')
            all_url_values = ' '.join(e.get('parameter','') for e in root.iter()
                                      if 'parameter' in e.attrib)
            self.assertNotIn('secondlife.com/account/',all_url_values)
            self.assertNotIn('secondlife.com/my/account/membership',all_url_values)
            self.assertNotIn('secondlife.com/my/lindex/market',all_url_values)

    def test_all_thirteen_locales_drop_legacy_secondlife_economy(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        menus = sorted(base.glob('*/menu_viewer.xml'))
        self.assertEqual(len(menus), 13)
        obsolete = {'Buy and Sell L$', 'Membership', 'SL Marketplace',
                    'LindenXchange', 'SL Community Pages', 'Manage My Account'}
        for path in menus:
            root = ET.parse(path).getroot()
            self.assertTrue(any(e.get('name') == 'Me' for e in root.iter('menu')),
                            str(path))
            self.assertFalse(obsolete & {e.get('name') for e in root.iter()},
                             str(path))

    def test_every_translated_account_marketplace_news_and_issue_url_is_pinned(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        expected = {
            'Manage Account': 'https://stadt-nexverse.de/',
            'MarketplaceListings': 'https://marketplace.stadt-nexverse.de/',
            'NexVerse News': 'https://stadt-nexverse.de/',
            'Report Bug': 'https://github.com/marlin198323-glitch/ogl-viewer/issues',
        }
        menus = sorted(base.glob('*/menu_viewer.xml'))
        self.assertEqual(len(menus), 13)
        for path in menus:
            root = ET.parse(path).getroot()
            for name, uri in expected.items():
                matching = [e for e in root.iter() if e.get('name') == name]
                self.assertLessEqual(len(matching), 1, (path,name))
                for entry in matching:
                    self.assertEqual(entry.tag, 'menu_item_call', (path,name))
                    click = entry.find('menu_item_call.on_click')
                    self.assertIsNotNone(click, (path,name))
                    self.assertEqual(click.get('function'), 'Advanced.ShowURL',
                                     (path,name))
                    self.assertEqual(click.get('parameter'), uri, (path,name))
                    self.assertNotIn('secondlife.com',
                                     ET.tostring(entry, encoding='unicode'))

    def test_portuguese_account_never_opens_secondlife_membership(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        pt = ET.parse(base / 'pt/menu_viewer.xml').getroot()
        all_text = ET.tostring(pt,encoding='unicode')
        self.assertNotIn('accounts.secondlife.com', all_text)
        self.assertNotIn('secondlife.com/my/account', all_text)
        self.assertNotIn('change_membership', all_text)
        self.assertNotIn('WebLaunchJoinNow', all_text)
        account = next(e for e in pt.iter('menu_item_call')
                       if e.get('name') == 'Manage Account')
        self.assertEqual(account.get('label'), 'NexVerse Portal')
        self.assertEqual(account.find('menu_item_call.on_click').get('parameter'),
                         'https://stadt-nexverse.de/')
        for lang in ('tr', 'da'):
            translated = ET.parse(base / lang / 'menu_viewer.xml').getroot()
            self.assertFalse(any(e.get('name') == 'Manage My Account'
                                 for e in translated.iter()), lang)
            # No parallel account menu: the English base defines the
            # shared, working NexVerse menu and this locale inherits it.
            self.assertFalse(any(e.get('name') == 'Manage Account'
                                 for e in translated.iter()), lang)

    def test_locale_overlay_keeps_ogl_portal_and_real_opensim_commands(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        english = ET.parse(base / 'en/menu_viewer.xml').getroot()
        for name in ('Manage Account', 'MarketplaceListings', 'Report Abuse',
                     'Buy Land', 'World Map', 'Nearby Chat', 'Conversations',
                     'About Second Life', 'Report Bug'):
            self.assertTrue(any(e.get('name') == name for e in english.iter()),
                            name)
        for path in sorted(base.glob('*/menu_viewer.xml')):
            tree = ET.parse(path).getroot()
            self.assertTrue(any(e.get('name') == 'Help' for e in tree.iter('menu')),
                            str(path))
            self.assertFalse(any('firestorm' in e.get('label','').lower()
                                 for e in tree.iter()),str(path))

    def test_ogl_main_menu_navigation_labels_preserve_callback_names_and_shortcuts(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        labels = {
            'en': {'Communicate':'Chat & Contacts','World':'Explore',
                   'Help':'Help & Support', 'Nearby Chat':'Local Chat',
                   'Conversations':'Messages'},
            'de': {'Communicate':'Chat & Kontakte','World':'Welt entdecken',
                   'Help':'Hilfe & Support','Nearby Chat':'Lokaler Chat',
                   'Conversations':'Nachrichten'},
        }
        for language, names in labels.items():
            root = ET.parse(base / language / 'menu_viewer.xml').getroot()
            for name,label in names.items():
                entries = [e for e in root.iter() if e.get('name')==name]
                self.assertEqual(len(entries),1,(language,name))
                self.assertEqual(entries[0].get('label'),label)
            self.assertEqual(root.find('.//menu[@name="Help"]').tag,'menu')
        english=ET.parse(base / 'en/menu_viewer.xml').getroot()
        for name,shortcut,target in (
            ('Nearby Chat','control|H','fs_nearby_chat'),
            ('Conversations','control|T','fs_im_container'),
        ):
            item=next(e for e in english.iter('menu_item_check') if e.get('name')==name)
            self.assertEqual(item.get('shortcut'),shortcut)
            self.assertTrue(any(e.get('parameter')==target for e in item))
        # Grid-specific core menu and account windows remain.
        self.assertIsNotNone(english.find('.//menu[@name="Me"]'))
        self.assertIsNotNone(english.find('.//menu[@name="BuildTools"]'))

    def test_ogl_world_menus_have_no_obsolete_linden_or_firestorm_entries(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        paths = sorted(base.glob('*/menu_viewer.xml'))
        self.assertEqual(len(paths), 13)
        obsolete = {'Events', 'Linden Home', 'Official Linden Blog',
                    'Public Issue Tracker', 'Firestorm MoTD'}
        for path in paths:
            menu = ET.parse(path).getroot()
            self.assertEqual(menu.tag, 'menu_bar')
            self.assertFalse(obsolete & {e.get('name') for e in menu.iter()},
                             str(path))
            self.assertFalse(any('Firestorm' in e.get('label', '')
                                 for e in menu.iter()), str(path))
        # Keep the actual land and world functions on OpenSim.
        english = ET.parse(base / 'en/menu_viewer.xml').getroot()
        for name in ('Mini-Map', 'World Map', 'Buy Land',
                     'About Second Life', 'Report Abuse', 'Script Library'):
            self.assertTrue(any(e.get('name') == name for e in english.iter()),
                            name)

    def test_ogl_nexverse_news_uses_portal_instead_of_old_motd(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        for lang, label in (('en','NexVerse News'),
                            ('de','NexVerse Nachrichten')):
            root = ET.parse(base / lang / 'menu_viewer.xml').getroot()
            news = [e for e in root.iter('menu_item_call')
                    if e.get('name') == 'NexVerse News']
            self.assertEqual(len(news), 1, lang)
            self.assertEqual(news[0].get('label'), label)
            callback = news[0].find('menu_item_call.on_click')
            self.assertIsNotNone(callback)
            self.assertEqual(callback.get('function'), 'Advanced.ShowURL')
            self.assertEqual(callback.get('parameter'), 'https://stadt-nexverse.de/')
            discover = next(e for e in root.iter('menu')
                            if e.get('name') == 'Content')
            self.assertEqual(discover.get('label'),
                             'Discover' if lang == 'en' else 'Entdecken')

    def test_ogl_issue_tracker_and_downloads_have_real_menu_actions(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        routes = {'Report Bug':
                  'https://github.com/marlin198323-glitch/ogl-viewer/issues',
                  'OGL Viewer Downloads':
                  'https://github.com/marlin198323-glitch/ogl-viewer/releases'}
        for lang in ('en','de'):
            root = ET.parse(base / lang / 'menu_viewer.xml').getroot()
            help_menu = next(e for e in root.iter('menu')
                             if e.get('name') == 'Help')
            for name, route in routes.items():
                candidates = [e for e in help_menu.iter('menu_item_call')
                              if e.get('name') == name]
                self.assertEqual(len(candidates), 1, (lang,name))
                on_click = candidates[0].find('menu_item_call.on_click')
                self.assertIsNotNone(on_click)
                self.assertEqual(on_click.get('function'), 'Advanced.ShowURL')
                self.assertEqual(on_click.get('parameter'), route)
            self.assertTrue(any(e.get('name') == 'NexVerse Portal'
                                for e in help_menu.iter('menu_item_call')))
            self.assertTrue(any(e.get('name') == 'About Second Life'
                                for e in help_menu.iter('menu_item_call')))

    def test_translated_script_library_remains_callable(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        for lang in ('en', 'de', 'pt'):
            root = ET.parse(base / lang / 'menu_viewer.xml').getroot()
            found = [e for e in root.iter() if e.get('name') == 'Script Library']
            self.assertEqual(len(found), 1, lang)
            self.assertEqual(found[0].tag, 'menu_item_call', lang)
            if lang == 'en':
                on_click = found[0].find('menu_item_call.on_click')
                self.assertEqual(on_click.get('function'), 'PromptShowURL')
                self.assertIn('LSL_Library', on_click.get('parameter'))

    def test_login_and_world_menus_remove_upstream_promotions(self):
        removed = {
            "Firestorm Blog", "Firestorm Primfeed", "Firestorm Flickr",
            "Firestorm YouTube", "Firestorm Twitter", "Firestorm Plurk",
            "Firestorm Wiki", "firestorm_support_group",
            "Firestorm Classes Schedule", "Firestorm Events Calendar",
        }
        base = ROOT / "indra/newview/skins/default/xui"
        paths = list(base.glob("*/menu_login.xml")) + list(base.glob("*/menu_viewer.xml"))
        self.assertGreaterEqual(len(paths), 26)
        for path in paths:
            tree = ET.parse(path)
            for node in tree.iter():
                for visible in ("label", "tool_tip"):
                    self.assertNotIn("firestorm", node.get(visible, "").lower(),
                                     f"{path}:{node.get('name')}")
                if node.tag == "menu_item_call":
                    self.assertNotIn(node.get("name"), removed, str(path))
        for menu in ("menu_viewer.xml", "menu_login.xml"):
            en = ET.parse(base / "en" / menu).getroot()
            self.assertTrue(any(e.get("name") == "NexVerse Portal"
                                for e in en.iter("menu_item_call")))
            self.assertTrue(any(e.get("name") == "About Second Life"
                                for e in en.iter("menu_item_call")))
            data = (base / "en" / menu).read_text()
            self.assertIn("WebLaunchExternalTarget,https://stadt-nexverse.de/", data)
            self.assertNotIn("wiki.firestormviewer.org", data)
            self.assertNotIn("firestormviewer.org", data)

    def test_ogl_legacy_login_controls_removed_from_every_locale(self):
        paths = sorted((ROOT / 'indra/newview/skins').glob(
            '*/xui/*/panel_fs_nui_login.xml'))
        self.assertEqual(len(paths), 12)
        for path in paths:
            root = ET.parse(path).getroot()
            names = {node.get('name') for node in root.iter() if node.get('name')}
            for obsolete in ('mode_selection', 'mode_selection_text',
                             'mode_combo', 'grid_builder_text'):
                self.assertNotIn(obsolete, names, str(path))
            self.assertIn('ui_stack', names, str(path))
        cpp = self.source('indra/newview/fspanellogin.cpp')
        self.assertIn('if (APP_NAME != "OGL")', cpp)
        self.assertIn('LLUICtrl& mode_combo', cpp)
        self.assertIn('LLTextBox* grid_builder_text', cpp)
        self.assertIn('buildFromFile( "panel_fs_login.xml")', cpp)
        self.assertIn('getChild<LLUICtrl>("mode_combo")->setValue(original_value)', cpp)
        for path in sorted((ROOT / 'indra/newview/skins').glob(
            '*/xui/*/panel_fs_login.xml')):
            ET.parse(path)
        # Retain source and license assets of old alternative skins, even
        # though the OGL startup always uses the canonical new login.
        self.assertTrue((ROOT / 'indra/newview/skins/default/xui/en/panel_fs_login.xml').is_file())

    def test_fixed_grid_selector_and_saved_account_callbacks(self):
        xui = ROOT / "indra/newview/skins/default/xui"
        login = ET.parse(xui / "en/panel_fs_nui_login.xml").getroot()
        de = ET.parse(xui / "de/panel_fs_nui_login.xml").getroot()
        controls = {n.get("name"): n for n in login.iter() if n.get("name")}
        localized = {n.get("name"): n for n in de.iter() if n.get("name")}
        self.assertEqual(controls["server_combo"].get("allow_text_entry"), "false")
        self.assertEqual(controls["grid_panel"].get("visible"), "true")
        self.assertIn("Stadt NexVerse Beta",
                      (controls["ServerComboTooltip"].text or ""))
        self.assertIn("Beta-Anmeldung",
                      (localized["ServerComboTooltip"].text or ""))
        for root in (login, de):
            password_url = next(n for n in root.iter()
                                if n.get("name") == "forgot_password_url")
            self.assertEqual(password_url.text.strip(), "https://stadt-nexverse.de/")
        self.assertEqual(controls["username_combo"].get("allow_text_entry"), "true")
        self.assertEqual(controls["password_edit"].get("is_password"), "true")
        self.assertEqual(controls["remember_username_check"].get("control_name"),
                         "FSRememberUsername")
        self.assertEqual(controls["remember_check"].get("control_name"),
                         "RememberPassword")
        self.assertEqual(controls["remember_check"].get("enabled_control"),
                         "FSRememberUsername")
        source = self.source("indra/newview/fspanellogin.cpp")
        self.assertIn('setCommitCallback(boost::bind(&FSPanelLogin::onSelectServer', source)
        self.assertIn('setCommitCallback(boost::bind(&FSPanelLogin::onSelectUser', source)
        self.assertIn('gSecAPIHandler->loadCredential(cred_name)', source)
        self.assertIn('gSecAPIHandler->deleteCredential(credential)', source)
        self.assertIn('setMaxTextChars(LLGridManager::getInstance()->isInSecondLife()', source)
        self.assertIn('if (APP_NAME != "OGL")', source)
        self.assertIn('LLTextBox* grid_mgr_help_text', source)
        self.assertIn('OGLNexVerseGrid::isFixedGrid(server_combo_val.asString())', source)

    def test_alternate_skin_logins_keep_all_required_ogl_controls(self):
        # Actual --skin starlight runtime needs a full XUI definition:
        # a root-only override caused dummy controls and a black login.
        # Keep the two pre-existing override paths fully functional.
        canonical = self.source(
            "indra/newview/skins/default/xui/en/panel_fs_nui_login.xml")
        for skin in ("starlight", "starlightcui"):
            file = ROOT / f"indra/newview/skins/{skin}/xui/en/panel_fs_nui_login.xml"
            self.assertEqual(file.read_text(), canonical)
            root = ET.parse(file).getroot()
            names = {n.get("name") for n in root.iter()}
            for required in ("ui_stack", "username_combo", "password_edit",
                             "connect_btn", "server_combo", "remember_check",
                             "login_html"):
                self.assertIn(required, names)
            self.assertNotIn("login_fs_logo", file.read_text())

    def test_saved_account_isolation_and_removal_regression(self):
        # No credentials are created or read by this test. Check that the
        # real authentication callbacks retain their secure storage calls.
        cpp = self.source("indra/newview/fspanellogin.cpp")
        start = self.source("indra/newview/llstartup.cpp")
        self.assertIn('gSecAPIHandler->loadCredential(credentialName())', cpp)
        self.assertIn('gSecAPIHandler->loadCredential(cred_name)', cpp)
        self.assertIn('gSecAPIHandler->deleteCredential(credential)', cpp)
        self.assertIn('const LLSD selected_credential = combo->getSelectedValue()', cpp)
        self.assertIn('Refused deletion of unlisted-grid credential.', cpp)
        self.assertIn('A typed username is not a saved account.', cpp)
        self.assertIn('gSecAPIHandler->saveCredential(gUserCredential, gRememberPassword)', start)
        self.assertIn('if (gSavedSettings.getBOOL("FSRememberUsername"))', start)
        self.assertIn('remember = sInstance->getChild<LLUICtrl>("remember_check")->getValue()', cpp)
        self.assertIn('gSavedSettings.setBOOL("RememberPassword", gRememberPassword)', start)
        self.assertIn('APP_NAME == "OGL" && !OGLNexVerseGrid::isFixedGrid(gridname)', cpp)
        self.assertIn('Rejected saved account from unlisted grid.', cpp)
        self.assertIn('APP_NAME == "OGL" || gSavedSettings.getBOOL("ForceShowGrid")', cpp)
        self.assertIn('username_combo->setTextEntry(LLStringUtil::null)', cpp)
        start_switch = cpp.index('LLGridManager::getInstance()->getGrid() != previous_grid')
        switch = cpp[start_switch:cpp.index('/*\n     * Determine whether or not the value in the start_location_combo',start_switch)]
        self.assertIn('clearLoginFieldsForGridChange();', switch)
        start_helper = cpp.index('void FSPanelLogin::clearLoginFieldsForGridChange()')
        helper = cpp[start_helper:cpp.index('void FSPanelLogin::onSelectServer()', start_helper)]
        for term in ('getChild<LLLineEditor>("password_edit")->clear()',
                     'mPasswordModified = false;', 'mPasswordLength = 0;',
                     'mUsernameLength = 0;', 'sPassword.clear();',
                     'addUsersToCombo(true)', 'updateLoginButtons()'):
            self.assertIn(term, helper)
        self.assertLess(helper.index('addUsersToCombo(true)'),
                        helper.index('username_combo->setTextEntry(LLStringUtil::null)'))
        # A non-separated @-username must not lose its last character.
        self.assertIn('cname = cname.substr(0, arobase);', cpp)
        self.assertIn('LLStringUtil::trim(cname);', cpp)

    def test_linux_ogl_desktop_identity_and_legacy_error_links(self):
        sdl = self.source('indra/llwindow/llwindowsdl2.cpp')
        idx=sdl.index('SDL_SetHint("SDL_VIDEO_X11_WMCLASS", "ogl-viewer")')
        self.assertLess(idx, sdl.index('SDL_Init(SDL_INIT_VIDEO'))
        self.assertIn('SDL_SetHint("SDL_VIDEO_WAYLAND_WMCLASS", "ogl-viewer")', sdl)
        self.assertIn('XSetClassHint(mSDL_Display, mSDL_XWindowID, &ogl_class)', sdl)
        self.assertIn('ogl_class.res_name = const_cast<char*>("ogl-viewer")', sdl)
        self.assertIn('ogl_class.res_class = const_cast<char*>("ogl-viewer")', sdl)
        self.assertIn('OGL Viewer requires a 32-bit True Color display.', sdl)
        self.assertIn('if (APP_NAME == "OGL")', sdl)
        desktop = self.source('indra/newview/linux_tools/refresh_desktop_app_entry.sh')
        self.assertIn('Name=OGL Viewer', desktop)
        self.assertIn('StartupWMClass=ogl-viewer', desktop)
        self.assertIn('ogl-viewer.desktop', desktop)
        self.assertIn("printf '%b", desktop)
        self.assertIn('grep -Fqx "Exec=', desktop)
        self.assertIn('rm -f -- "$old_entry"', desktop)
        self.assertNotIn('Name=Firestorm Viewer', desktop)
        self.assertNotIn('StartupWMClass=do-not-directly-run-firestorm-bin', desktop)
        app = self.source('indra/newview/llappviewer.cpp')
        self.assertIn('Please reinstall an official OGL Viewer build', app)
        self.assertIn('APP_NAME == "OGL" ? "https://stadt-nexverse.de/"', app)
        self.assertIn('https://www.firestormviewer.org/choose-your-platform/', app)

    def test_ogl_does_not_request_legacy_firestorm_network_services(self):
        fs_data = self.source('indra/newview/fsdata.cpp')
        start = fs_data.index('void FSData::startDownload()')
        body = fs_data[start:fs_data.index('// call this _after_ the login screen', start)]
        self.assertIn('if (APP_NAME == "OGL")', body)
        self.assertIn('mFSDataDone = true;', body)
        self.assertIn('Skipped legacy Firestorm configuration downloads.', body)
        self.assertLess(body.index('mFSDataDone = true;'),
                        body.index('callbackHttpGetRaw('))
        self.assertLess(body.index('return;'), body.index('callbackHttpGetRaw('))
        start = fs_data.index('void FSData::downloadAgents()')
        agents = fs_data[start:fs_data.index('void FSData::processData(', start)]
        self.assertIn('if (APP_NAME == "OGL")', agents)
        self.assertIn('mAgentsDone = true;', agents)
        self.assertLess(agents.index('return;'), agents.index('callbackHttpGetRaw('))

    def test_ogl_browser_user_agent_uses_ogl_skin_not_firestorm(self):
        media = self.source('indra/newview/llviewermedia.cpp')
        start = media.index('std::string LLViewerMedia::getCurrentUserAgent()')
        part = media[start:media.index('void LLViewerMedia::updateBrowserUserAgent()', start)]
        self.assertIn('(APP_NAME == "OGL" ? "OGL" : skin_name)', part)
        self.assertIn('codec << "SecondLife/";', part)
        self.assertIn('LLVersionInfo::instance().getVersion()', part)

    def test_prelogin_quit_avoids_legacy_modal_without_skipping_cleanup(self):
        source = self.source("indra/newview/llappviewer.cpp")
        start = source.index('void LLAppViewer::userQuit()')
        end = source.index('static bool finish_early_exit', start)
        callback = source[start:end]
        self.assertIn('APP_NAME == "OGL"', callback)
        self.assertIn('LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP', callback)
        self.assertLess(callback.index('requestQuit();'),
                        callback.index('LLNotificationsUtil::add("ConfirmQuit");'))
        self.assertIn('gViewerWindow->getProgressView()->getVisible()', callback)
        self.assertIn('LLNotificationsUtil::add("ConfirmQuit");', callback)
        self.assertIn('void LLAppViewer::requestQuit()', source)
        self.assertIn('forceQuit();', source)

    def test_grid_bearing_start_location_isolates_login_credentials(self):
        cpp = self.source("indra/newview/fspanellogin.cpp")
        start = cpp.index("void FSPanelLogin::onUpdateStartSLURL(")
        end = cpp.index("void FSPanelLogin::setLocation(", start)
        callback = cpp[start:end]
        self.assertIn('OGLNexVerseGrid::isFixedGrid(slurl_grid)', callback)
        self.assertIn('Blocked unlisted grid in start location.', callback)
        self.assertLess(callback.index('Blocked unlisted grid in start location.'),
                        callback.index('LLGridManager::getInstance()->setGridChoice(slurl_grid)'))
        self.assertIn('server_combo->setSelectedByValue(slurl_grid, true)', callback)
        self.assertIn('sInstance->clearLoginFieldsForGridChange()', callback)
        self.assertLess(callback.index('sInstance->clearLoginFieldsForGridChange()'),
                        callback.index('updateServer(); // to change the links'))

    def test_login_welcome_url_cannot_be_replaced_and_creds_not_logged(self):
        cpp = self.source("indra/newview/fspanellogin.cpp")
        start = cpp.index('void FSPanelLogin::loadLoginPage()')
        end = cpp.index('void FSPanelLogin::handleMediaEvent', start)
        loader = cpp[start:end]
        self.assertIn('OGLNexVerseGrid::kWelcomeURI', loader)
        self.assertIn('if (APP_NAME != "OGL" && force_login_url.length() > 0)', loader)
        self.assertIn('gSavedSettings.getString("ForceLoginURL")', loader)
        self.assertNotIn('<< *credential', cpp)
        self.assertIn('Restoring login fields from credential store.', cpp)
        self.assertIn('if (credential.isNull())', cpp)
        self.assertIn('gSecAPIHandler->loadCredential(credentialName())', cpp)

    def test_ogl_identity_is_not_logged_in_plain_text(self):
        startup = self.source('indra/newview/llstartup.cpp')
        self.assertIn('Attempting login on the selected NexVerse grid.', startup)
        self.assertIn('if (APP_NAME == "OGL")', startup)
        cpp = self.source('indra/newview/fspanellogin.cpp')
        self.assertIn('LL_DEBUGS("Favorites") << "Current user:', cpp)
        self.assertIn('if (APP_NAME != "OGL")', cpp)
        self.assertIn('Restoring login fields from credential store.', cpp)

    def test_transient_login_credential_restoration_and_username_privacy(self):
        cpp = self.source('indra/newview/fspanellogin.cpp')
        stored = cpp.index('LLPointer<LLCredential> stored_credential = gSecAPIHandler->loadCredential(cred_name);')
        self.assertLess(cpp.index('if (stored_credential.isNull() ||', stored),
                        cpp.index('stored_credential->getAuthenticator()', stored))
        self.assertIn('sInstance->getChild<LLLineEditor>("password_edit")->setText(sPassword)', cpp)
        assert 'sPassword' in cpp
        connect = cpp[cpp.index('void FSPanelLogin::onClickConnect(void *)'):
                      cpp.index('void FSPanelLogin::onClickNewAccount(void*)')]
        self.assertIn('APP_NAME != "OGL" || gSavedSettings.getBOOL("FSRememberUsername")', connect)
        self.assertLess(connect.index('gSavedSettings.getBOOL("FSRememberUsername")'),
                        connect.index('gSavedSettings.setString("UserLoginInfo", credentialName())'))
        self.assertIn('gSavedSettings.getControl("UserLoginInfo")->resetToDefault()', connect)
        self.assertIn('getChild<LLUICtrl>("remember_username_check")->setCommitCallback(', cpp)
        self.assertIn('if (!gSavedSettings.getBOOL("FSRememberUsername"))', cpp)
        self.assertIn('gSavedSettings.getControl("UserLoginInfo")->resetToDefault()', cpp)
        self.assertIn('if (!control->getValue().asBoolean())', cpp)
        self.assertLess(cpp.index('remember_username_check")->setCommitCallback('),
                        cpp.index('void FSPanelLogin::onClickConnect(void *)'))
        for prefix in ('retrieving username:', 'account: ', 'agent: '):
            self.assertIn(f'LL_INFOS("Credentials", "Authentication") << "{prefix}"', cpp)
        self.assertGreaterEqual(cpp.count('if (APP_NAME != "OGL")'), 3)

    def test_ogl_remember_username_optout_disables_implicit_password_persistence(self):
        cpp = self.source("indra/newview/fspanellogin.cpp")
        start = cpp.index('if (!gSavedSettings.getBOOL("FSRememberUsername"))')
        end = cpp.index('mPreviousUsername = username_combo', start)
        opts = cpp[start:end]
        self.assertIn('gSavedSettings.getControl("UserLoginInfo")->resetToDefault()', opts)
        self.assertIn('gSavedSettings.setBOOL("RememberPassword", false)', opts)
        self.assertIn('gSavedSettings.setBOOL("AutoLogin", false)', opts)
        self.assertIn('if (!control->getValue().asBoolean())', opts)
        self.assertIn('getChild<LLUICtrl>("remember_check")->setValue(false)', opts)
        self.assertNotIn('deleteCredential(', opts)
        self.assertNotIn('clearCredentials(', opts)
        self.assertNotIn('syncProtectedMap()', opts)

    def test_ogl_optout_blocks_autologin_before_startup_connect(self):
        startup = self.source('indra/newview/llstartup.cpp')
        start = startup.index('The OGL privacy preference is authoritative')
        stop = startup.index('else if (gSavedSettings.getBOOL("AutoLogin"))', start)
        before_auto = startup[start:stop]
        self.assertIn('APP_NAME == "OGL"', before_auto)
        self.assertIn('!gSavedSettings.getBOOL("FSRememberUsername")', before_auto)
        self.assertIn('gSavedSettings.setBOOL("AutoLogin", false)', before_auto)
        self.assertIn('gSavedSettings.setBOOL("RememberPassword", false)', before_auto)
        self.assertIn('gSavedSettings.getControl("UserLoginInfo")->resetToDefault()', before_auto)
        self.assertIn('gRememberPassword = false', before_auto)
        self.assertLess(start, startup.index('else if (gSavedSettings.getBOOL("AutoLogin"))', start))
        self.assertIn('show_connect_box = true', before_auto)

    def test_saved_account_deletion_requires_explicit_confirmation(self):
        for language in ('en', 'de'):
            notices = ET.parse(ROOT / f'indra/newview/skins/default/xui/{language}/notifications.xml')
            dialog = next(n for n in notices.iter('notification')
                          if n.get('name') == 'ConfirmRemoveCredential')
            buttons = dialog.find('form').findall('button')
            self.assertEqual([b.get('name') for b in buttons], ['OK', 'Cancel'])
            self.assertEqual([b.get('name') for b in buttons
                              if b.get('default') == 'true'], ['Cancel'])
            self.assertNotEqual(buttons[0].get('text'), 'OK')
            self.assertTrue(buttons[1].get('text'))

    def test_grid_label_fits_and_no_forced_antivirus_modal(self):
        root = ET.parse(ROOT / "indra/newview/skins/default/xui/en/panel_fs_nui_login.xml").getroot()
        controls = {n.get("name"): n for n in root.iter() if n.get("name")}
        label = controls["grid_login_text"]
        combo = controls["server_combo"]
        group = controls["grid_panel"]
        self.assertGreaterEqual(int(label.get("width")), 55)
        self.assertLessEqual(int(label.get("left")) + int(label.get("width"))
                             + int(combo.get("left_pad")) + int(combo.get("width")),
                             int(group.get("width")))
        start = self.source("indra/newview/llstartup.cpp")
        self.assertIn('if (APP_NAME != "OGL")', start)
        self.assertIn('LLNotificationsUtil::add("WhitelistReminder");', start)
        self.assertIn('gSavedSettings.setBOOL("FSShowWhitelistReminder", false);', start)
        self.assertLess(start.index('if (APP_NAME != "OGL")'),
                        start.index('LLNotificationsUtil::add("WhitelistReminder");'))

    def test_password_visibility_is_within_password_field(self):
        root = ET.parse(ROOT / "indra/newview/skins/default/xui/en/panel_fs_nui_login.xml").getroot()
        controls = {n.get("name"): n for n in root.iter() if n.get("name")}
        pw = controls["password_edit"]
        pane = controls["password_container"]
        self.assertGreaterEqual(int(pw.get("text_pad_right")), 30)
        pw_top = int(controls["ogl_password_label"].get("top")) + int(
            controls["ogl_password_label"].get("height")) + int(pw.get("top_pad"))
        for name in ("password_show_btn", "password_hide_btn"):
            btn = controls[name]
            self.assertEqual(btn.get("follows"), "right|top")
            self.assertEqual(btn.get("tab_stop"), "true")
            self.assertTrue(pw_top <= int(btn.get("top")))
            self.assertTrue(int(btn.get("top")) + int(btn.get("height"))
                            <= pw_top + int(pw.get("height")))
            self.assertLessEqual(int(btn.get("left")) + int(btn.get("width")),
                                 int(pane.get("width")))
        self.assertEqual(controls["password_show_btn"].get("visible"), "true")
        self.assertEqual(controls["password_hide_btn"].get("visible"), "false")
        cpp = self.source("indra/newview/fspanellogin.cpp")
        self.assertIn('childSetAction("password_show_btn", onShowHidePasswordClick', cpp)
        self.assertIn('childSetAction("password_hide_btn", onShowHidePasswordClick', cpp)
        self.assertIn('syncShowHidePasswordButton()', cpp)

    def test_cross_platform_ogl_hidpi_login_window_contract(self):
        core = self.source('indra/newview/llviewerwindow.cpp')
        self.assertIn('static F32 oglConstrainLoginScale(LLWindow* window, F32 requested)', core)
        self.assertIn('F32(physical.mX) / 1024.f', core)
        self.assertIn('F32(physical.mY) / 680.f', core)
        self.assertIn('window->getDesktopWorkArea(&origin, &available)', core)
        self.assertIn('static bool ogl_initial_resize_checked = false;', core)
        self.assertEqual(core.count('oglExpandLoginWindow(mWindow);'), 2)
        self.assertIn('SDL_SetWindowPosition(mWindow, position.mX, position.mY)', self.source('indra/llwindow/llwindowsdl2.cpp'))
        self.assertIn('gSavedSettings.getF32("UIScaleFactor") * mWindow->getSystemUISize()', core)
        self.assertIn('window->setSize(LLCoordScreen(width, height))', core)
        self.assertIn('window->setPosition(LLCoordScreen(', core)
        self.assertEqual(core.count('ui_scale_factor = oglConstrainLoginScale(mWindow, ui_scale_factor);'), 2)
        self.assertIn('origin.mX + (available.mX - width) / 2', core)
        self.assertIn('origin.mY + (available.mY - height) / 2', core)
        self.assertNotIn('gSavedSettings.setF32("UIScaleFactor", requested)', core)
        base = self.source('indra/llwindow/llwindow.h')
        self.assertIn('getDesktopWorkArea(LLCoordScreen* position, LLCoordScreen* size)', base)
        for platform in ('sdl2', 'win32', 'macosx'):
            header = self.source(f'indra/llwindow/llwindow{platform}.h')
            impl = self.source(f'indra/llwindow/llwindow{platform}.cpp')
            self.assertIn('getDesktopWorkArea(LLCoordScreen* position, LLCoordScreen* size) override;', header)
            self.assertIn('getDesktopWorkArea(LLCoordScreen* position, LLCoordScreen* size)', impl)
        self.assertIn('SDL_GetDisplayUsableBounds(', self.source('indra/llwindow/llwindowsdl2.cpp'))
        self.assertIn('MonitorFromWindow(', self.source('indra/llwindow/llwindowwin32.cpp'))
        self.assertIn('CGDisplayBounds(', self.source('indra/llwindow/llwindowmacosx.cpp'))

    def test_720p_initial_window_and_safe_antivirus_login_notice(self):
        settings = ET.parse(ROOT / "indra/newview/app_settings/settings.xml")
        fields = list(settings.getroot().find("map"))
        defaults = {}
        for i in range(0, len(fields) - 1, 2):
            if fields[i].tag == "key":
                defaults[fields[i].text] = fields[i + 1]
        def default_value(key):
            record = list(defaults[key])
            return {record[i].text: record[i + 1].text
                    for i in range(0, len(record) - 1, 2)}["Value"]
        self.assertLessEqual(int(default_value("WindowHeight")), 680)
        self.assertLessEqual(int(default_value("WindowWidth")), 1024)
        for language in ("en", "de"):
            root = ET.parse(ROOT / (
                f"indra/newview/skins/default/xui/{language}/notifications.xml"))
            notices = [n for n in root.iter("notification")
                       if n.get("name") == "WhitelistReminder"]
            self.assertEqual(len(notices), 1)
            body = notices[0].text or ""
            self.assertNotIn("wiki.firestormviewer.org", body)
            self.assertIn("[APP_NAME]", body)
            floater = self.source(
                f"indra/newview/skins/default/xui/{language}/floater_whitelist.xml")
            self.assertNotIn("wiki.firestormviewer.org", floater)

    def test_rtx_world_frame_bridge_never_claims_live_rendering(self):
        pipeline = self.source("indra/newview/pipeline.cpp")
        module = self.source("ogl/modules/rtx/ogl_rtx_module.cpp")
        self.assertIn('ogl_rtx_submit_camera_sun(', pipeline)
        self.assertIn('getSunDirection()', pipeline)
        self.assertIn('camera->getOrigin().mV', pipeline)
        self.assertIn('ogl_rtx_submit_frame', module)
        self.assertIn('ogl_rtx_submit_geometry', module)
        self.assertIn('OGL RTX CPU scene snapshot: triangles=', module)
        self.assertIn('rendering_active=0', module)
        self.assertIn('OGL_RTX_MAX_SCENE_TRIANGLES', pipeline)
        self.assertIn('gObjectList.getObject(', pipeline)
        self.assertIn('getVolumeFace(', pipeline)
        self.assertIn('rendering_active=0', module)

    def test_launcher_keeps_test_profile_isolated(self):
        launcher = self.source("ogl/scripts/run-viewer-test.sh")
        self.assertIn('export OGL_X64_USER_DIR="$PROFILE"', launcher)
        self.assertIn('register_hopprotocol', launcher)
        shortcut = self.source("ogl/scripts/install-test-shortcut.sh")
        self.assertIn("login_ogl_logo.png", shortcut)

    def test_linux_icons_match_ogl_sizes(self):
        from struct import unpack
        icons = {
            "indra/newview/res/firestorm_icon.png": 512,
            "indra/newview/res-sdl/firestorm_icon.png": 512,
            "indra/newview/res-sdl/firestorm_icon128.png": 128,
            "indra/newview/skins/default/textures/firestorm_icon.png": 128,
        }
        for size in (16, 32, 48, 128, 256, 512):
            icons[f"indra/newview/icons/private-os/firestorm_{size}.png"] = size
        for filename, size in icons.items():
            data = (ROOT / filename).read_bytes()
            self.assertEqual(data[:8], bytes.fromhex("89504e470d0a1a0a"), filename)
            self.assertEqual(unpack("!II", data[16:24]), (size, size), filename)

    def test_ogl_bottom_chatbar_preserves_voice_and_toggle_callbacks(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        main = ET.parse(base / 'en/panel_toolbar_view.xml').getroot()
        button = next(e for e in main.iter('button')
                      if e.get('name') == 'local_chat_button')
        self.assertEqual(button.get('label'), 'Local Chat')
        self.assertEqual(button.get('height'), '24')
        self.assertEqual(button.get('width'), '104')
        self.assertIn('Ctrl+H', button.get('tool_tip'))
        callback = button.find('commit_callback')
        self.assertIsNotNone(callback)
        self.assertEqual(callback.get('function'), 'Floater.Toggle')
        self.assertEqual(callback.get('parameter'), 'fs_nearby_chat')
        voice = [e for e in main.iter('fs_nearby_chat_voice_monitor')
                 if e.get('name') == 'default_chat_bar']
        self.assertEqual(len(voice), 1)
        self.assertEqual(voice[0].get('spellcheck'), 'true')
        self.assertEqual(voice[0].get('follows'), 'left|right|bottom')
        self.assertTrue(any(e.get('name') == 'toolbar_bottom'
                            for e in main.iter('toolbar')))

    def test_old_toolbar_overlays_do_not_resurrect_removed_controls(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        for lang in ('de','pt','zh'):
            root = ET.parse(base / lang / 'panel_toolbar_view.xml').getroot()
            self.assertFalse(any(e.get('name') == 'nearby_voice_output_monitor'
                                 for e in root.iter()), lang)
            self.assertFalse(any(e.get('name') == 'open_chat_bar_button'
                                 for e in root.iter()), lang)
            self.assertFalse(any(e.get('name') == 'open_chat_bar_button_panel'
                                 for e in root.iter()), lang)
            self.assertEqual(
                len([e for e in root.iter('button')
                     if e.get('name') == 'local_chat_button']), 1, lang)
        german = ET.parse(base / 'de/panel_toolbar_view.xml').getroot()
        chat = next(e for e in german.iter('button')
                    if e.get('name') == 'local_chat_button')
        self.assertEqual(chat.get('label'), 'Lokaler Chat')
        self.assertEqual(chat.get('width'), '110')
        self.assertIn('Strg+H', chat.get('tool_tip'))

    def test_ogl_toolbar_customizer_fits_compact_windows_without_losing_actions(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        root = ET.parse(base / 'en/floater_toybox.xml').getroot()
        self.assertEqual(root.get('name'), 'Toybox')
        self.assertEqual(root.get('title'), 'Customize OGL Toolbar')
        self.assertEqual(root.get('width'), '820')
        self.assertEqual(root.get('height'), '520')
        self.assertEqual(root.get('can_resize'), 'true')
        self.assertLessEqual(int(root.get('min_width')), 820)
        self.assertLessEqual(int(root.get('min_height')), 520)
        self.assertTrue(root.get('save_rect') == 'true')
        bar = next(e for e in root.iter('toolbar')
                   if e.get('name') == 'toybox_toolbar')
        self.assertEqual(bar.get('read_only'), 'true')
        self.assertEqual(bar.get('right'), '-20')
        for name, action in (('btn_clear_all','Toybox.ClearAll'),
                             ('btn_restore_defaults','Toybox.RestoreDefaults')):
            button = next(e for e in root.iter('button')
                          if e.get('name') == name)
            self.assertEqual(button.get('disabled_control'), 'LockToolbars')
            self.assertEqual(button.get('top'), '490')
            callback = button.find('button.commit_callback')
            self.assertEqual(callback.get('function'), action)
        locked = next(e for e in root.iter('check_box')
                      if e.get('name') == 'check_toolbars_locked')
        self.assertEqual(locked.get('control_name'), 'LockToolbars')
        german = ET.parse(base / 'de/floater_toybox.xml').getroot()
        self.assertEqual(german.get('title'), 'OGL-Werkzeugleiste anpassen')

    def test_toolbar_context_menu_labels_keep_action_and_lock_semantics(self):
        base = ROOT / 'indra/newview/skins/default/xui'
        for orientation in ('horizontal','vertical'):
            for lang, label in (('en','Customize toolbar...'),
                                ('de','Werkzeugleiste anpassen...')):
                root = ET.parse(base / lang /
                                f'menu_toolbars_{orientation}.xml').getroot()
                entry = next(e for e in root.iter('menu_item_call')
                             if e.get('name') == 'Choose Buttons')
                self.assertEqual(entry.get('label'), label)
                if lang == 'en':
                    callback = entry.find('menu_item_call.on_click')
                    self.assertEqual(callback.get('function'), 'Floater.Show')
                    self.assertEqual(callback.get('parameter'), 'toybox')
                    self.assertTrue(any(e.get('name') == 'icons_only'
                                        for e in root.iter('menu_item_check')))
                    locked = next(e for e in root.iter('menu_item_check')
                                  if e.get('name') == 'toolbars_locked')
                    self.assertEqual(locked.find('on_check').get('control'),
                                     'LockToolbars')
                    self.assertEqual(locked.find('on_click').get('function'),
                                     'ToggleControl')
        inactive = ET.parse(base / 'en/menu_toolbars.xml').getroot()
        self.assertEqual(
            next(e for e in inactive.iter('menu_item_call')
                 if e.get('name') == 'Choose Buttons').get('label'),
            'Customize toolbar...')

    def test_ogl_chat_header_has_larger_round_avatar_and_readable_metadata(self):
        base = ROOT / 'indra/newview/skins/default/xui/en'
        root = ET.parse(base / 'panel_chat_header.xml').getroot()
        avatar = root.find('.//avatar_icon')
        name = root.find('.//text[@name="user_name"]')
        time = root.find('.//text[@name="time_box"]')
        layout = root.find('.//layout_stack[@name="header_ls"]')
        self.assertEqual(root.get('height'), '34')
        self.assertEqual(layout.get('height'), '34')
        self.assertEqual(avatar.get('circular'), 'true')
        self.assertEqual(avatar.get('height'), '26')
        self.assertEqual(avatar.get('width'), '26')
        self.assertEqual(name.get('font.style'), 'BOLD')
        self.assertEqual(name.get('use_ellipses'), 'true')
        self.assertEqual(name.get('height'), '34')
        self.assertEqual(time.get('halign'), 'right')
        self.assertEqual(time.get('follows'), 'right')
        self.assertEqual(time.get('width'), '72')
        # Narrower timestamp reservation leaves additional space for names.
        self.assertEqual(layout.get('right'), '-84')

    def test_ogl_chat_item_has_round_avatar_and_theme_aware_wrapped_text(self):
        base = ROOT / 'indra/newview/skins/default/xui/en'
        root = ET.parse(base / 'panel_chat_item.xml').getroot()
        avatar = root.find('.//avatar_icon')
        text = root.find('.//text_chat[@name="msg_text"]')
        self.assertEqual(avatar.get('circular'), 'true')
        self.assertEqual(avatar.get('width'), '26')
        self.assertEqual(avatar.get('height'), '26')
        self.assertEqual(text.get('left'), '38')
        self.assertEqual(text.get('right'), '-8')
        self.assertEqual(text.get('word_wrap'), 'true')
        self.assertEqual(text.get('text_color'), 'ChatHistoryTextColor')

    def test_ogl_nearby_chat_window_is_readable_and_resizable(self):
        base = ROOT / 'indra/newview/skins/default/xui/en'
        root = ET.parse(base / 'floater_fs_nearby_chat.xml').getroot()
        self.assertEqual(root.get('width'), '440')
        self.assertEqual(root.get('height'), '392')
        self.assertEqual(root.get('min_width'), '300')
        self.assertEqual(root.get('min_height'), '220')
        self.assertEqual(root.get('can_resize'), 'true')
        histories = {e.get('name'): e for e in root.iter('fs_chat_history')}
        for name in ('chat_history', 'chat_history_muted'):
            self.assertEqual(histories[name].get('font'), 'SansSerif')
            self.assertEqual(histories[name].get('follows'), 'all')
        editor = next(e for e in root.iter('chat_editor') if e.get('name') == 'chat_box')
        self.assertEqual(editor.get('font'), 'SansSerif')
        self.assertEqual(editor.get('height'), '24')
        self.assertEqual(editor.get('wrap'), 'true')
        self.assertEqual(editor.get('spellcheck'), 'true')
        self.assertEqual(editor.get('is_expandable'), 'true')
        self.assertEqual(root.find('.//layout_panel[@name="chat_layout_panel"]').get('height'), '30')
        channel = next(e for e in root.iter('spinner') if e.get('name') == 'ChatChannel')
        self.assertNotIn('Firestorm', channel.get('tool_tip', ''))
        for child in ('chat_options_btn', 'chat_search_btn', 'chat_history_btn'):
            self.assertTrue(any(e.get('name') == child for e in root.iter()), child)

    def test_ogl_direct_and_group_chat_use_same_readable_message_style(self):
        base = ROOT / 'indra/newview/skins/default/xui/en'
        root = ET.parse(base / 'floater_fs_im_session.xml').getroot()
        self.assertEqual(root.get('width'), '460')
        self.assertEqual(root.get('height'), '392')
        self.assertEqual(root.get('min_width'), '320')
        self.assertEqual(root.get('min_height'), '220')
        self.assertEqual(root.get('can_resize'), 'true')
        stack = root.find('.//layout_stack[@name="chat_stack"]')
        self.assertEqual(stack.get('width'), '460')
        self.assertEqual(stack.get('height'), '368')
        history = next(e for e in root.iter('fs_chat_history')
                       if e.get('name') == 'chat_history')
        self.assertEqual(history.get('font'), 'SansSerif')
        self.assertEqual(history.get('follows'), 'all')
        editor = next(e for e in root.iter('chat_editor') if e.get('name') == 'chat_editor')
        self.assertEqual(editor.get('font'), 'SansSerif')
        self.assertEqual(editor.get('height'), '24')
        self.assertEqual(editor.get('spellcheck'), 'true')
        self.assertEqual(editor.get('wrap'), 'true')
        self.assertEqual(root.find('.//layout_panel[@name="chat_layout_panel"]').get('height'), '30')
        for name in ('view_profile_btn', 'group_info_btn', 'chat_options_btn'):
            self.assertTrue(any(e.get('name') == name for e in root.iter()), name)

    def test_ogl_conversation_container_keeps_tab_and_session_navigation(self):
        base = ROOT / 'indra/newview/skins/default/xui/en'
        root = ET.parse(base / 'floater_fs_im_container.xml').getroot()
        tabs = root.find('.//tab_container[@name="im_box_tab_container"]')
        self.assertEqual(root.get('width'), '500')
        self.assertEqual(root.get('height'), '432')
        self.assertEqual(root.get('can_resize'), 'true')
        self.assertEqual(tabs.get('width'), '498')
        self.assertEqual(tabs.get('height'), '432')
        self.assertEqual(tabs.get('follows'), 'left|right|top|bottom')
        self.assertEqual(tabs.get('tab_width'), '132')
        self.assertEqual(tabs.get('tab_max_width'), '132')
        self.assertEqual(tabs.get('tab_position'), 'left')
        self.assertEqual(tabs.get('open_tabs_on_drag_and_drop'), 'true')
        for tag in ('first_tab', 'middle_tab', 'last_tab'):
            self.assertIsNotNone(tabs.find(tag), tag)

    def test_upstream_copyright_still_present(self):
        self.assertIn("Linden Research, Inc.",
                      self.source("indra/llcommon/indra_constants.h"))
        self.assertIn("FirestormViewer/phoenix-firestorm",
                      self.source("ogl/README.md"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
