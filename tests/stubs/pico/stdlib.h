#ifndef TEST_PICO_STDLIB_H
#define TEST_PICO_STDLIB_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
uint64_t time_us_64(void);
bool gpio_get(uint32_t pin);
void gpio_put(uint32_t pin, bool value);
void sleep_us(uint64_t delay_us);
#endif
