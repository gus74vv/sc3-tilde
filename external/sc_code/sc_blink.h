// SPDX-License-Identifier: GPL-3.0-only
/* sc_blink.h — blink of the evaluated block in `sc_code` (phase S5e, SCODE.md §7.18).
 *
 * User request: when evaluating (⌘⏎ / ⇧⏎ / ⌘⇧⏎) the evaluated block has to "blink" to
 * confirm WHAT was sent to sclang, with **selectable color and duration**.
 *
 * Parity with the IDE (verified in `vendor/supercollider`): - What blinks: the EXACT
 * range being evaluated, which in the line fallback is the whole block **including a
 * trailing `//` comment** (`sc_editor.cpp` evaluateLine/evaluateRegion + `blinkCode`,
 * `overlay.cpp` L71: the cursor goes to StartOfBlock..EndOfBlock). Nothing is
 * trimmed: that is what scide does. - How: `QPropertyAnimation` of opacity **1.0 →
 * 0.0** with `QEasingCurve::InCubic` during `mBlinkDuration`, which is an IDE SETTING
 * (`core/settings/manager.cpp:73` ⇒ default **600 ms**; the color comes from the
 * theme entry `evaluatedCode`, orange in the factory themes). Here the default is
 * **500 ms** (400 ms in 0.8.4; raised by user request in 0.9.4) and the duration is
 * the `@blinkduration` attribute.
 *
 * **Headless** and minimal module: only the state and the opacity curve, so
 * duration and curve can be pinned in a test table (`tests/blink_test.cpp`) without
 * Max. The object sets the offsets (the `EvalSpan` ones from `sc_region`), repaints
 * with a `t_clock` every `kStep` ms and paint uses `alpha()`.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#pragma once

namespace sc_code {

class Blink {
  public:
	static constexpr double kDefaultDuration = 500.; /* ms (the IDE uses 600) */
	static constexpr double kMinDuration = 50.;      /* a 0 ms blink cannot be seen */
	static constexpr double kMaxDuration = 5000.;
	static constexpr double kStep = 25.; /* ms between repaints (≈20 frames in 500) */

	/* Starts the blink of the range [start, end) (byte offsets of the buffer). */
	void start(int start, int end, double now_ms);

	/* Lo cancela (nuevo eval, documento recargado, objeto liberado). */
	void stop();

	/* Duration in ms, clamped to [kMinDuration, kMaxDuration]. */
	void set_duration(double ms);
	double duration() const { return mDuration > 0. ? mDuration : kDefaultDuration; }

	bool active() const { return mActive; }
	int start_off() const { return mStart; }
	int end_off() const { return mEnd; }
	double started_ms() const { return mStartMs; }

	double elapsed(double now_ms) const { return mActive ? now_ms - mStartMs : duration(); }
	/* 1 at the start, 0 at the end: paint multiplies it by the color alpha. */
	double alpha(double now_ms) const;
	bool expired(double now_ms) const { return !mActive || elapsed(now_ms) >= duration(); }

  private:
	bool mActive = false;
	int mStart = -1;
	int mEnd = -1;
	double mStartMs = 0.;
	double mDuration = kDefaultDuration;
};

} // namespace sc_code
