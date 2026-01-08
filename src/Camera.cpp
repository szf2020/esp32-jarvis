#include "esp_camera.h"
#include "Arduino.h"
#include "pins_config.h"

class Camera {
public:
    static bool init() {
        camera_config_t config;
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer = LEDC_TIMER_0;
        config.pin_d0 = Pins::Y2;
        config.pin_d1 = Pins::Y3;
        config.pin_d2 = Pins::Y4;
        config.pin_d3 = Pins::Y5;
        config.pin_d4 = Pins::Y6;
        config.pin_d5 = Pins::Y7;
        config.pin_d6 = Pins::Y8;
        config.pin_d7 = Pins::Y9;
        config.pin_xclk = Pins::XCLK;
        config.pin_pclk = Pins::PCLK;
        config.pin_vsync = Pins::VSYNC;
        config.pin_href = Pins::HREF;
        config.pin_sccb_sda = Pins::SIOD;
        config.pin_sccb_scl = Pins::SIOC;
        config.pin_pwdn = Pins::PWDN;
        config.pin_reset = Pins::RESET;
        config.xclk_freq_hz = 20000000;
        config.pixel_format = PIXFORMAT_JPEG;

        if(psramFound()) {
            config.frame_size = FRAMESIZE_HVGA;
            config.jpeg_quality = 12;
            config.fb_count = 2;
            config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
            config.fb_location = CAMERA_FB_IN_DRAM;
        } else {
            config.frame_size = FRAMESIZE_QVGA;
            config.jpeg_quality = 12;
            config.fb_count = 1;
            config.fb_location = CAMERA_FB_IN_DRAM;
        }

        esp_err_t err = esp_camera_init(&config);
        if (err!= ESP_OK) {
            Serial.printf("Camera init failed: 0x%x\n", err);
            return false;
        }
        return true;
    }
    static sensor_t* getSensor() {
        return esp_camera_sensor_get();
    }
};