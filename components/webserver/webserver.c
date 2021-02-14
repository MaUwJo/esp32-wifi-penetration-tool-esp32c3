// References: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html

#include "webserver.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_wifi_types.h"

#include "wifi_controller.h"

#include "include/pages.h"


static const char* TAG = "webserver";

static esp_err_t uri_root_get_handler(httpd_req_t *req) {
    return httpd_resp_send(req, page_root, HTTPD_RESP_USE_STRLEN);
}

static httpd_uri_t uri_root_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = uri_root_get_handler,
    .user_ctx = NULL
};

static esp_err_t uri_ap_list_get_handler(httpd_req_t *req) {
    uint16_t ap_max_count = 20;
    wifi_ap_record_t ap_records[ap_max_count];

    wifictl_scan_nearby_aps(&ap_max_count, ap_records);
    ESP_LOGI(TAG, "Found %u APs.", ap_max_count);

    // 33 SSID + 6 BSSID + 1 RSSI
    char resp_chunk[40];
    
    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    for(unsigned i = 0; i < ap_max_count; i++){
        memcpy(resp_chunk, ap_records[i].ssid, 33);
        memcpy(&resp_chunk[33], ap_records[i].bssid, 6);
        memcpy(&resp_chunk[39], &ap_records[i].rssi, 1);
        ESP_ERROR_CHECK(httpd_resp_send_chunk(req, resp_chunk, 40));
    }
    return httpd_resp_send_chunk(req, resp_chunk, 0);
}

static httpd_uri_t uri_ap_list_get = {
    .uri = "/ap-list",
    .method = HTTP_GET,
    .handler = uri_ap_list_get_handler,
    .user_ctx = NULL
};

static esp_err_t uri_ap_select_post_handler(httpd_req_t *req) {
    char ap_record_id;
    // TODO - returns number of bytes
    httpd_req_recv(req, &ap_record_id, 1);
    ESP_LOGD(TAG, "Using AP with ID %u", ap_record_id);
    return httpd_resp_send(req, NULL, 0);
}

static httpd_uri_t uri_ap_select_post = {
    .uri = "/ap-select",
    .method = HTTP_POST,
    .handler = uri_ap_select_post_handler,
    .user_ctx = NULL
};

void webserver_run(){
    ESP_LOGD(TAG, "Running webserver");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_root_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_ap_list_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_ap_select_post));
}