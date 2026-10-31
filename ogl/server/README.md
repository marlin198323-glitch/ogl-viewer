# Stadt NexVerse: optional ROBUST HTTPS upgrade

**Not required for the current OGL test.** Stadt NexVerse has intentionally
chosen an existing HTTP login. OGL supports the one fixed HTTP login URI after
an explicit, temporary, localized user confirmation. HTTP remains unencrypted.
This file documents an **optional future** server TLS migration only.


Status (2026-10-31): **deployment not performed**. The actual OpenSim/ROBUST
host is `195.90.222.105` (`hg.stadt-nexverse.de`), whereas the connected
Desktop Commander devices are the Linux Mint PC and Raspberry Pi. No valid
SSH authentication to the grid host has been established. No grid service,
DNS entry, firewall rule or ROBUST configuration has been changed.

## What is confirmed

- GET `http://hg.stadt-nexverse.de/get_grid_info` returns valid OpenSim XML.
- `[GridInfoService]` advertises `http://hg.stadt-nexverse.de:80/` as `login`.
- TCP 80 and 22 are reachable externally; 443 is not accepting connections.
- The login page `https://stadt-nexverse.de/service/welcome` returns HTTP 200.
- Existing Hypergrid entries advertise `http://hg.stadt-nexverse.de:80`.
- Current OGL builds reject password submission over HTTP (including autologin).

## Deployment only on the actual grid host (requires authorized admin)

**Do not run the following on the Raspberry Pi or Linux Mint PC.** The DNS
points to the grid host, which is a different server.

1. Make a backup of the current ROBUST/OpenSim *.ini files and server
   configuration; record the service management method and a rollback path.
   Do not print database passwords, API keys or stored credential values.
2. Inspect `ss -lntp` and confirm port 80 runs the actual OpenSim web
   service on the **same host** and that `curl --noproxy '*' -fsS
   http://127.0.0.1:80/get_grid_info` succeeds. If ROBUST binds only a
   private address or another port, adjust the upstream explicitly; do
   not guess or proxy to a public Internet HTTP endpoint.
3. Run `bash grid-https-preflight.sh` from this directory to inspect
   the listener, public GridInfo and Caddy availability. The check is
   read-only.
4. Allow inbound TCP/443 in the OS and any provider firewall. Retain
   existing TCP/80 and region/Hypergrid UDP/TCP service ports.
5. Install Caddy using the grid server's supported package manager.
   Review the included `Caddyfile.hg.stadt-nexverse.de`. Confirm its
   upstream points only to the trusted **local** ROBUST HTTP listener.
   Validate using:
   ```sh
   caddy validate --config ./Caddyfile.hg.stadt-nexverse.de --adapter caddyfile
   ```
   Place a reviewed copy in the server's Caddy configuration path and
   load it using the service manager on the grid host.
6. Caddy listens on HTTPS port 443 only; automatic HTTP redirects are
   disabled so OpenSim keeps port 80. The ACME issuer uses TLS-ALPN-01
   on port 443, not HTTP-01 on 80. The grid's A/AAAA records must
   resolve to the reachable host and the relevant TCP/443 route must
   reach Caddy, or issuance will fail.
7. Test `curl -fsS https://hg.stadt-nexverse.de/get_grid_info` **without
   `-k`**. Ensure hostname/certificate validation succeeds. POST-based
   XML-RPC login must be supported, not just GET. Inspect Caddy logs for
   errors without printing body content or account passwords.
8. Only after the HTTPS proxy works, update the active ROBUST
   `Robust.ini` / `Robust.HG.ini` or equivalent inherited configuration,
   in the `[GridInfoService]` section:
   ```ini
   [GridInfoService]
   login = https://hg.stadt-nexverse.de/
   ```
   Keep `gridname`, `gridnick`, `welcome`, `gatekeeper`, and `home`
   values as currently configured unless a deliberate Hypergrid migration
   has been separately validated. If applicable, inspect the
   `[LoginService]` `HasProxy` setting for the installed OpenSim version.
   Do **not** blindly change the other public service URLs.
9. Restart only the required ROBUST service with an approved maintenance
   procedure. Validate that public GridInfo XML actually advertises
   `https://hg.stadt-nexverse.de/`, that existing regions still register
   and Hypergrid entries continue to work, and that TLS requests do not
   produce unexpected redirects.
10. Update **the single central OGL viewer value**
    `indra/newview/oglnexversegrid.h`:
    ```cpp
    inline constexpr const char* kLoginURI = "https://hg.stadt-nexverse.de/";
    ```
    Rebuild `ogl-develop`, stage the resources, run unit/native-module
    checks and test login with a controlled test account. Only after
    verified login, accept 3D assets, sky, avatar and teleport tests.

## Rollback / safety

- The Caddyfile never takes over port 80 or stops the OpenSim backend.
- If the new HTTPS endpoint malfunctions, revert the changed
  `[GridInfoService]` login value from the backed-up configuration and
  restart **only** the affected ROBUST service under the existing
  maintenance procedure. Stop the new Caddy service only if necessary.
- Current OGL supports the one verified Stadt NexVerse HTTP endpoint
  following a per-process user warning/opt-in. HTTP traffic is unencrypted;
  do not extend that exception to arbitrary grids or unattended login.
- Never route passwords through `https://stadt-nexverse.de` to the grid
  across a public, unencrypted HTTP hop.
- A valid HTTPS GridInfo page does not, by itself, prove that the login
  service supports XML-RPC POST and token/session exchange.

Sources:
- https://github.com/opensim/opensim/blob/master/bin/Robust.HG.ini.example
- https://opensimulator.dev/wiki/ROBUST
- https://caddyserver.com/docs/automatic-https
- https://caddyserver.com/docs/caddyfile/directives/tls
- https://caddyserver.com/docs/caddyfile/options

## Client activation once HTTPS is deployed

From the Linux Mint PC, first run the read-only client verification:

```bash
cd "$HOME/Schreibtisch/OGL-Viewer/ogl-firestorm"
python3 ogl/scripts/activate-grid-https.py --check
```

Only if this passes with a valid public certificate **and** GridInfo
publishes exactly `https://hg.stadt-nexverse.de/`, run:

```bash
python3 ogl/scripts/activate-grid-https.py --activate
bash ogl/scripts/build-test.sh
```

The activation changes only the pinned viewer source header; it does not
change DNS, the grid server or authentication data. Do not run activation
before the grid host is configured.
