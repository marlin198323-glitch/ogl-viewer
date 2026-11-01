#include "../../indra/newview/oglnexversegrid.h"

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

    assert(!OGLNexVerseGrid::gHTTPLoginApprovedThisSession);
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
    OGLNexVerseGrid::gHTTPLoginApprovedThisSession = true;
    assert(OGLNexVerseGrid::isApprovedPinnedHTTPLogin(kKey,{kLoginURI},""));
    assert(!OGLNexVerseGrid::isApprovedPinnedHTTPLogin(
        kKey,{"http://evil.example/"},""));
    return 0;
}
