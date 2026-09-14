#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "pico/stdlib.h"

float measure_distance(uint trig_pin, uint echo_pin);

#endif
