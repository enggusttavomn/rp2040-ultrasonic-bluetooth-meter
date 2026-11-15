#include "hcsr04.h"
#include "pico/stdlib.h"

#define ECHO_TIMEOUT_US 30000u

bool hcsr04_measure_cm(uint32_t trigger_pin, uint32_t echo_pin, float *distance_cm) {
    if (distance_cm == NULL) {
        return false;
    }

    /* Reject a stuck HIGH input before issuing a new measurement. */
    uint64_t wait_start = time_us_64();
    while (gpio_get(echo_pin)) {
        if (time_us_64() - wait_start >= ECHO_TIMEOUT_US) {
            return false;
        }
    }

    gpio_put(trigger_pin, 0);
    sleep_us(2);
    gpio_put(trigger_pin, 1);
    sleep_us(10);
    gpio_put(trigger_pin, 0);

    wait_start = time_us_64();
    while (!gpio_get(echo_pin)) {
        if (time_us_64() - wait_start >= ECHO_TIMEOUT_US) {
            return false;
        }
    }
    const uint64_t echo_start = time_us_64();
    while (gpio_get(echo_pin)) {
        if (time_us_64() - echo_start >= ECHO_TIMEOUT_US) {
            return false;
        }
    }

    const float measured_cm = (float)(time_us_64() - echo_start) / 58.0f;
    if (measured_cm < 2.0f || measured_cm > 400.0f) {
        return false;
    }
    *distance_cm = measured_cm;
    return true;
}
