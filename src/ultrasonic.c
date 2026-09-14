
#include "ultrasonic.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define SOUND_SPEED_CM_PER_US 0.0343

float measure_distance(uint trig_pin, uint echo_pin) {
    // Send a 10us pulse to trigger the measurement
    gpio_put(trig_pin, 1);
    sleep_us(10);
    gpio_put(trig_pin, 0);

    // Wait for the echo start
    while (gpio_get(echo_pin) == 0);

    // Measure the time for echo to return
    absolute_time_t start_time = get_absolute_time();
    while (gpio_get(echo_pin) == 1);
    absolute_time_t end_time = get_absolute_time();

    int64_t pulse_duration_us = absolute_time_diff_us(start_time, end_time);

    // Calculate the distance (duration / 2 * speed of sound)
    float distance_cm = (pulse_duration_us / 2.0) * SOUND_SPEED_CM_PER_US;

    return distance_cm;
} 