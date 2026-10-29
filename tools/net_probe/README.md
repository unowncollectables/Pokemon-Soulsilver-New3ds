# net_probe

Standalone libctru app (no game code, no save access) for the online-trading project (GTS through a community Wi-Fi Connection server). It connects over the 3DS's own Wi-Fi with plain sockets and logs what the server answers, so we know what the game's network code has to cope with before touching it.

Build (devkitPro `3ds-dev`): `cd tools/net_probe && make` gives `net_probe.3dsx`. Copy it to `sdmc:/3ds/` and start it from the Homebrew Launcher with Wi-Fi on. No ROM or save is involved.

It sends: a GTS info GET, a GTS challenge GET, one NAS POST with an unknown action (creates nothing), and TCP connect tests on ports 443, 29900 and 29901. Server IP defaults to the Poke Classic Network (178.62.43.212); put another IPv4 in `sdmc:/3ds/net_probe_ip.txt` to override.

Output: top screen and `sdmc:/3ds/net_probe_log.txt` (`[NET]` lines). Send that file back. START exits.

What the answers tell us: whether plain-HTTP login works (decides how hard SSL is), whether the GTS handshake works from a 3DS, and whether the friend-code ports are open at all.
