#include "Arduino.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "esp_http_server.h"

const char* ssid = "MOVISTAR_6A18";
const char* password = "TYRYTVCJTYVKX4TbJYPY";

#define PWDN_GPIO_NUM   -1
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM   10
#define SIOD_GPIO_NUM   40
#define SIOC_GPIO_NUM   39

#define Y9_GPIO_NUM     48
#define Y8_GPIO_NUM     11
#define Y7_GPIO_NUM     12
#define Y6_GPIO_NUM     14
#define Y5_GPIO_NUM     16
#define Y4_GPIO_NUM     18
#define Y3_GPIO_NUM     17
#define Y2_GPIO_NUM     15
#define VSYNC_GPIO_NUM  38
#define HREF_GPIO_NUM   47
#define PCLK_GPIO_NUM   13

// HTTP Server Logic
httpd_handle_t stream_httpd = NULL;

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>ESP32-S3 Sense Stream</title>
    <style>
      body { font-family: Arial; text-align: center; margin: 0px; padding: 0px; background: #222; color: #fff; }
      h1 { margin: 10px; }
      img { width: 100%; max-width: 640px; height: auto; border: 4px solid #fff; border-radius: 4px; }
      .foot { margin-top: 20px; color: #aaa; font-size: 0.8rem; }
    </style>
  </head>
  <body>
    <h1>Live Vision</h1>
    <img src="/stream" id="stream-view">
    <div class="foot">Powered by XIAO ESP32S3</div>
    <script>
      // Script auto-reconnect jika stream putus
      const img = document.getElementById('stream-view');
      img.onload = () => { console.log('Frame received'); };
      img.onerror = () => { 
        console.log('Stream lost, retrying...');
        setTimeout(() => { img.src = "/stream?t=" + new Date().getTime(); }, 1000);
      };
    </script>
  </body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
}
static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];

    // --- FIX START: GUNAKAN MIME TYPE STANDAR ---
    // Jangan pakai "_stream". Safari butuh kejelasan.
    res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
    // --- FIX END ---
    
    if(res != ESP_OK) {
        return ESP_FAIL;
    }

    // Set CORS agar tidak diblokir browser modern (Opsional tapi bagus)
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    // Loop Streaming
    while (true)
    {
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if (res == ESP_OK) {
            // Header per frame (Sama seperti tutorial referensimu)
            size_t hlen = snprintf((char *)part_buf, 64, "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if (res == ESP_OK){
            // Boundary penutup frame
            res = httpd_resp_send_chunk(req, "\r\n--frame\r\n", 12);
        }
        
        // Return buffer ke driver
        if (fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if (_jpg_buf) {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }

        if (res != ESP_OK) {
            // Jika klien (browser) menutup tab, send_chunk akan gagal
            // Break loop untuk menghemat CPU
            break;
        }
    }
    return res;
}

void startCameraServer(){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_uri_t index_uri = {
        .uri    = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL
    };

    httpd_uri_t stream_uri = {
        .uri    = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL
    };


    Serial.printf("Attempting to start server on port: '%d'\n", config.server_port);

    esp_err_t res = httpd_start(&stream_httpd, &config);

    if(res == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &index_uri);
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        Serial.println(">>> SERVER STARTED SUCCESSFULLY! <<<");
    } else {
        Serial.printf("!!! Server Failed to Start!! error Code: 0x%x\n", res);
        Serial.println("Check: Sisa Memory (Heap)? Konflik Port?");
    }
};

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    delay(2000);
    Serial.println();

    // checking the PSRAM
    if (psramFound()){
        Serial.println("PSRAM Found@ (Good, kita butuh ini untuk buffer video!)");
    } else {
        Serial.println("FATAL ERROR! PSRAM Not found. Enable OPI PSRAM do Tools!");
        return;
    }

    // camera config
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_QVGA;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 2;

    // Init camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    // decrease the frame size 
    sensor_t * s = esp_camera_sensor_get();
    if (s->id.PID == OV2640_PID) {
        s->set_framesize(s, FRAMESIZE_HVGA);
    } 

    // trying to connect to the wifi.
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);

    Serial.print("WFi Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("Try to connect...\n");
    }
    Serial.println("");
    Serial.println("Wifi Connected.");

    // Start Server
    startCameraServer();
    Serial.print("Camera Ready! \nuse 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("/' to Connect");
}

void loop() {
    delay(10000);
}   
