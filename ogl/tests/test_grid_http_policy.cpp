#include "../../indra/newview/oglnexversegrid.h"

// Assertions must remain active for these security tests in Release CI.
// In normal Release builds NDEBUG would otherwise silently skip every check.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <vector>
#include <string>

int main() {
    using OGLNexVerseGrid::isExactPinnedHTTPLogin;
    using OGLNexVerseGrid::kKey;
    using OGLNexVerseGrid::kLoginURI;
    assert(isExactPinnedHTTPLogin(kKey, {kLoginURI}));
    assert(!isExactPinnedHTTPLogin("other-grid.example", {kLoginURI}));
    assert(!isExactPinnedHTTPLogin(kKey, {}));
    assert(!isExactPinnedHTTPLogin(kKey, {"http://evil.example/"}));
    assert(!isExactPinnedHTTPLogin(kKey, {kLoginURI, "http://evil.example/"}));
    assert(!isExactPinnedHTTPLogin(kKey, {"http://hg.stadt-nexverse.de/"}));
    assert(!isExactPinnedHTTPLogin(kKey, {"https://hg.stadt-nexverse.de/"}));
    // Even perfectly formed TLS links are hostile unless the exact endpoint
    // and its protocol have been approved by this viewer release.
    assert(!isExactPinnedHTTPLogin(kKey, {"https://evil.example/"}));
    assert(!isExactPinnedHTTPLogin(kKey, {"https://hg.stadt-nexverse.de:443/"}));
    assert(!isExactPinnedHTTPLogin(kKey, {"https://hg.stadt-nexverse.de:80/"}));
    assert(!isExactPinnedHTTPLogin(kKey, {kLoginURI, "https://evil.example/"}));
    assert(!isExactPinnedHTTPLogin(kKey, {"http://hg.stadt-nexverse.de:80/#fragment"}));
    assert(!isExactPinnedHTTPLogin(kKey, {"http://HG.STADT-NEXVERSE.DE:80/"}));
    assert(!isExactPinnedHTTPLogin(kKey, {"http://hg.stadt-nexverse.de:80@evil.example/"}));
    assert(!isExactPinnedHTTPLogin(kKey, {kLoginURI + std::string("login")}));
    assert(!isExactPinnedHTTPLogin(OGLNexVerseGrid::kBetaKey, {kLoginURI}));

    // Actual native policy tests: only the exact explicitly selected saved
    // identity on the active OGL grid can authorize credential-store reuse.
    using OGLNexVerseGrid::canReuseSavedAuthenticator;
    const std::string mainAccount = std::string("test.agent@") + kKey;
    const std::string betaAccount = std::string("test.agent@") + OGLNexVerseGrid::kBetaKey;
    assert(canReuseSavedAuthenticator(kKey, mainAccount, mainAccount, true, false));
    assert(canReuseSavedAuthenticator(OGLNexVerseGrid::kBetaKey,
                                      betaAccount, betaAccount, true, false));
    assert(!canReuseSavedAuthenticator(kKey, mainAccount, mainAccount, false, false));
    assert(!canReuseSavedAuthenticator(kKey, mainAccount, mainAccount, true, true));
    assert(!canReuseSavedAuthenticator(kKey, "", mainAccount, false, false));
    assert(!canReuseSavedAuthenticator(kKey, "", mainAccount, true, false));
    assert(!canReuseSavedAuthenticator(kKey, betaAccount, mainAccount, true, false));
    assert(!canReuseSavedAuthenticator(kKey, betaAccount, betaAccount, true, false));
    assert(!canReuseSavedAuthenticator(OGLNexVerseGrid::kBetaKey,
                                      mainAccount, mainAccount, true, false));
    assert(!canReuseSavedAuthenticator(kKey, "test.agent", "test.agent", true, false));
    assert(!canReuseSavedAuthenticator(kKey, "@" + std::string(kKey),
                                      "@" + std::string(kKey), true, false));
    assert(!canReuseSavedAuthenticator(kKey, mainAccount + " ",
                                      mainAccount, true, false));
    assert(!canReuseSavedAuthenticator(kKey, mainAccount,
                                      mainAccount + "@other-grid", true, false));
    assert(!canReuseSavedAuthenticator("other-grid", mainAccount, mainAccount, true, false));
    assert(!canReuseSavedAuthenticator(OGLNexVerseGrid::kName,
                                      "test.agent@Stadt NexVerse",
                                      "test.agent@Stadt NexVerse", true, false));
    assert(!canReuseSavedAuthenticator(OGLNexVerseGrid::kBetaName,
                                      "test.agent@Stadt NexVerse Beta",
                                      "test.agent@Stadt NexVerse Beta", true, false));

    assert(!OGLNexVerseGrid::gHTTPLoginApprovedForNextAttempt);
    assert(!OGLNexVerseGrid::isApprovedPinnedHTTPLogin(kKey,{kLoginURI},""));
    assert(!OGLNexVerseGrid::isApprovedPinnedHTTPLogin(
        kKey,{kLoginURI},"http://example.invalid/"));
    assert(OGLNexVerseGrid::isApprovedPinnedHTTPLogin(kKey,{kLoginURI},kLoginURI));
    assert(!OGLNexVerseGrid::isApprovedPinnedHTTPLogin(
        "other-grid.example",{kLoginURI},kLoginURI));
    assert(!OGLNexVerseGrid::isApprovedPinnedHTTPLogin(
        kKey,{"http://evil.example/"},kLoginURI));
    assert(!OGLNexVerseGrid::isApprovedPinnedHTTPLogin(
        kKey,{kLoginURI,"http://evil.example/"},kLoginURI));
    OGLNexVerseGrid::gHTTPLoginApprovedForNextAttempt = true;
    assert(OGLNexVerseGrid::isApprovedPinnedHTTPLogin(kKey,{kLoginURI},""));
    assert(!OGLNexVerseGrid::isApprovedPinnedHTTPLogin(
        kKey,{"http://evil.example/"},""));
    OGLNexVerseGrid::consumeOneTimeHTTPLoginApproval();
    assert(!OGLNexVerseGrid::gHTTPLoginApprovedForNextAttempt);
    assert(!OGLNexVerseGrid::isApprovedPinnedHTTPLogin(kKey,{kLoginURI},""));
    assert(OGLNexVerseGrid::isApprovedPinnedHTTPLogin(kKey,{kLoginURI},kLoginURI));
    return 0;
}
