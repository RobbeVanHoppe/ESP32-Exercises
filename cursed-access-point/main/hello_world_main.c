#include <stdio.h>
#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

static const char *TAG = "main";

#define WIFI_SSID "ESP32-AP"
#define WIFI_PASS "password123"

static esp_err_t index_get_handler(httpd_req_t *req);
static esp_err_t submit_post_handler(httpd_req_t *req);

httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_get_handler,
    .user_ctx  = NULL
};

httpd_uri_t submit_uri = {
    .uri       = "/submit",
    .method    = HTTP_POST,
    .handler   = submit_post_handler,
    .user_ctx  = NULL
};

static const char *index_html = "<!DOCTYPE html>"
"<html>"
"<head><title>ESP32 AP</title></head>"
"<body>"
"<h1>Register / Login</h1>"
"<form action=\"/submit\" method=\"POST\">"
"Email: <input type=\"email\" name=\"email\"><br>"
"Password: <input type=\"password\" name=\"password\"><br>"
"<input type=\"submit\" value=\"Submit\">"
"</form>"
"</body>"
"</html>";

static esp_err_t index_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t submit_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    
    buf[req->content_len] = '\0';
    ESP_LOGI(TAG, "Received form data: %s", buf);

    // Parse email and password (simple parsing, not robust)
    char *email = strtok(buf, "&");
    char *password = strtok(NULL, "&");
    
    if (email && password) {
        ESP_LOGI(TAG, "Email: %s", email);
        ESP_LOGI(TAG, "Password: %s", password);
        httpd_resp_send(req, "Registration successful", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send(req, "Invalid input", HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}

void wifi_init_softap() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             WIFI_SSID, WIFI_PASS);
}

static httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &submit_uri);
    }
    return server;
}

void app_main() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
    start_webserver();
}
