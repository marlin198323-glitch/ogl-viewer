#ifndef OGL_NEXVERSE_GRID_H
#define OGL_NEXVERSE_GRID_H

// OGL-specific fixed Stadt NexVerse profiles: public OpenSim and the Beta portal.
//
// These values follow the public Stadt NexVerse /get_grid_info response.
// If the server changes its login address, update it here and rebuild.
// This public grid currently uses HTTP. Only the exact pinned endpoint is
// eligible for explicit opt-in, optionally remembered for this exact URI.
// HTTPS is strongly preferred. Never allow generic/alternate HTTP grids.
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

// Beta is deliberately selectable in the login grid list, but its HTTPS
// certificate and OpenSim/XML-RPC compatibility have NOT been verified.
// Do not send credentials to a generic website as if it were an OpenSim
// login service. Enable authentication only when a matching transport
// and protocol adapter have been implemented and tested.
inline constexpr const char* kBetaKey = "beta.stadt-nexverse.de";
inline constexpr const char* kBetaName = "Stadt NexVerse Beta";
inline constexpr const char* kBetaLoginURI = "https://beta.stadt-nexverse.de/";
inline constexpr const char* kBetaWebsite = "https://beta.stadt-nexverse.de/";

inline bool isFixedGrid(const std::string& grid)
{
    return grid == kKey || grid == kName ||
           grid == kBetaKey || grid == kBetaName;
}
inline bool isBetaGrid(const std::string& grid)
{
    return grid == kBetaKey || grid == kBetaName;
}

// Credential-store access is not authorized by merely typing a name. Only
// an exact saved-account selection belonging to the currently selected,
// canonical OGL grid can reuse a previously stored authenticator. A fresh
// password edit must always take precedence over the stored secret.
inline bool canReuseSavedAuthenticator(const std::string& currentGrid,
                                      const std::string& selectedCredential,
                                      const std::string& requestedCredential,
                                      bool hasSavedSelection,
                                      bool passwordModified)
{
    if (!hasSavedSelection || passwordModified ||
        (currentGrid != kKey && currentGrid != kBetaKey) ||
        selectedCredential != requestedCredential)
    {
        return false;
    }

    // A protected credential name has exactly one user/grid delimiter.
    // Without this check, an account like user@other-grid@our-grid would
    // appear to belong to the trusted world despite containing another
    // destination component. Do not silently normalize migrated entries.
    const size_t separator = requestedCredential.find('@');
    return separator != std::string::npos && separator > 0 &&
           requestedCredential.find('@', separator + 1) == std::string::npos &&
           requestedCredential.substr(separator + 1) == currentGrid;
}

// Approval for one pending authentication attempt, plus an optional persisted approval for
// the *exact* trusted URI. A changed URI or a different grid never inherits
// approval, and no passwords or session identifiers are stored here.
inline bool gHTTPLoginApprovedForNextAttempt = false;

// Called at the actual network-login boundary. The transient approval is
// single-use; a failed login must not silently permit a second transmission.
inline void consumeOneTimeHTTPLoginApproval()
{
    gHTTPLoginApprovedForNextAttempt = false;
}

inline bool isExactPinnedHTTPLogin(const std::string& selectedGrid,
                                  const std::vector<std::string>& loginURIs)
{
    return selectedGrid == kKey && loginURIs.size() == 1 &&
           loginURIs.front() == kLoginURI;
}
inline bool isApprovedPinnedHTTPLogin(const std::string& selectedGrid,
                                     const std::vector<std::string>& loginURIs,
                                     const std::string& rememberedURI)
{
    return isExactPinnedHTTPLogin(selectedGrid, loginURIs) &&
           (gHTTPLoginApprovedForNextAttempt ||
            rememberedURI == kLoginURI);
}
} // namespace OGLNexVerseGrid

#endif
