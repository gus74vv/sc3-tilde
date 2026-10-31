// SPDX-License-Identifier: GPL-3.0-only
// 7_ExtServer.sc — boot/quit neutralised: the external owns the World.
//
// In the IDE, `s.boot` forks scsynth and `s.quit` kills it. Inside the external
// the World (libscsynth) ALREADY exists and the external controls its lifecycle:
// a third-party `startup.scd` (or user code) calling s.boot/s.quit must neither
// start a separate scsynth nor kill the embedded World.
//
// Policy (decision v0.14): both are visible NO-OPs. `serverRunning` is set by the
// external on adoption (s.statusWatcher.serverRunning = true + /notify).
// `s.waitForBoot` / `s.doWhenBooted` still work: they poll serverRunning inside a
// Routine and fire once the external adopts the server.
+ Server {
	boot { | startAliveThread = true, recover = false, onFailure |
		"SCMaxCompat: s.boot ignored (the external owns the server).".postln;
		^this
	}

	quit { | onComplete, onFailure, watchShutDown = true |
		"SCMaxCompat: s.quit ignored (the external closes the World).".postln;
		onComplete.value;
		^this
	}
}
