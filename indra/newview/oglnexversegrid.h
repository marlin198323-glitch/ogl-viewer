#ifndef OGL_NEXVERSE_GRID_H
#define OGL_NEXVERSE_GRID_H

// OGL-specific fixed OpenSim grid profile.
//
// These values follow the public Stadt NexVerse /get_grid_info response.
// If the server changes its login address, update it here and rebuild.
// This public grid currently uses HTTP. Only the exact pinned endpoint is
// eligible for an explicit, per-process opt-in. HTTPS is strongly preferred.
// Never allow generic/alternate HTTP grids, and never auto-login over HTTP.
#include <string>
#include <vector>
namespace OGLNexVerseGrid {
inline constexpr const char* kKey = "hg.stadt-nexverse.de:80";
inline constexpr const char* kName = "Stadt NexVerse";
inline constexpr const char* kNick = "Stadt NexVerse";
inline constexpr const char* kLoginURI = "http://hg.stadt-nexverse.de:80/";
inline constexpr const char* kWelcomeURI =
    "https://stadt-nexverse.de/service/welcome";
inline constexpr const char* kWebsite = "https://stadt-nexverse.de/";
inline constexpr const char* kSLURLBase = "hop://hg.stadt-nexverse.de:80/";
inline constexpr const char* kAppSLURLBase =
    "x-grid-location-info://hg.stadt-nexverse.de:80/app";

// A runtime-only opt-in. Never persist this, including in user settings.
inline bool gHTTPLoginApprovedThisSession = false;

inline bool isExactPinnedHTTPLogin(const std::string& selectedGrid,
                                  const std::vector<std::string>& loginURIs)
{
    return selectedGrid == kKey && loginURIs.size() == 1 &&
           loginURIs.front() == kLoginURI;
}
} // namespace OGLNexVerseGrid

#endif
