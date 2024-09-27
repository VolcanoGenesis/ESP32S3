#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "esp_camera.h"
#include "camera_pins.h"
#include <TFT_eSPI.h>

// Create TFT object
TFT_eSPI tft = TFT_eSPI(); 

void setup() {
  Serial.begin(115200);
  delay(1000); // Give time for serial to initialize
  Serial.println("ESP32-S3 Camera Test");

  // Initialize the TFT display
  tft.init();
  tft.setRotation(1); // Rotate the display 90 degrees to the left
  tft.fillScreen(TFT_BLACK);

  // Initialize the camera
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565; // Use RGB565 for simplicity
  config.frame_size = FRAMESIZE_240X240;
  config.jpeg_quality = 10; // Lower value means better quality
  config.fb_count = 1;

  // Initialize the camera
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
  } else {
    Serial.println("Camera init success");
  }
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();  // Capture a new frame
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Push the RGB565 buffer directly to the TFT
  tft.pushImage(0, 0, 240, 240, (uint16_t*)fb->buf); // Center crop for 240x240 screen

  esp_camera_fb_return(fb);  // Return the frame buffer back to the driver
  delay(30);  // Short delay for stability
}
