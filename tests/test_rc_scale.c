/* SPDX-License-Identifier: LGPL-2.1+ */
#include "rc_scale.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

static void check_range(int min, int max)
{
    int64_t previous = 0;
    int64_t span = (int64_t)max - min;

    assert(rc_scale_axis(min, min, max) == 1000);
    assert(rc_scale_axis(max, min, max) == 2000);
    assert(rc_scale_axis(min - (min > INT_MIN), min, max) == 1000);
    assert(rc_scale_axis(max + (max < INT_MAX), min, max) == 2000);

    for (int64_t i = 0; i <= 1000; i++) {
        int sample = (int)((int64_t)min + span * i / 1000);
        int64_t actual = rc_scale_axis(sample, min, max);
        assert(actual >= 1000 && actual <= 2000);
        assert(actual >= previous);
        previous = actual;
    }
}

int main(void)
{
    check_range(0, 255);
    check_range(0, 1023);
    check_range(-32768, 32767);
    check_range(INT_MIN, INT_MAX);
    assert(rc_scale_axis(512, 0, 1023) == 1500);
    assert(rc_scale_axis(0, 0, 0) == 1000);
    assert(rc_scale_axis(100, 100, 99) == 1000);
    puts("PASS rc_scale_axis: boundaries, midpoint, monotonicity, invalid ranges");
    return 0;
}
