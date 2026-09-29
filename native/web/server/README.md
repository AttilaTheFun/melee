# Browser room service

This Worker serves the staged browser build and uses one Durable Object for each
private two-player room. WebSockets carry only signaling. Game inputs travel on
an ordered WebRTC DataChannel; TURN relays that connection when needed. A room
expires after one hour. Either player leaving closes the room.

## Build and stage

From the repository root, build the browser target as described in
[the browser README](../README.md), then run:

```sh
python3 native/web/stage_site.py
node native/web/test-room-server.mjs
node native/web/test-net-session.mjs
PLAYWRIGHT_MODULE=/path/to/playwright/index.mjs node native/web/test-room-launcher.mjs
```

Deploy **only** `native/build/wasm-site`. The staging script copies an explicit
allowlist and generates `build.json` from the game runtime and protocol files.
Never configure assets to point at `native/build` or a disc directory. Each
player selects their own local disc file; the site does not upload it.

## Cloudflare configuration (deployment pending)

Copy `wrangler.example.jsonc` into an untracked local configuration. Preserve
paths relative to its location or make the `main` and asset paths absolute.
Configure your account, Worker name and an account-unique rate-limit namespace.
Keep account identifiers and credentials out of this repository.

Set Worker secrets `TURN_KEY_ID` and `TURN_KEY_API_TOKEN` using Cloudflare's
secret management. The key must authorize the Realtime TURN credentials API.
The API token remains on the server; a client receives temporary ICE credentials
only after authenticating with its room token. No browser-side API token exists.
See [Cloudflare's credential API](https://developers.cloudflare.com/realtime/turn/generate-credentials/).

The example creates the `MeleeRoom` SQLite Durable Object class, a room creation
rate limiter and an ASSETS binding. Worker responses add COOP/COEP headers needed
by shared-memory Wasm. Serve on HTTPS.

## Playing and verification

Both players select the exact same disc representation and browser build. The
host chooses **Online · create room** and sends the invitation shown on screen.
The other player chooses **Online · join room** and pastes it. The invitation is
in the URL fragment; the host token is never put in the share link. Both players
use Player 1 keyboard bindings or their first gamepad. Online sessions use fresh
memory cards to avoid differing saved settings. Keep both tabs visible; leaving
a tab terminates the session rather than silently pausing only one player.

After deployment, verify across separate networks with **Force TURN relay**
enabled. `test-net-transport.mjs` also supports `MELEE_ICE_CONFIG` pointing to an
untracked JSON array of temporary ICE servers and `MELEE_RELAY_ONLY=1`; it
asserts a relay candidate was actually selected. Do not commit that file.

Current evidence: room boundary unit tests, a two-browser launcher test, and
actual workerd/SQLite Durable Object tests pass. Two Wasm engines completed
9,600 matching logic-tick snapshots through results over both direct WebRTC and
a forced local TURN relay. Uneven input delivery also passes. Cloudflare
deployment/credentials, separate-network routing and acceptable
online performance remain unverified. Online is currently input-delay lockstep, without rollback, and
rendering stalls slow simulation.

### Local relay regression

For relay-path checks without a Cloudflare account, install
[coturn](https://github.com/coturn/coturn) (`brew install coturn` on macOS) and run:

```sh
PLAYWRIGHT_MODULE=/path/to/playwright/index.mjs python3 native/web/test-local-turn.py
MELEE_DISC=/path/to/melee.ciso PLAYWRIGHT_MODULE=/path/to/playwright/index.mjs \
  python3 native/web/test-local-turn.py --game --ticks 2200
```

This starts an authenticated UDP-only relay bound to `127.0.0.1`, uses temporary
random credentials, asserts relay candidate selection in both real browsers,
and terminates the relay and removes credentials afterward. It never starts a
system service. Use `--ticks 9600` for the full-match/results scenario. This tests
actual TURN packet relay but not Cloudflare's credential endpoint or separate
networks/NATs. `MELEE_NET_JITTER=1` adds asymmetric ordered delivery delays to
the actual game test; it models latency variation, not packet loss on the wire.

Add `--loss-percent 5` to drop actual UDP datagrams through a loopback proxy in
both directions. The transport regression has recovered all 1,000 ordered
packets per peer under this impairment. A full game also passed 9,600 matching
logic snapshots through results with six ticks of input delay, a 17-second
worker-clock offset and 3,996 of 78,358 UDP datagrams dropped. It measured about
57 simulation ticks/s and 28 rendered frames/s while another offline test ran
on the same Mac; this is correctness evidence, not a broad performance claim.
The proxy uses a fixed random seed and prints actual dropped/observed counts.
It and the relay close their sockets on exit. This can also be combined with
`--game --ticks 9600` to validate a full match. Record pacing separately:
SCTP retransmission stalls can slow input-delay lockstep.

The full loss/clock-skew regression is reproducible with:

```sh
MELEE_DISC=/path/to/melee.ciso PLAYWRIGHT_MODULE=/path/to/playwright/index.mjs \
  MELEE_NET_DELAY=6 MELEE_NET_CLOCK_OFFSET_MS=17000 \
  python3 native/web/test-local-turn.py --game --ticks 9600 --loss-percent 5
```


### Worker toolchain checks

The current configuration passes a dry run with Wrangler 4.143.0. The runtime
test uses its Miniflare 5.20260926.0-alpha dependency and workerd 1.20260926.1.
Install tools into the ignored build directory and run from the repository root:

```sh
npm install --prefix native/build/worker-tools --save-exact wrangler@4.143.0 miniflare@5.20260926.0-alpha
native/build/worker-tools/node_modules/.bin/wrangler deploy --dry-run \
  --config native/web/server/wrangler.example.jsonc --outdir native/build/worker-dry-run
node native/web/test-room-runtime.mjs
```

The runtime test uses the actual Worker and SQLite-backed Durable Object,
including WebSocket upgrades, slot ownership, room signaling, credential cache
single-flight and disconnect cleanup. Only the outbound Cloudflare credentials
API is mocked with test values; no account credentials are needed. Miniflare's
local runtime listeners are disposed in `finally`. The dry run does not deploy
anything. Override `MINIFLARE_MODULE` to use another installed copy only when
intentionally checking toolchain compatibility.

The host selects input delay before creating the room (2, 3, 4 or 6 ticks; default
3). The server records it and both peers must use it. Larger buffers tolerate
more delivery variation at the cost of input response. In the earlier local
relay test with 10–65 ms artificial delivery delay, a six-tick buffer passed a
short roughly 60-tick/s gameplay check; four ticks slowed a full match to about
49 ticks/s. This is a measured test scenario, not a universal network guarantee.
For pacing checks, set `MELEE_NET_DELAY=6 MELEE_NET_JITTER=1 MELEE_NET_MIN_TPS=55`.
Those faster measurements used adaptive catch-up, which is now disabled by
default after a stress test exposed draw-dependent gameplay divergence. The
corrected default uses a fixed two-tick draw cadence shared by both peers and may
still fail that performance target under load;
do not re-enable catch-up to claim a pacing pass. See the browser milestone log.
On macOS, prefix long commands with `caffeinate -i` to inhibit idle sleep only
while the test runs; it does not keep the display awake.
