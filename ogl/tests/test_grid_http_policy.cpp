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
