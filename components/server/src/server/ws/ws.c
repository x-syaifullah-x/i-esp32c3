#include "server/ws/ws.h"

#include "cJSON.h"
#include "command.h"
#include "command/auth.h"
#include "command/wifi/ap.h"
#include "command/wifi/sta.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/semphr.h"
#include "server/response.h"

#define WS_MUTEX_TIMEOUT_MS 100
#define WS_MAX_PAYLOAD_SIZE 128

enum {
    /* RFC 6455 */
    WS_CLOSE_NORMAL = 1000,
    WS_CLOSE_GOING_AWAY = 1001,
    WS_CLOSE_PROTOCOL_ERROR = 1002,
    WS_CLOSE_UNSUPPORTED_DATA = 1003,

    WS_CLOSE_INVALID_PAYLOAD = 1007,
    WS_CLOSE_POLICY_VIOLATION = 1008,
    WS_CLOSE_MESSAGE_TOO_BIG = 1009,
    WS_CLOSE_MANDATORY_EXTENSION = 1010,
    WS_CLOSE_INTERNAL_ERROR = 1011,

    /* RFC 8441 / registered codes */
    WS_CLOSE_SERVICE_RESTART = 1012,
    WS_CLOSE_TRY_AGAIN_LATER = 1013,
    WS_CLOSE_BAD_GATEWAY = 1014,

    /* Local/internal codes — MUST NOT be sent */
    WS_CLOSE_NO_STATUS = 1005,
    WS_CLOSE_ABNORMAL_CLOSURE = 1006,
    WS_CLOSE_TLS_HANDSHAKE = 1015,
};

typedef struct __attribute__((packed)) {
    uint8_t id;
} session_t;

static const char* const TAG = "server_ws_ws.c";

static int s_client_fds[CONFIG_WS_MAX_CLIENTS] = {[0 ... CONFIG_WS_MAX_CLIENTS - 1] = -1};
static size_t s_client_count = 0;

static SemaphoreHandle_t s_mutex = NULL;

void ws_broadcast(const httpd_handle_t handle, const uint8_t* const payload, size_t payload_len, httpd_ws_type_t type) {
    httpd_ws_frame_t frame = {
        .type = type,
        .payload = (uint8_t*)payload,
        .len = payload_len,
        .final = true,
    };

    for (size_t i = 0; i < s_client_count; i++) {
        httpd_ws_send_frame_async(handle, s_client_fds[i], &frame);
    }

    // if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(WS_MUTEX_TIMEOUT_MS)) != pdTRUE) return;

    // for (int i = (int)s_client_count - 1; i >= 0; i--) {
    //     esp_err_t err = httpd_ws_send_frame_async(handle, s_client_fds[i], &frame);
    //     if (err != ESP_OK) {
    //         ESP_LOGW(TAG, "Send failed fd=%d err=%d, removing", s_client_fds[i], err);
    //         httpd_sess_trigger_close(handle, s_client_fds[i]);
    //         s_client_fds[i] = s_client_fds[--s_client_count];
    //     }
    // }

    // xSemaphoreGive(s_mutex);
}

static void ws_unregister(int fd) {
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(WS_MUTEX_TIMEOUT_MS)) != pdTRUE) return;
    for (size_t i = 0; i < s_client_count; i++) {
        if (s_client_fds[i] == fd) {
            s_client_fds[i] = s_client_fds[--s_client_count];
            break;
        }
    }
    xSemaphoreGive(s_mutex);
}

static void ws_close(httpd_req_t* req, const char* reason, uint16_t code) {
    const int fd = httpd_req_to_sockfd(req);
    if (fd <= 0) {
        return;
    }

    size_t reason_len = strlen(reason);
    if (reason_len > 128) {
        ESP_LOGW(TAG, "ws_close reason too long (%d), truncating", reason_len);
        reason_len = 128;
    }

    const size_t payload_len = 2 + reason_len;
    uint8_t payload[payload_len];
    payload[0] = code >> 8;    // high byte
    payload[1] = code & 0xFF;  // low byte
    memcpy(&payload[2], reason, reason_len);

    httpd_ws_frame_t frame = {
        .type = HTTPD_WS_TYPE_CLOSE,
        .payload = payload,
        .len = payload_len,
        .final = true,
    };

    ws_unregister(fd);
    esp_err_t err = httpd_ws_send_frame_async(req->handle, fd, &frame);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send close frame to fd=%d: %s", fd, esp_err_to_name(err));
    }
    httpd_sess_trigger_close(req->handle, fd);
}

static esp_err_t ws_send(httpd_handle_t handle, uint8_t fd, uint8_t* payload, size_t payload_len) {
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(WS_MUTEX_TIMEOUT_MS)) != pdTRUE)
        return ESP_ERR_TIMEOUT;

    httpd_ws_frame_t frame = {
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = payload,
        .len = payload_len,
        .final = true,
    };

    esp_err_t err = httpd_ws_send_frame_async(handle, fd, &frame);

    xSemaphoreGive(s_mutex);

    return err;
}

static esp_err_t ws_register(httpd_req_t* req, const uint8_t* const payload, size_t payload_len) {
    // cJSON* root = cJSON_ParseWithLength((const char*)frame.payload, frame.len);
    // cJSON* token = cJSON_GetObjectItem(root, "token");
    // ESP_LOGI(TAG, "token: %s", token->valuestring);

    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(WS_MUTEX_TIMEOUT_MS)) != pdTRUE) {
        ws_close(req, "Time Out", 1013);
        return ESP_OK;
    }

    if (s_client_count >= CONFIG_WS_MAX_CLIENTS) {
        xSemaphoreGive(s_mutex);
        ws_close(req, "Server Full", 1013);
        return ESP_OK;
    }

    session_t* const sess = calloc(1, sizeof(session_t));
    if (!sess) {
        xSemaphoreGive(s_mutex);
        ws_close(req, "Memory Full", 1013);
        return ESP_OK;
    }

    sess->id = httpd_req_to_sockfd(req);

    req->sess_ctx = sess;
    req->free_ctx = free;

    s_client_fds[s_client_count++] = sess->id;

    xSemaphoreGive(s_mutex);

    cJSON* root = response_event_create(COMMAND_TYPE_AUTH, AUTH_ACTION_LOGIN, ESP_OK);
    if (!root) {
        ws_close(req, "Failed to create response (type=auth, action=login)", 1013);
        return ESP_OK;
    }

    cJSON* root_data = cJSON_CreateArray();

    command_ctx_t command_ctx;

    // WIFI_AP_BEGIN
    command_ctx.out.data = cJSON_CreateObject();
    int ret = command_execute(COMMAND_TYPE_WIFI_AP, WIFI_AP_ACTION_STATUS, &command_ctx).status;
    if (ret != COMMAND_STATUS_OK) {
        cJSON_Delete(command_ctx.out.data);
        cJSON_Delete(root);
        ws_close(req, "COMMAND_TYPE_WIFI_AP | WIFI_AP_ACTION_STATUS | WIFI_AP_ERR_INVALID_ARG", 1013);
        return ESP_OK;
    }
    cJSON_AddItemToArray(root_data, command_ctx.out.data);
    // WIFI_AP_END

    // WIFI_STA_BEGIN
    command_ctx.out.data = cJSON_CreateObject();
    if (command_execute(COMMAND_TYPE_WIFI_STA, WIFI_STA_ACTION_STATUS, &command_ctx).status != COMMAND_STATUS_OK) {
        cJSON_Delete(command_ctx.out.data);
        cJSON_Delete(root);
        ws_close(req, "COMMAND_TYPE_WIFI_STA | WIFI_STA_ACTION_STATUS | WIFI_STA_ERR_INVALID_ARG", 1013);
        return ESP_OK;
    }
    cJSON_AddItemToArray(root_data, command_ctx.out.data);
    // WIFI_STA_END

    cJSON_AddItemToObject(root, "data", root_data);

    char* json = cJSON_PrintUnformatted(root);
    if (!json) {
        cJSON_Delete(root);
        ws_close(req, "Internal Server Error", 1013);
        return ESP_OK;
    }

    ws_send(req->handle, sess->id, (uint8_t*)json, strlen(json));

    cJSON_free(json);
    cJSON_Delete(root);

    return ESP_OK;
}

static inline esp_err_t ws_prosess_text(httpd_req_t* req, const uint8_t* data, size_t data_len) {
    cJSON* payload = cJSON_ParseWithLengthOpts((const char*)data, data_len, 0, 0);
    if (!payload) {
        ws_close(req, "INVALID_PAYLOAD", WS_CLOSE_INVALID_PAYLOAD);
        return ESP_OK;
    }

    const cJSON* const payload_type = cJSON_GetObjectItem(payload, "type");
    if (!payload_type || !cJSON_IsNumber(payload_type)) {
        cJSON_Delete(payload);
        ws_close(req, "INVALID_PAYLOAD_TYPE", WS_CLOSE_INVALID_PAYLOAD);
        return ESP_OK;
    }
    command_type_t type = payload_type->valueint;

    const cJSON* const payload_action = cJSON_GetObjectItem(payload, "action");
    if (!payload_action || !cJSON_IsNumber(payload_action)) {
        cJSON_Delete(payload);
        ws_close(req, "INVALID_PAYLOAD_ACTION", WS_CLOSE_INVALID_PAYLOAD);
        return ESP_OK;
    }
    command_action_t action = payload_action->valueint;

    command_ctx_t ctx = {.in.data = payload, .out.data = cJSON_CreateObject()};
    if (!ctx.out.data) {
        ESP_LOGE(TAG, "Failed to allocate response JSON object, closing WebSocket connection (code=%d)", WS_CLOSE_INTERNAL_ERROR);
        cJSON_Delete(payload);
        ws_close(req, "INTERNAL_SERVER_ERROR", WS_CLOSE_INTERNAL_ERROR);
        return ESP_OK;
    }

    command_res_t command_res = command_execute(type, action, &ctx);

    cJSON_Delete(ctx.in.data);

    switch (command_res.status) {
        case COMMAND_STATUS_OK: {
            cJSON* root = response_event_create(type, action, ESP_OK);
            if (!cJSON_AddItemToObject(root, "data", ctx.out.data)) {
                ESP_LOGE(TAG, "Failed to create 'data' JSON object, closing WebSocket connection (code=%d)", WS_CLOSE_INTERNAL_ERROR);
                cJSON_Delete(root);
                cJSON_Delete(ctx.out.data);
                ws_close(req, "INTERNAL_SERVER_ERROR", WS_CLOSE_INTERNAL_ERROR);
                return ESP_OK;
            }

            char* json = cJSON_PrintUnformatted(root);
            if (!json) {
                ESP_LOGE(TAG, "Failed to serialize JSON response");
                cJSON_Delete(root);
                cJSON_Delete(ctx.out.data);
                ws_close(req, "INTERNAL_SERVER_ERROR", WS_CLOSE_INTERNAL_ERROR);
                return ESP_OK;
            }

            const session_t* const session = req->sess_ctx;

            ws_send(req->handle, session->id, (uint8_t*)json, strlen(json));

            cJSON_free(json);
            cJSON_Delete(root);

            return ESP_OK;
        }

        case COMMAND_STATUS_INVALID_TYPE: {
            ESP_LOGE(TAG, "Command status: COMMAND_STATUS_INVALID_TYPE");
            cJSON_Delete(ctx.out.data);
            ws_close(req, "COMMAND_STATUS_INVALID_TYPE", WS_CLOSE_INVALID_PAYLOAD);
            return ESP_OK;
        }

        case COMMAND_STATUS_INVALID_ACTION: {
            ESP_LOGE(TAG, "Command status: COMMAND_STATUS_INVALID_ACTION");
            cJSON_Delete(ctx.out.data);
            ws_close(req, "COMMAND_STATUS_INVALID_ACTION", WS_CLOSE_INVALID_PAYLOAD);
            return ESP_OK;
        }

        case COMMAND_STATUS_INVALID_CTX: {
            ESP_LOGE(TAG, "Command status: COMMAND_STATUS_INVALID_CTX");
            cJSON_Delete(ctx.out.data);
            ws_close(req, "COMMAND_STATUS_INVALID_CTX", WS_CLOSE_INVALID_PAYLOAD);
            return ESP_OK;
        }

        default: {
            ESP_LOGE(TAG, "Unknown command status: %d", command_res.status);
            cJSON_Delete(ctx.out.data);
            ws_close(req, "INTERNAL_SERVER_ERROR", WS_CLOSE_INTERNAL_ERROR);
            return ESP_OK;
        }
    }
}

static esp_err_t handler(httpd_req_t* req) {
    httpd_ws_frame_t httpd_ws_frame = {0};
    esp_err_t err;

    if ((err = httpd_ws_recv_frame(req, &httpd_ws_frame, 0)) != ESP_OK) {
        ws_close(req, esp_err_to_name(err), 1013);
        return ESP_OK;
    }

    if (httpd_ws_frame.len == 0) {
        return ESP_OK;
    }

    if (httpd_ws_frame.len > WS_MAX_PAYLOAD_SIZE) {
        ws_close(req, "REQUEST_TOO_LARGE", 1009);
        return ESP_OK;
    }

    uint8_t httpd_ws_frame_payload[httpd_ws_frame.len];
    httpd_ws_frame.payload = httpd_ws_frame_payload;
    if ((err = httpd_ws_recv_frame(req, &httpd_ws_frame, httpd_ws_frame.len))) {
        ws_close(req, esp_err_to_name(err), 1013);
        return ESP_OK;
    }

    switch (httpd_ws_frame.type) {
        case HTTPD_WS_TYPE_TEXT: {
            if (!req->sess_ctx) {
                return ws_register(req, httpd_ws_frame.payload, httpd_ws_frame.len);
            }

            return ws_prosess_text(req, httpd_ws_frame.payload, httpd_ws_frame.len);
        }

        case HTTPD_WS_TYPE_BINARY: {
            if (!req->sess_ctx) {
                ws_close(req, "INVALID_CREDENTIAL", 1013);
                return ESP_FAIL;
            }

            ws_send(req->handle, httpd_req_to_sockfd(req), httpd_ws_frame.payload, httpd_ws_frame.len);
            return ESP_OK;
        }

        case HTTPD_WS_TYPE_CLOSE: {
            uint16_t close_code = 0;
            if (httpd_ws_frame.len >= 2) {
                close_code = ((uint16_t)httpd_ws_frame.payload[0] << 8) | httpd_ws_frame.payload[1];
            }
            ws_close(req, "", close_code);
            return ESP_OK;
        }

        default:
            return ESP_OK;
    }
}

static const httpd_uri_t uri_status = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = handler,
    .is_websocket = true,
    .handle_ws_control_frames = true,
};

static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    httpd_handle_t handle = arg;
    if (event_id == WIFI_EVENT_STA_CONNECTED) {
        const wifi_event_sta_connected_t* event = event_data;
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "type", "wifi_sta");
        cJSON_AddStringToObject(root, "action", "connect");
        cJSON_AddNumberToObject(root, "status", ESP_OK);
        cJSON* data = cJSON_CreateObject();
        char ssid_buf[33] = {0};
        memcpy(ssid_buf, event->ssid, event->ssid_len);
        ssid_buf[event->ssid_len] = '\0';
        cJSON_AddStringToObject(data, "ssid", ssid_buf);
        cJSON_AddStringToObject(data, "message", "connected");
        cJSON_AddItemToObject(root, "data", data);
        char* json = cJSON_PrintUnformatted(root);
        if (json == NULL) {
            cJSON_Delete(root);
            return;
        }
        ws_broadcast(handle, (uint8_t*)json, strlen(json), HTTPD_WS_TYPE_TEXT);
        cJSON_free(json);
        cJSON_Delete(root);
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t* event = event_data;
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "type", "wifi_sta");
        cJSON_AddStringToObject(root, "action", "disconnect");
        cJSON_AddNumberToObject(root, "status", ESP_OK);
        cJSON* data = cJSON_CreateObject();
        char ssid_buf[33] = {0};
        memcpy(ssid_buf, event->ssid, event->ssid_len);
        ssid_buf[event->ssid_len] = '\0';
        cJSON_AddStringToObject(data, "ssid", ssid_buf);
        cJSON_AddStringToObject(data, "message", "disconnected");
        cJSON_AddItemToObject(root, "data", data);
        char* json = cJSON_PrintUnformatted(root);
        if (json == NULL) {
            cJSON_Delete(root);
            return;
        }
        ws_broadcast(handle, (uint8_t*)json, strlen(json), HTTPD_WS_TYPE_TEXT);
        cJSON_free(json);
        cJSON_Delete(root);
    } else if (event_id == WIFI_EVENT_SCAN_DONE) {
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);

        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "type", "wifi_sta");
        cJSON_AddStringToObject(root, "action", "scan");
        cJSON_AddNumberToObject(root, "status", ESP_OK);
        cJSON* data = cJSON_CreateArray();
        cJSON_AddItemToObject(root, "data", data);
        if (ap_count == 0) {
            char* json = cJSON_PrintUnformatted(root);
            if (json == NULL) {
                cJSON_Delete(root);
                return;
            }

            ws_broadcast(handle, (uint8_t*)json, strlen(json), HTTPD_WS_TYPE_TEXT);
            return;
        }

        wifi_ap_record_t* ap_records = malloc(sizeof(wifi_ap_record_t) * ap_count);
        if (ap_records == NULL) {
            cJSON_Delete(root);
            ESP_LOGE(TAG, "Out of memory for AP records");
            return;
        }

        esp_err_t err = esp_wifi_scan_get_ap_records(&ap_count, ap_records);
        if (err != ESP_OK) {
            free(ap_records);
            cJSON_Delete(root);
            return;
        }

        for (int i = 0; i < ap_count; i++) {
            cJSON* ap = cJSON_CreateObject();
            cJSON_AddStringToObject(ap, "ssid", (char*)ap_records[i].ssid);
            cJSON_AddNumberToObject(ap, "authmode", ap_records[i].authmode);
            cJSON_AddNumberToObject(ap, "rssi", ap_records[i].rssi);
            cJSON_AddItemToArray(data, ap);
        }

        free(ap_records);

        char* json = cJSON_PrintUnformatted(root);
        if (json == NULL) {
            cJSON_Delete(root);
            return;
        }

        ws_broadcast(handle, (uint8_t*)json, strlen(json), HTTPD_WS_TYPE_TEXT);

        cJSON_free(json);
        cJSON_Delete(root);
    }
}

static void wifi_ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    const httpd_handle_t handle = arg;
    switch (event_id) {
        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t* event = event_data;
            command_ctx_t c = {.in.data = event->mac};
            if (command_execute(COMMAND_TYPE_WIFI_AP, WIFI_AP_ACTION_STACONNECTED, &c).status == 0) {
                cJSON* root = response_event_create(COMMAND_TYPE_WIFI_STA, WIFI_AP_ACTION_STACONNECTED, ESP_OK);
                cJSON* root_data = cJSON_CreateObject();
                char mac_str[18];
                snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(event->mac));
                cJSON_AddStringToObject(root_data, "mac", mac_str);
                cJSON_AddNullToObject(root_data, "ip");
                cJSON_AddNullToObject(root_data, "hostname");
                cJSON_AddItemToObject(root, "data", root_data);

                response_event_send(handle, root);
            }
            break;
        }

        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t* event = event_data;
            command_ctx_t c = {.in.data = event->mac};
            if (command_execute(COMMAND_TYPE_WIFI_AP, WIFI_AP_ACTION_STADISCONNECTED, &c).status == 0) {
                cJSON* root = response_event_create(COMMAND_TYPE_WIFI_AP, WIFI_AP_ACTION_STADISCONNECTED, ESP_OK);
                cJSON* root_data = cJSON_CreateObject();
                char mac_str[18];
                snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(event->mac));
                cJSON_AddStringToObject(root_data, "mac", mac_str);
                cJSON_AddNumberToObject(root_data, "aid", event->aid);
                cJSON_AddNumberToObject(root_data, "reason", event->reason);

                cJSON_AddItemToObject(root, "data", root_data);

                response_event_send(handle, root);
            }
            break;
        }
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    const httpd_handle_t handle = arg;
    switch (event_id) {
        case IP_EVENT_STA_GOT_IP: {
            const ip_event_got_ip_t* event = event_data;
            ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
            break;
        }
        case IP_EVENT_ASSIGNED_IP_TO_CLIENT: {
            const ip_event_assigned_ip_to_client_t* event = event_data;
            cJSON* root = response_event_create(COMMAND_TYPE_WIFI_AP, WIFI_AP_ACTION_ASSIGNED_IP_TO_CLIENT, ESP_OK);
            cJSON* root_data = cJSON_CreateObject();
            char mac_str[18];
            snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(event->mac));
            cJSON_AddStringToObject(root_data, "mac", mac_str);
            char ip_str[16];
            const esp_ip4_addr_t ip = event->ip;
            esp_ip4addr_ntoa(&ip, ip_str, sizeof(ip_str));
            cJSON_AddStringToObject(root_data, "ip", ip_str);
            cJSON_AddStringToObject(root_data, "hostname", event->hostname);
            ESP_LOGI("TAG", "mac: %s", mac_str);
            command_ctx_t c = {.in.data = root_data};
            int ret = command_execute(COMMAND_TYPE_WIFI_AP, WIFI_AP_ACTION_ASSIGNED_IP_TO_CLIENT, &c).status;
            if (ret == 0) {
                cJSON_AddItemToObject(root, "data", root_data);
                response_event_send(handle, root);
            }
            break;
        }
    }
}

esp_err_t ws_register_uri(httpd_handle_t handle) {
    if (s_mutex)
        return ESP_OK;

    s_mutex = xSemaphoreCreateMutex();

    esp_err_t err = httpd_register_uri_handler(handle, &uri_status);
    if (err) {
        s_mutex = NULL;
    } else {
        char ip_str[16] = "[ip]";
        command_ctx_t c = {.in.data = ip_str};
        command_execute(COMMAND_TYPE_WIFI_AP, WIFI_AP_ACTION_GET_IP, &c);
        ESP_LOGI(TAG, "WebSocket server started → ws://%s%s", ip_str, strlen(uri_status.uri) == 1 ? "" : uri_status.uri);

        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, handle, NULL));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &wifi_sta_event_handler, handle, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &wifi_sta_event_handler, handle, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_sta_event_handler, handle, NULL));

        err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ASSIGNED_IP_TO_CLIENT, &ip_event_handler, handle, NULL);
        if (err) return err;

        err = esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifi_ap_event_handler, handle, NULL);
        if (err) return err;

        err = esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &wifi_ap_event_handler, handle, NULL);
        if (err) return err;
    }
    return err;
}
