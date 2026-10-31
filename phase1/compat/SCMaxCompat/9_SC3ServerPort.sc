// SPDX-License-Identifier: GPL-3.0-only
// 9_SC3ServerPort.sc — the external's World listens on a port IT PICKS
//
// Problem: sclang/scsynth (the SuperCollider IDE) always take 127.0.0.1:57110,
// and scsynth's asio binds WITHOUT SO_REUSEADDR ⇒ whoever arrives second loses.
// With the external's World pinned to 57110, having the IDE open (or its Server
// booted) forced you to close it.
//
// Solution: the external binds FIRST — port 0 = the kernel picks a free one, or
// `@port N` — and hands the real port to sclang through the environment
// (SC3_WORLD_PORT) before this library is compiled. Here `Server.default.addr` is
// pointed at that port, so sclang sends its commands (d_recv, s_new, sync…) to the
// external's World and not to the IDE's. The user's `~s`/`s` is already pointed.
//
// Why an `*initClass` (and not redefining Server): `Main:startup` runs
//   platform.initPlatform → super.startup (Process.startup → initClassTree)
//   → interpreter.s = Server.default → platform.startup → StartUp.run
// (SCClassLibrary/DefaultLibrary/Main.sc), i.e. ALL `initClass` methods run
// BEFORE `interpreter.s = Server.default` and before the user's startup.scd is
// loaded. That is the only early point available without patching SuperCollider:
// `startup.scd` (and anything reading `s.addr.port`) sees the right port.
//
// This is not an override: `Server:addr_` is a public setter (3.14.1) that only
// recomputes inProcess/isLocal/remoteControlled; it touches neither `s.options`
// nor the statusWatcher. Without the external (in the IDE) SC3_WORLD_PORT is
// undefined and this does nothing: exact stock behaviour.

SC3ServerPort {
	*initClass {
		var port;
		port = "SC3_WORLD_PORT".getenv;             // nil in the IDE ⇒ stock
		if (port.isNil) { ^this };
		port = port.asInteger;
		if (port <= 0) { ^this };
		Class.initClassTree(Server);                // Server.default ya existe
		if (Server.default.addr.port != port) {
			Server.default.addr = NetAddr("127.0.0.1", port);
		}
	}
}
