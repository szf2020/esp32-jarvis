#include "Arduino.h"
#include "esp_camera.h"
#include "esp_http_server.h"

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>ESP32-S3 Neural Vision</title>
    <style>
      body { background: #111; color: #eee; font-family: sans-serif; text-align: center; }
      img { border: 2px solid #0f0; max-width: 100%; border-radius: 8px; }
    </style>
  </head>
  <body>
    <h2>Neural Uplink</h2>
    <img src="/stream" id="stream">
    <script>
      const img = document.getElementById('stream');
      img.onerror = () => { setTimeout(() => { img.src = "/stream?t=" + Date.now(); }, 1000); };
    </script>
  </body>
</html>
)rawliteral";

class StreamServer {
private:
    httpd_handle_t httpd = NULL;

    static esp_err_t index_handler(httpd_req_t *req) {
        httpd_resp_set_type(req, "text/html");
        return httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    }

    static esp_err_t stream_handler(httpd_req_t *req) {
        camera_fb_t * fb = NULL;
        esp_err_t res = ESP_OK;
        char part_buf[64];

        res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
        if(res != ESP_OK) return ESP_FAIL;

        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

        while (true)
        {
            fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("Frame Drop!");
                res = ESP_FAIL;
            } else {
                if (res == ESP_OK) {
                    size_t hlen = snprintf(part_buf, 64, "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
                    res = httpd_resp_send_chunk(req, part_buf, hlen);
                }
                if (res == ESP_OK) {
                    res = httpd_resp_send_chunk(req, (const char *) fb->buf, fb->len);
                }
                if (res == ESP_OK) {
                    res = httpd_resp_send_chunk(req, "\r\n--frame\r\n", 12);
                }
                esp_camera_fb_return(fb);
            }
            if(res != ESP_OK) break;
        }
        return res;
    }
public: 
    void start() {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.server_port = 80;

        httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler= index_handler, .user_ctx = NULL};
        httpd_uri_t stream_uri = { .uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL};

        Serial.printf("Starting web server on port %d\n", config.server_port);
        if (httpd_start(&httpd, &config) == ESP_OK) {
            httpd_register_uri_handler(httpd, &index_uri);
            httpd_register_uri_handler(httpd, &stream_uri);
            Serial.println(">>> Server ready <<<");
        } else {
            Serial.println("!!! Server Start Failed !!!");
        }
    }
};