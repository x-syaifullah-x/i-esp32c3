#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "led/led_default.h"
#include "nvs_flash.h"
#include "server.h"
#include "wifi.h"

void app_main(void) {
    // esp_log_level_set("*", ESP_LOG_NONE);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init();

    httpd_handle_t server = server_start(80);

    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_8, 0);

    // const esp_timer_create_args_t timer_args = {.callback = led_default_blink_cb, .name = "led_default_blink"};
    // esp_timer_handle_t timer;
    // esp_timer_create(&timer_args, &timer);
    // esp_timer_start_periodic(timer, 1000000);
}