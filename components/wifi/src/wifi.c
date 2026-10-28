#include "wifi.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_remote.h"
#include "lwip/ip_addr.h"
#include "wifi/ap.h"
#include "wifi/sta.h"

static const char* const TAG = "wifi";

void wifi_init(void) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    esp_netif_t* esp_netif_ap = wifi_ap_init();

    wifi__sta_init();

    ESP_ERROR_CHECK(esp_wifi_start());

#if CONFIG_IDF_TARGET_ESP32C3
    esp_wifi_set_max_tx_power(34);
#endif

    if (esp_netif_napt_enable(esp_netif_ap) != ESP_OK)
        ESP_LOGE(TAG, "NAPT not enabled on netif: %p", esp_netif_ap);
}