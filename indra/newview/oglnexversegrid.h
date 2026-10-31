#ifndef OGL_NEXVERSE_GRID_H
#define OGL_NEXVERSE_GRID_H

// OGL-specific fixed OpenSim grid profile.
//
// These values follow the public Stadt NexVerse /get_grid_info response.
// If the server changes its login address, update it here and rebuild.
// Do not substitute an unverified HTTPS URI for the currently advertised HTTP
// endpoint. Credential submission remains blocked until TLS is available.
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
} // namespace OGLNexVerseGrid

#endif
