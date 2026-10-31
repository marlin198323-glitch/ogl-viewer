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
    OGLNexVerseGrid::gHTTPLoginApprovedThisSession = true;
    assert(OGLNexVerseGrid::gHTTPLoginApprovedThisSession);
    return 0;
}
