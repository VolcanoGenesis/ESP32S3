#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include <TFT_eSPI.h>
#include "esp_camera.h"
#include "camera_pins.h"
#include <Wire.h>

// Initialize the display
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

// Display settings
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 240

void setup() {
  Serial.begin(115200);
  delay(1000); // Give time for serial to initialize
  
  Serial.println("ESP32-S3 Camera Test");
  
  // Initialize I2C with a lower clock speed
  Wire.begin(SIOD_GPIO_NUM, SIOC_GPIO_NUM, 100000); // 100kHz I2C clock
  
  // Initialize display
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  
  Serial.println("Display initialized");
  
  // Add a delay before initializing the camera
  delay(2000);
  
  // Initialize camera
  setupCamera();
}

void loop() {
  displayCameraFeed();
  delay(100);
}

void setupCamera() {
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

  config.xclk_freq_hz = 25000000; // 20MHz
  config.frame_size = FRAMESIZE_240X240; // Change to 240x240 to match display
  config.pixel_format = PIXFORMAT_RGB565; // RGB format for color display
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM; // Use PSRAM for larger frames
  config.jpeg_quality = 12; // Adjust quality for balance
  config.fb_count = 1; // Single frame buffer
  
  Serial.println("Starting camera initialization...");
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  Serial.println("Camera initialized successfully.");
  
  sensor_t * s = esp_camera_sensor_get();
  if (!s) {
    Serial.println("Failed to get camera sensor.");
    return;
  }

  // Apply some sensor settings
  s->set_brightness(s, 1);  // Increase brightness
  s->set_contrast(s, 1);    // Increase contrast
  s->set_saturation(s, 1);  // Increase saturation
  s->set_whitebal(s, 1);    // Enable white balance
  s->set_awb_gain(s, 1);    // Enable AWB gain
  s->set_exposure_ctrl(s, 1); // Enable auto exposure

  Serial.printf("Camera sensor ID: 0x%x\n", s->id.PID);
}

void displayCameraFeed() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  Serial.printf("Frame captured: %dx%d %d-bit\n", fb->width, fb->height, fb->format == PIXFORMAT_RGB565 ? 16 : 8);
  
  // Create sprite for display
  sprite.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);

  // Resize and push image to the display (240x240 fits perfectly now)
  sprite.pushImage(0, 0, fb->width, fb->height, (uint16_t *)fb->buf);
  sprite.pushSprite(0, 0);

  // Release the frame buffer back to the driver
  esp_camera_fb_return(fb);
}
