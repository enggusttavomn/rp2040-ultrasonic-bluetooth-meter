#ifndef HCSR04_H
#define HCSR04_H

#include <stdbool.h>
#include <stdint.h>

/* Returns false on missing/stuck echo or a reading outside 2–400 cm.
 * The output is only updated after a valid measurement. */
bool hcsr04_measure_cm(uint32_t trigger_pin, uint32_t echo_pin, float *distance_cm);

#endif
