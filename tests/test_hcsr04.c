#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "hcsr04.h"

static uint64_t now_us, trigger_end, echo_length;
static int scenario;
static bool triggered;

uint64_t time_us_64(void) { now_us += 10; return now_us; }
void sleep_us(uint64_t delay_us) { now_us += delay_us; }
void gpio_put(uint32_t pin, bool value) {
    (void)pin;
    if (value) triggered = true;
    else if (triggered) trigger_end = now_us;
}
bool gpio_get(uint32_t pin) {
    (void)pin;
    if (scenario == 1) return false; /* no echo */
    if (scenario == 2) return true;  /* stuck before trigger */
    if (!triggered) return false;
    if (scenario == 3) return now_us >= trigger_end + 50; /* never falls */
    return now_us >= trigger_end + 50 && now_us < trigger_end + 50 + echo_length;
}
static void check(bool condition, const char *message) {
    if (!condition) { fprintf(stderr, "%s\n", message); exit(1); }
}
static void reset(int mode, uint64_t length) {
    now_us = trigger_end = 0; triggered = false; scenario = mode; echo_length = length;
}
int main(void) {
    float distance = -1;
    reset(0, 5800);
    check(hcsr04_measure_cm(20, 14, &distance), "100 cm echo should succeed");
    check(fabsf(distance - 100.0f) < 0.5f, "pulse duration conversion is incorrect");
    for (int mode = 1; mode <= 3; ++mode) {
        reset(mode, 0); distance = -1;
        check(!hcsr04_measure_cm(20, 14, &distance), "missing/stuck echo should fail");
        check(now_us <= 61000, "timeout must bound both waits");
        check(distance == -1, "failed measurement must not overwrite output");
    }
    reset(0, 30);
    check(!hcsr04_measure_cm(20, 14, &distance), "too-short echo should fail");
    reset(0, 25000);
    check(!hcsr04_measure_cm(20, 14, &distance), "out-of-range echo should fail");
    check(!hcsr04_measure_cm(20, 14, NULL), "NULL output must fail safely");
    puts("HCSR04 timeout and conversion tests passed");
    return 0;
}
