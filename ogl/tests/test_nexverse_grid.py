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
        self.assertIn('grid_builder_text->setClickedCallback(onClickGridBuilder, NULL)', self.panel)

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

    def test_ogl_never_downloads_untrusted_legacy_grid_status_feed(self):
        startup=self.startup
        guard='if (APP_NAME != "OGL" && gSavedSettings.getBOOL("AutoQueryGridStatus"))'
        self.assertIn(guard,startup)
        start=startup.index(guard)
        end=startup.index('// </FS:PP>',start)
        body=startup[start:end]
        self.assertIn('FSCoreHttpUtil::callbackHttpGetRaw(',body)
        self.assertIn('gSavedSettings.getString("AutoQueryGridStatusURL")',body)
        self.assertLess(body.index('APP_NAME != "OGL"'),
                        body.index('FSCoreHttpUtil::callbackHttpGetRaw('))
        # A persisted true value from a prior installation still
        # cannot bypass the OGL-specific condition.
        self.assertNotIn('if (gSavedSettings.getBOOL("AutoQueryGridStatus"))',body)

    def test_direct_login_button_rechecks_grid_before_any_credentials(self):
        start = self.panel.index('void FSPanelLogin::onClickConnect(void *)')
        end = self.panel.index('void FSPanelLogin::onClickNewAccount(void*)', start)
        callback = self.panel[start:end]
        for guard in ('combo_val.isUndefined()', 'OGLNexVerseGrid::isFixedGrid(combo_val.asString())',
                      'Blocked unlisted grid in login request.',
                      'combo_val.asString() != LLGridManager::getInstance()->getGrid()',
                      'sInstance->clearLoginFieldsForGridChange()',
                      'Grid switched before login; re-enter credentials.'):
            self.assertIn(guard, callback)
        secret_read = callback.index('std::string password =')
        self.assertLess(callback.index('Blocked unlisted grid in login request.'), secret_read)
        self.assertLess(callback.index('sInstance->clearLoginFieldsForGridChange()'), secret_read)
        self.assertLess(callback.index('Blocked beta authentication:'), secret_read)
        self.assertIn('OGLGridChangedReenterCredentials', callback)
        for lang in ('en', 'de'):
            strings = ET.parse(ROOT / f'indra/newview/skins/default/xui/{lang}/strings.xml')
            matches = [n for n in strings.iter('string')
                       if n.get('name') == 'OGLGridChangedReenterCredentials']
            self.assertEqual(len(matches), 1)
            self.assertGreater(len((matches[0].text or '').strip()), 30)

    def test_saved_account_removal_dialog_is_bound_to_current_login_form(self):
        begin = self.panel.index('void FSPanelLogin::onClickRemove(void*)')
        end = self.panel.index('void FSPanelLogin::onRemoveCallback(', begin)
        creator = self.panel[begin:end]
        self.assertIn('selected_credential.isUndefined()', creator)
        self.assertIn('OGLNexVerseGrid::isFixedGrid(', creator)
        self.assertIn('++sOGLRemovalConfirmationSequence;', creator)
        for key in ('OGLFormRevision', 'OGLConfirmationSequence',
                    'OGLSelectedGrid', 'CredName'):
            self.assertIn(key, creator)
        self.assertLess(creator.index('OGLNexVerseGrid::isFixedGrid('),
                        creator.index('ConfirmRemoveCredential'))

    def test_stale_removal_cannot_touch_protected_credential_store(self):
        start = self.panel.index('void FSPanelLogin::onRemoveCallback(')
        end = self.panel.index('void FSPanelLogin::onClickGridMgrHelp', start)
        method = self.panel[start:end]
        for guard in ('selected.isUndefined()', 'selected.asString() != credName',
                      'current_grid != credName.substr(sep + 1)',
                      'payload["OGLSelectedGrid"].asString() != current_grid',
                      'payload["OGLFormRevision"].asString() != std::to_string(sOGLLoginFormRevision)',
                      'payload["OGLConfirmationSequence"].asString() != std::to_string(sOGLRemovalConfirmationSequence)',
                      'Ignored stale saved-login deletion confirmation.'):
            self.assertIn(guard,method)
        self.assertLess(method.index('Ignored stale saved-login deletion confirmation.'),
                        method.index('gSecAPIHandler->loadCredential(credName)'))
        self.assertLess(method.index('++sOGLRemovalConfirmationSequence;'),
                        method.index('gSecAPIHandler->loadCredential(credName)'))
        self.assertLess(method.index('if (credential.isNull())'),
                        method.index('gSecAPIHandler->deleteCredential(credential)'))
        self.assertIn('++sOGLLoginFormRevision;',method)
        self.assertIn('OGLNexVerseGrid::consumeOneTimeHTTPLoginApproval();',method)

    def test_confirmed_account_removal_cannot_delete_foreign_grid(self):
        start = self.panel.index('void FSPanelLogin::onRemoveCallback(')
        end = self.panel.index('void FSPanelLogin::onClickGridMgrHelp', start)
        callback = self.panel[start:end]
        for guard in ('if (option == 0)', 'OGLNexVerseGrid::isFixedGrid(credName.substr(sep + 1))',
                      'Refused removal of unlisted-grid credential.',
                      'if (credential.isNull())', 'Saved login no longer exists.'):
            self.assertIn(guard, callback)
        self.assertLess(callback.index('Refused removal of unlisted-grid credential.'),
                        callback.index('gSecAPIHandler->loadCredential(credName)'))
        self.assertLess(callback.index('if (credential.isNull())'),
                        callback.index('credential->userID()'))
        self.assertLess(callback.index('if (credential.isNull())'),
                        callback.index('gSavedSettings.getControl("UserLoginInfo")->resetToDefault()'))
        self.assertLess(callback.index('if (credential.isNull())'),
                        callback.index('gSecAPIHandler->deleteCredential(credential)'))

    def test_credentials_are_reused_only_after_exact_saved_account_selection(self):
        start=self.panel.index('void FSPanelLogin::getFields(')
        end=self.panel.index('bool FSPanelLogin::areCredentialFieldsDirty(',start)
        body=self.panel[start:end]
        self.assertIn('OGLNexVerseGrid::canReuseSavedAuthenticator(',body)
        self.assertIn('getChild<LLComboBox>("username_combo")->getSelectedValue()',body)
        self.assertIn('!selected_account.isUndefined()',body)
        self.assertIn('sInstance->mPasswordModified',body)
        self.assertLess(body.index('canReuseSavedAuthenticator('),
                        body.index('gSecAPIHandler->loadCredential(requested_credential)'))
        self.assertIn('if (use_saved_authenticator)',body)
        self.assertNotIn('gSecAPIHandler->loadCredential(credentialName())',body)

    def test_invalid_saved_selection_cannot_login_with_masked_filler(self):
        start=self.panel.index('void FSPanelLogin::getFields(')
        end=self.panel.index('bool FSPanelLogin::areCredentialFieldsDirty(',start)
        body=self.panel[start:end]
        self.assertIn('if (!use_saved_authenticator && !sInstance->mPasswordModified)',body)
        reject=body.index('Saved-login selection is no longer valid; enter a password.')
        self.assertLess(reject,body.index('gSecAPIHandler->loadCredential(requested_credential)'))
        self.assertIn('credential = nullptr;',body[reject:body.index('if (use_saved_authenticator)')])
        self.assertIn('return;',body[reject:body.index('if (use_saved_authenticator)')])

    def test_disappeared_saved_password_fails_closed_without_network_auth(self):
        start=self.panel.index('void FSPanelLogin::getFields(')
        end=self.panel.index('bool FSPanelLogin::areCredentialFieldsDirty(',start)
        body=self.panel[start:end]
        self.assertIn('if (APP_NAME == "OGL" && credential.isNull())',body)
        self.assertIn('Selected saved login is unavailable; enter a password.',body)
        self.assertLess(body.index('if (APP_NAME == "OGL" && credential.isNull())'),
                        body.index('credential->getAuthenticator()'))
        connect=self.panel[self.panel.index('void FSPanelLogin::onClickConnect(void *)'):
                           self.panel.index('void FSPanelLogin::onClickNewAccount(void*)')]
        self.assertIn('if (cred.isNull())',connect)
        self.assertIn('OGLNexVerseGrid::consumeOneTimeHTTPLoginApproval();',connect)

    def test_manual_password_never_uses_previous_credential_pointer(self):
        start=self.panel.index('void FSPanelLogin::getFields(')
        end=self.panel.index('bool FSPanelLogin::areCredentialFieldsDirty(',start)
        body=self.panel[start:end]
        reset=body.index('credential = nullptr;',
                         body.index('A manually entered password'))
        self.assertLess(reset, body.index('LLSD authenticator = LLSD::emptyMap();'))
        self.assertIn('if (FSPanelLogin::sInstance->mPasswordModified)',body)
        self.assertIn('authenticator["type"] = CRED_AUTHENTICATOR_TYPE_CLEAR;',body)
        self.assertIn('authenticator["type"] = CRED_AUTHENTICATOR_TYPE_HASH;',body)
        self.assertIn('credential = gSecAPIHandler->createCredential(',body)

    def test_saved_authenticator_native_policy_uses_canonical_grid_names(self):
        header=(ROOT/'indra/newview/oglnexversegrid.h').read_text(encoding='utf-8')
        self.assertIn('inline bool canReuseSavedAuthenticator(',header)
        self.assertIn('selectedCredential != requestedCredential',header)
        self.assertIn('!hasSavedSelection || passwordModified',header)
        self.assertIn('(currentGrid != kKey && currentGrid != kBetaKey)',header)
        self.assertIn('requestedCredential.substr(separator + 1) == currentGrid',header)
        native=(ROOT/'ogl/tests/test_grid_http_policy.cpp').read_text(encoding='utf-8')
        self.assertGreaterEqual(native.count('canReuseSavedAuthenticator('),16)
        self.assertIn('assert(canReuseSavedAuthenticator(kKey, mainAccount, mainAccount, true, false))',native)
        self.assertIn('assert(!canReuseSavedAuthenticator(kKey, mainAccount, mainAccount, true, true))',native)

    def test_credential_restoration_rejects_cross_grid_or_unpinned_secret(self):
        start = self.panel.index('void FSPanelLogin::setFields(')
        end = self.panel.index('void FSPanelLogin::getFields(', start)
        code = self.panel[start:end]
        self.assertIn('OGLNexVerseGrid::isFixedGrid(name.substr(separator + 1))', code)
        self.assertIn('name.substr(separator + 1) != LLGridManager::getInstance()->getGrid()', code)
        self.assertIn('Ignored credential from non-current grid.', code)
        self.assertLess(code.index('Ignored credential from non-current grid.'),
                        code.index('identifier = credential->getIdentifier()'))
        self.assertIn('sInstance->clearLoginFieldsForGridChange()', code)
        self.assertLess(code.index('Ignored credential from non-current grid.'),
                        code.index('sCredentialSet = true'))
        self.assertIn('return;', code[code.index('Ignored credential from non-current grid.'):
                                      code.index('identifier = credential->getIdentifier()')])

    def test_editing_saved_username_clears_password_before_connect(self):
        start = self.panel.index('void FSPanelLogin::onUsernameTextChanged()')
        end = self.panel.index('void FSPanelLogin::onModeChange(', start)
        body = self.panel[start:end]
        self.assertIn('current_name != mPreviousUsername && mPasswordLength != 0', body)
        self.assertIn('getChild<LLLineEditor>("password_edit")->clear()', body)
        self.assertIn('sPassword.clear()', body)
        self.assertIn('mPasswordLength = 0;', body)
        self.assertIn('mPasswordModified = false;', body)
        self.assertIn('getChild<LLButton>("remove_user_btn")->setEnabled(false)', body)
        self.assertLess(body.index('password_edit")->clear()'), body.index('updateLoginButtons()'))
        selected = self.panel[
            self.panel.index('void FSPanelLogin::onSelectUser()'):
            self.panel.index('void FSPanelLogin::updateServerCombo()')]
        self.assertIn('APP_NAME == "OGL" || !sInstance->mPasswordModified', selected)
        self.assertIn('Rejected saved account from unlisted grid.', selected)
        self.assertLess(selected.index('Rejected saved account from unlisted grid.'),
                        selected.index('sInstance->clearLoginFieldsForGridChange();'))

    def test_grid_change_does_not_automatically_restore_target_password(self):
        start = self.panel.index('void FSPanelLogin::updateServer()')
        end = self.panel.index('void FSPanelLogin::updateLoginButtons()', start)
        method = self.panel[start:end]
        self.assertIn('APP_NAME != "OGL" && !sInstance->areCredentialFieldsDirty()', method)
        self.assertLess(method.index('APP_NAME != "OGL"'),
                        method.index('gSecAPIHandler->loadCredential(credentialName())'))
        self.assertIn('updateServerCombo();', method)
        self.assertIn('loadLoginPage();', method)
        self.assertIn('setMaxTextChars(', method)
        # Previously a clean login form could silently refill the password
        # right after clearLoginFieldsForGridChange().
        self.assertNotIn('if(!sInstance->areCredentialFieldsDirty())', method)

    def test_internal_location_link_rejects_invalid_coordinates(self):
        start = self.panel.index('class LLLoginLocationAutoHandler')
        end = self.panel.index('LLLoginLocationAutoHandler gLoginLocationAutoHandler;', start)
        body = self.panel[start:end]
        self.assertIn('#include <cmath>', self.panel)
        self.assertEqual(body.count('Rejected invalid location coordinates.'), 2)
        self.assertIn('LLStringUtil::convertToF32(tokens[1].asString(), xpos)', body)
        self.assertIn('LLStringUtil::convertToF32(tokens[2].asString(), ypos)', body)
        self.assertIn('LLStringUtil::convertToF32(tokens[3].asString(), zpos)', body)
        self.assertIn('std::isfinite(xpos)', body)
        self.assertIn('std::isfinite(ypos)', body)
        self.assertIn('std::isfinite(zpos)', body)
        self.assertNotIn('codec >> xpos;', body)
        self.assertNotIn('codec >> ypos;', body)
        self.assertNotIn('codec >> zpos;', body)

    def test_embedded_location_link_does_not_silently_log_in(self):
        start = self.panel.index('void FSPanelLogin::autologinToLocation(')
        end = self.panel.index('void FSPanelLogin::closePanel()', start)
        callback = self.panel[start:end]
        ogl = callback[callback.index('if (APP_NAME == "OGL")'):
                       callback.index('LL_DEBUGS("AppInit")')]
        self.assertIn('slurl.getType() != LLSLURL::LOCATION', ogl)
        self.assertIn('OGLNexVerseGrid::isFixedGrid(destination_grid)', ogl)
        self.assertIn('LLStartUp::setStartSLURL(slurl)', ogl)
        self.assertIn('++sOGLLoginFormRevision', ogl)
        self.assertIn('manual login required', ogl)
        self.assertNotIn('onClickConnect', ogl)
        self.assertNotIn('mCallback', ogl)
        self.assertIn('return;', ogl)
        self.assertIn('onClickConnect(unused_parameter)', callback)
        self.assertLess(callback.index('return;', callback.index('manual login required')),
                        callback.index('onClickConnect(unused_parameter)'))

    def test_saved_cross_grid_account_never_loads_secret_before_switch(self):
        start = self.panel.index('void FSPanelLogin::onSelectUser()')
        end = self.panel.index('void FSPanelLogin::updateServerCombo()', start)
        callback = self.panel[start:end]
        switch = callback.index('selected_grid != LLGridManager::getInstance()->getGrid()')
        clear = callback.index('sInstance->clearLoginFieldsForGridChange()', switch)
        load = callback.index('gSecAPIHandler->loadCredential(cred_name)')
        self.assertLess(switch, load)
        self.assertLess(clear, load)
        self.assertIn('OGLGridChangedReenterCredentials', callback)
        self.assertIn('return;', callback[clear:load])
        self.assertIn('APP_NAME != "OGL" &&', callback)
        self.assertIn('Saved account changed the login world; fields reset.', callback)

    def test_disappeared_saved_login_is_rejected_without_password(self):
        start = self.panel.index('void FSPanelLogin::onSelectUser()')
        end = self.panel.index('void FSPanelLogin::updateServerCombo()', start)
        callback = self.panel[start:end]
        self.assertIn('OGLNexVerseGrid::isFixedGrid(cred_name.substr(separator + 1))', callback)
        self.assertIn('Selected saved login was unavailable.', callback)
        self.assertIn('sInstance->clearLoginFieldsForGridChange()', callback)
        self.assertLess(callback.index('if (credential.isNull())'),
                        callback.index('sInstance->setFields(credential)'))
        self.assertLess(callback.index('if (credential.isNull())'),
                        callback.index('sInstance->getChild<LLButton>("remove_user_btn")->setEnabled(true)'))

    def test_empty_login_is_rejected_before_http_consent(self):
        callback = self.panel[
            self.panel.index('void FSPanelLogin::onClickConnect(void *)'):
            self.panel.index('void FSPanelLogin::onClickNewAccount(void*)')]
        self.assertIn('LLStringUtil::trim(entered_name)', callback)
        self.assertLess(callback.index('LLNotificationsUtil::add("MustHaveAccountToLogIn")'),
                        callback.index('OGLHTTPLoginWarning'))
        self.assertLess(callback.index('LLNotificationsUtil::add("MustEnterPasswordToLogIn")'),
                        callback.index('OGLHTTPLoginWarning'))
        self.assertLess(callback.index('LLStringUtil::trim(entered_name)'),
                        callback.index('OGLHTTPLoginWarning'))
        self.assertIn('if (cred.isNull())', callback)
        self.assertLess(callback.index('if (cred.isNull())'),
                        callback.index('cred->identifierType(identifier_type)'))
        self.assertIn('Unable to create login credential.', callback)

    def test_beta_credentials_never_sent_from_login_button(self):
        blocked = self.panel.index('Blocked beta authentication: protocol and TLS not verified.')
        credentials = self.panel.index('std::string username = sInstance->getChild<LLUICtrl>("username_combo")', blocked)
        self.assertLess(blocked, credentials)
        self.assertIn('OGLNexVerseGrid::isBetaGrid(LLGridManager::getInstance()->getGrid())', self.panel)
        self.assertIn('LLTrans::getString("OGLBetaLoginUnavailable")', self.panel)

    def test_http_once_permission_expires_on_login_form_change(self):
        source = self.panel
        self.assertEqual(source.count('++sOGLLoginFormRevision;'), 11)
        self.assertGreaterEqual(
            source.count('OGLNexVerseGrid::consumeOneTimeHTTPLoginApproval();'), 12)
        for field in ('password edited', 'loading saved credentials',
                      'changing destination invalidates consent'):
            i = source.index(field)
            self.assertIn('consumeOneTimeHTTPLoginApproval();', source[i-90:i+180])
        callback = source[source.index('void FSPanelLogin::onClickConnect(void *)'):
                          source.index('void FSPanelLogin::onClickNewAccount(void*)')]
        self.assertIn('InvalidCredentialFormat', callback)
        self.assertGreaterEqual(callback.count('consumeOneTimeHTTPLoginApproval();'), 2)
        for language, expected in (('en','Continue once'),('de','Einmal fortfahren')):
            notes = ET.parse(ROOT / f'indra/newview/skins/default/xui/{language}/notifications.xml')
            warning = next(n for n in notes.iter('notification') if n.get('name')=='OGLHTTPLoginWarning')
            self.assertIn(expected,warning.text)
            self.assertEqual(next(b.get('text') for b in warning.iter('button')
                                  if b.get('name')=='OGLHTTPConnectOnce'),expected)

    def test_http_once_approval_is_consumed_at_authentication_boundary(self):
        startup = self.startup
        begin = startup.index('Blocked beta authentication: no verified protocol/TLS adapter.')
        connect = startup.index('login->connect(gUserCredential)', begin)
        consumed = startup.index('OGLNexVerseGrid::consumeOneTimeHTTPLoginApproval()', connect)
        self.assertLess(connect, consumed)
        self.assertLess(consumed, startup.index('LLGridManager::getInstance()->saveGridList()', connect))
        for diagnostic in ('Blocked beta authentication: no verified protocol/TLS adapter.',
                           'Rejected login: unpinned grid endpoint.',
                           'Rejected login: explicit fixed-grid HTTP consent required.'):
            pos = startup.index(diagnostic)
            self.assertIn('OGLNexVerseGrid::consumeOneTimeHTTPLoginApproval()',
                          startup[pos-160:pos])
        self.assertIn('consumeOneTimeHTTPLoginApproval()', self.values)
        self.assertIn('gHTTPLoginApprovedForNextAttempt = false;', self.values)
        self.assertIn('rememberedURI == kLoginURI', self.values)

    def test_beta_credentials_never_sent_from_auto_login(self):
        guard = self.startup.index('Blocked beta authentication: no verified protocol/TLS adapter.')
        call = self.startup.index('login->connect(gUserCredential)')
        self.assertLess(guard, call)
        self.assertIn('OGLNexVerseGrid::isBetaGrid(LLGridManager::getInstance()->getGrid())', self.startup)
        self.assertIn('gSavedSettings.setBOOL("AutoLogin", false)', self.startup)
        self.assertIn('transition_back_to_login_panel("")', self.startup)

    def test_public_http_exception_remains_exact_and_opt_in(self):
        self.assertIn('gHTTPLoginApprovedForNextAttempt', self.values)
        self.assertIn('isExactPinnedHTTPLogin(', self.values)
        self.assertIn('rememberedURI == kLoginURI', self.values)
        self.assertIn('isApprovedPinnedHTTPLogin(', self.startup)
        self.assertIn('consented_pinned_http', self.startup)
        self.assertIn('OGLHTTPLoginWarning', self.panel)
        self.assertIn('OGLRememberPinnedHTTPLoginURI', self.panel)
        self.assertIn('option == 0 || option == 1', self.panel)
        self.assertIn('if (option == 1)', self.panel)
        self.assertIn('Rejected login: explicit fixed-grid HTTP consent required', self.startup)
        self.assertLess(self.startup.index('Rejected login: explicit fixed-grid HTTP consent required'),
                        self.startup.index('login->connect(gUserCredential)'))

    def test_https_login_endpoint_is_not_trusted_solely_because_of_tls(self):
        panel = self.panel[
            self.panel.index('void FSPanelLogin::onClickConnect(void *)'):
            self.panel.index('void FSPanelLogin::onClickNewAccount(void*)')]
        startup = self.startup[
            self.startup.index('Blocked beta authentication: no verified protocol/TLS adapter.'):
            self.startup.index('login->connect(gUserCredential)')]
        self.assertIn('isExactPinnedHTTPLogin(', panel)
        self.assertIn('Blocked unpinned grid login endpoint.', panel)
        self.assertIn('isExactPinnedHTTPLogin(', startup)
        self.assertIn('Rejected login: unpinned grid endpoint.', startup)
        self.assertIn('gSavedSettings.setBOOL("AutoLogin", false)', startup)
        self.assertNotIn('bool all_https', startup)
        self.assertNotIn('bool secure_login', panel)
        self.assertLess(startup.index('Rejected login: unpinned grid endpoint.'),
                        startup.index('consented_pinned_http'))
        self.assertLess(panel.index('Blocked unpinned grid login endpoint.'),
                        panel.index('OGLHTTPLoginWarning'))
        self.assertIn('OGLUnexpectedGridEndpoint', panel)
        self.assertIn('OGLUnexpectedGridEndpoint', startup)

    def test_delayed_http_approval_cannot_outlive_account_or_grid_change(self):
        panel = self.panel[
            self.panel.index('void FSPanelLogin::onClickConnect(void *)'):
            self.panel.index('void FSPanelLogin::onClickNewAccount(void*)')]
        self.assertIn('const std::string consent_grid =', panel)
        self.assertIn('const std::string consent_account =', panel)
        self.assertIn('[consent_grid, consent_account, consent_revision, consent_sequence]', panel)
        self.assertIn('Ignored stale HTTP login approval.', panel)
        self.assertIn('sInstance->getChild<LLComboBox>("server_combo")->getSelectedValue().asString() != consent_grid', panel)
        self.assertIn('getValue().asString() != consent_account', panel)
        self.assertIn('isExactPinnedHTTPLogin(consent_grid, current_uris)', panel)
        self.assertLess(panel.index('Ignored stale HTTP login approval.'),
                        panel.index('gHTTPLoginApprovedForNextAttempt = true'))
        self.assertLess(panel.index('Ignored stale HTTP login approval.'),
                        panel.index('gSavedSettings.setString('))
        for lang in ('en', 'de'):
            tree = ET.parse(ROOT / f'indra/newview/skins/default/xui/{lang}/strings.xml')
            matches = [e for e in tree.iter('string')
                       if e.get('name') == 'OGLUnexpectedGridEndpoint']
            self.assertEqual(len(matches), 1)
            self.assertGreater(len((matches[0].text or '').strip()), 60)

    def test_only_most_recent_http_dialog_can_authorize(self):
        panel=self.panel
        begin=panel.index('void FSPanelLogin::onClickConnect(void *)')
        end=panel.index('void FSPanelLogin::onClickNewAccount(void*)', begin)
        connect=panel[begin:end]
        self.assertIn('static U64 sOGLHTTPWarningSequence = 0;',panel)
        self.assertIn('const U64 consent_sequence = ++sOGLHTTPWarningSequence;',connect)
        self.assertIn('[consent_grid, consent_account, consent_revision, consent_sequence]',connect)
        self.assertIn('sOGLHTTPWarningSequence != consent_sequence',connect)
        self.assertLess(connect.index('sOGLHTTPWarningSequence != consent_sequence'),
                        connect.index('gHTTPLoginApprovedForNextAttempt = true;'))
        self.assertLess(connect.index('sOGLHTTPWarningSequence != consent_sequence'),
                        connect.index('gSavedSettings.setString('))
        self.assertLess(connect.index('++sOGLHTTPWarningSequence;',connect.index('Ignored stale HTTP login approval.')),
                        connect.index('gHTTPLoginApprovedForNextAttempt = true;'))

    def test_one_time_http_approval_cannot_be_replayed(self):
        connect=self.panel[self.panel.index('void FSPanelLogin::onClickConnect(void *)'):
                           self.panel.index('void FSPanelLogin::onClickNewAccount(void*)')]
        self.assertEqual(connect.count('const U64 consent_sequence = ++sOGLHTTPWarningSequence;'),1)
        # One increment creates the dialog, a second invalidates it before
        # granting either one-time or persistent permission.
        self.assertEqual(connect.count('++sOGLHTTPWarningSequence;'),2)
        self.assertLess(connect.index('++sOGLHTTPWarningSequence;',
                            connect.index('Ignored stale HTTP login approval.')),
                        connect.index('if (option == 1)'))
        self.assertLess(connect.index('if (option == 1)'),
                        connect.index('gHTTPLoginApprovedForNextAttempt = true;'))
        self.assertIn('sOGLLoginFormRevision != consent_revision',connect)
        self.assertNotIn('consent_password',connect)
        self.assertNotIn('consent_secret',connect)

    def test_cancelling_mode_change_never_resets_toolbars(self):
        source=self.panel
        begin=source.index('void FSPanelLogin::onModeChange(')
        confirm=source.index('void FSPanelLogin::onModeChangeConfirm(',begin)
        original=source[begin:confirm]
        end=len(source)
        callback=source[confirm:end]
        self.assertNotIn('ResetToolbarSettings',original)
        self.assertIn('original_value.asString() != new_value.asString()',original)
        self.assertIn('LLNotificationsUtil::add("ModeChange"',original)
        accept=callback.index('case 0:')
        decline=callback.index('case 1:')
        self.assertLess(accept,decline)
        accepted=callback[accept:decline]
        self.assertIn('gSavedSettings.getBOOL("FSToolbarsResetOnModeChange")',accepted)
        self.assertIn('gSavedSettings.setBOOL("ResetToolbarSettings", true)',accepted)
        self.assertLess(accepted.index('ResetToolbarSettings", true'),
                        accepted.index('getControl("SessionSettingsFile")->set(new_value)'))
        self.assertLess(accepted.index('getControl("SessionSettingsFile")->set(new_value)'),
                        accepted.index('requestQuit()'))
        self.assertNotIn('ResetToolbarSettings',callback[decline:])
        self.assertIn('setValue(original_value)',callback[decline:])

    def test_mode_change_dialog_generations_are_monotonic(self):
        panel=self.panel
        begin=panel.index('void FSPanelLogin::onModeChange(')
        end=panel.index('void FSPanelLogin::onModeChangeConfirm(',begin)
        maker=panel[begin:end]
        self.assertIn('static U64 sOGLModeConfirmationSequence = 0;', panel)
        self.assertIn('if (original_value.asString() != new_value.asString())', maker)
        self.assertIn('if (APP_NAME == "OGL")',maker)
        self.assertIn('payload["OGLModeConfirmationSequence"]',maker)
        self.assertIn('std::to_string(++sOGLModeConfirmationSequence)',maker)
        self.assertIn('LLNotificationsUtil::add("ModeChange", LLSD(), payload,',maker)
        self.assertNotIn('ResetToolbarSettings',maker)

    def test_outdated_mode_confirmation_cannot_modify_settings(self):
        panel=self.panel
        begin=panel.index('void FSPanelLogin::onModeChangeConfirm(')
        callback=panel[begin:]
        for check in ('notification["payload"]["OGLModeConfirmationSequence"].asString()',
                      'std::to_string(sOGLModeConfirmationSequence)',
                      '!sInstance',
                      'getChild<LLUICtrl>("mode_combo")->getValue().asString() != new_value.asString()',
                      'gSavedSettings.getString("SessionSettingsFile") != original_value.asString()',
                      'Ignored stale mode-change confirmation.'):
            self.assertIn(check,callback)
        self.assertLess(callback.index('Ignored stale mode-change confirmation.'),
                        callback.index('ResetToolbarSettings", true'))
        self.assertLess(callback.index('Ignored stale mode-change confirmation.'),
                        callback.index('getControl("SessionSettingsFile")->set(new_value)'))
        self.assertLess(callback.index('Ignored stale mode-change confirmation.'),
                        callback.index('getChild<LLUICtrl>("mode_combo")->setValue(original_value)'))

    def test_mode_confirmation_is_consumed_before_accept_or_decline(self):
        body=self.panel[self.panel.index('void FSPanelLogin::onModeChangeConfirm('):]
        self.assertIn('++sOGLModeConfirmationSequence;',body)
        self.assertLess(body.index('++sOGLModeConfirmationSequence;'),
                        body.index('getSelectedOption(notification, response)'))
        self.assertLess(body.index('++sOGLModeConfirmationSequence;'),
                        body.index('switch (option)'))
        self.assertLess(body.index('getSelectedOption(notification, response)'),
                        body.index('case 0:'))
        self.assertIn('if (gSavedSettings.getBOOL("FSToolbarsResetOnModeChange"))',body)
        self.assertIn('getChild<LLUICtrl>("mode_combo")->setValue(original_value)',body)
        self.assertNotIn('ResetToolbarSettings', body[body.index('case 1:'):])

    def test_privacy_and_teleport_changes_invalidate_old_http_consent(self):
        source = self.panel
        start = source.index('getChild<LLUICtrl>("remember_username_check")->setCommitCallback(')
        end = source.index('mPreviousUsername = username_combo->getValue().asString();', start)
        form = source[start:end]
        self.assertIn('++sOGLLoginFormRevision;', form)
        self.assertIn('getChild<LLUICtrl>("remember_check")->setCommitCallback(', form)
        self.assertGreaterEqual(form.count('++sOGLLoginFormRevision;'), 2)
        start = source.index('void FSPanelLogin::onLocationSLURL()')
        end = source.index('/////////////////////////////', start)
        self.assertIn('++sOGLLoginFormRevision; // changing destination', source[start:end])
        connect = source[source.index('void FSPanelLogin::onClickConnect(void *)'):
                         source.index('void FSPanelLogin::onClickNewAccount(void*)')]
        self.assertIn('sOGLLoginFormRevision != consent_revision', connect)

    def test_password_edits_invalidate_pending_http_consent_without_capturing_secret(self):
        panel = self.panel
        self.assertIn('static U64 sOGLLoginFormRevision = 0;', panel)
        self.assertIn('const U64 consent_revision = sOGLLoginFormRevision;', panel)
        self.assertIn('[consent_grid, consent_account, consent_revision, consent_sequence]', panel)
        self.assertIn('sOGLLoginFormRevision != consent_revision', panel)
        self.assertNotIn('consent_password', panel)
        self.assertNotIn('consent_secret', panel)
        self.assertIn('++sOGLLoginFormRevision; // password edited', panel)
        self.assertIn('++sOGLLoginFormRevision; // loading saved credentials', panel)
        self.assertIn('++sOGLLoginFormRevision; // invalidate approvals from older panels', panel)
        for hook in ('void FSPanelLogin::onUsernameTextChanged()',
                     'void FSPanelLogin::clearLoginFieldsForGridChange()',
                     'void FSPanelLogin::onSelectUser()'):
            part = panel[panel.index(hook):]
            self.assertIn('++sOGLLoginFormRevision;', part[:2200])

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
