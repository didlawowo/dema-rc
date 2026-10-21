/* SPDX-License-Identifier: LGPL-2.1+ */
#ifndef DEMA_RC_SCALE_H
#define DEMA_RC_SCALE_H

#include <stdint.h>

/* Convert a valid Linux evdev axis range to RC [1000, 2000].
 * Callers reject invalid device ranges before enabling RC input.
 */
static inline uint16_t rc_scale_axis(int value, int min, int max)
{
    int64_t span = (int64_t)max - min;
    int64_t position = (int64_t)value - min;

    if (span <= 0)
        return 1000;
    if (position < 0)
        position = 0;
    if (position > span)
        position = span;

    return (uint16_t)(1000 + position * 1000 / span);
}

#endif
