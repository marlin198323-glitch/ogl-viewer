#!/usr/bin/env bash
# Read-only HTTPS readiness check for the *actual* Stadt NexVerse grid host.
# No packages, firewall rules, certificates or OpenSim services are changed.
set -u
HOST="hg.stadt-nexverse.de"
EXPECTED_IP="195.90.222.105"
FAIL=0

section() { printf '\n=== %s ===\n' "$1"; }
ok() { printf 'OK: %s\n' "$1"; }
warn() { printf 'CHECK: %s\n' "$1"; FAIL=$((FAIL + 1)); }

section "Server identity"
echo "Machine: $(hostname -f 2>/dev/null || hostname)"
echo "User: $(id -un)"
echo "DNS A records:"
getent ahostsv4 "$HOST" 2>/dev/null | awk '$2=="STREAM" {print "  "$1}' | sort -u
if getent ahostsv4 "$HOST" | awk '$2=="STREAM" {print $1}' | grep -Fxq "$EXPECTED_IP"; then
    ok "Grid hostname resolves to expected public IPv4"
else
    warn "Grid DNS differs from the expected IPv4"
fi

section "Local OpenSim HTTP service"
if command -v ss >/dev/null 2>&1; then
    ss -ltn 2>/dev/null | awk 'NR==1 || /:80[[:space:]]|:443[[:space:]]/' | head -18
fi
for address in 'http://127.0.0.1:80/get_grid_info' \
               'http://localhost:80/get_grid_info'; do
    printf '%s -> ' "$address"
    curl --noproxy '*' -sS --max-time 6 -o /dev/null \
        -w 'HTTP %{http_code}\n' "$address" || :
done

section "Externally advertised grid URL"
TMP="$(mktemp)"
trap 'rm -f "$TMP"' EXIT
if curl -fsS --noproxy '*' --connect-timeout 4 --max-time 10 \
    "http://$HOST/get_grid_info" -o "$TMP"; then
    echo "GridInfo responds over HTTP"
    # Avoid printing XML beyond the public login field.
    python3 - "$TMP" <<'PY'
import sys
import xml.etree.ElementTree as ET
try:
    root = ET.parse(sys.argv[1]).getroot()
    login = (root.findtext("login") or "").strip()
    print("Published login URI:", login or "(missing)")
    print("Advertises HTTPS:", login.startswith("https://"))
except (OSError, ET.ParseError) as exc:
    print("Invalid GridInfo XML:", type(exc).__name__)
PY
else
    warn "Could not read public GridInfo endpoint"
fi

section "HTTPS and TLS certificate"
if curl -fsS --noproxy '*' --connect-timeout 4 --max-time 8 \
    "https://$HOST/get_grid_info" -o /dev/null; then
    ok "Valid public HTTPS endpoint reachable"
else
    warn "No working publicly trusted HTTPS endpoint yet"
fi

section "Proxy prerequisites"
if command -v caddy >/dev/null 2>&1; then
    caddy version
    CONFIG="$(cd "$(dirname "$0")" && pwd)/Caddyfile.hg.stadt-nexverse.de"
    caddy validate --config "$CONFIG" --adapter caddyfile || warn "Caddyfile validation failed"
else
    warn "Caddy is not installed (validation skipped)"
fi

section "Verdict"
if [ "$FAIL" -eq 0 ]; then
    ok "Checks passed, but authenticated login still needs a controlled acceptance"
else
    echo "$FAIL item(s) need attention before changing the ROBUST login URI."
fi
# Read-only: exit zero for successful diagnostics, even if deployment is not ready.
