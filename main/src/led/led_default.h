#pragma once

#include "esp_timer.h"
#include "soc/gpio_num.h"

static bool led = false;

static void led_default_blink_cb(void* arg) {
    led = !led;
    gpio_set_level(GPIO_NUM_8, led);
}