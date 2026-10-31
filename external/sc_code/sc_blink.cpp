// SPDX-License-Identifier: GPL-3.0-only
/* sc_blink.cpp — evaluation blink curve (headless). See sc_blink.h.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 */

#include "sc_blink.h"

namespace sc_code {

void Blink::start(int start, int end, double now_ms) {
	if (end <= start) {
		stop();
		return;
	}
	mStart = start;
	mEnd = end;
	mStartMs = now_ms;
	mActive = true;
}

void Blink::stop() {
	mActive = false;
	mStart = -1;
	mEnd = -1;
	mStartMs = 0.;
}

void Blink::set_duration(double ms) {
	/* `!(ms >= kMin)` instead of `ms < kMin` so that a NaN also falls into the min. */
	if (!(ms >= kMinDuration))
		ms = kMinDuration;
	if (ms > kMaxDuration)
		ms = kMaxDuration;
	mDuration = ms;
}

double Blink::alpha(double now_ms) const {
	if (!mActive)
		return 0.;
	const double t = elapsed(now_ms) / duration();
	if (t <= 0.)
		return 1.;
	if (t >= 1.)
		return 0.;
	/* Same curve as the IDE (`QEasingCurve::InCubic` over opacity 1.0 → 0.0): it
	 * stays high almost the whole blink and drops fast at the end. */
	return 1. - t * t * t;
}

} // namespace sc_code
