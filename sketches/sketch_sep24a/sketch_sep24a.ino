#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include <TFT_eSPI.h>
#include "esp_camera.h"
#include "camera_pins.h"
#include <Wire.h>
#include <lvgl.h>
#include "lv_xiao_round_screen.h"
#include <FS.h>
#include <SD_MMC.h>

// SD card settings
bool sdCardInitialized = false;

// Function declarations
void setupCamera();
void captureAndSaveImage();

// Button object
lv_obj_t *capture_btn;

// Callback for the button press event
static void capture_event_handler(lv_event_t *e) {
    captureAndSaveImage();
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Give time for serial to initialize
  
  Serial.println("ESP32-S3 Camera Test");

  // Initialize LVGL
  lv_init();
  lv_xiao_disp_init();
  lv_xiao_touch_init(); // Initialize touch

  // Initialize camera
  setupCamera();

  // Initialize SD card
  if (!SD_MMC.begin()) {
    Serial.println("SD Card Mount Failed");
  } else {
    Serial.println("SD Card Mount Successful");
    sdCardInitialized = true;
  }

  // Create a button using LVGL
  capture_btn = lv_btn_create(lv_scr_act());
  lv_obj_align(capture_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_obj_set_size(capture_btn, 100, 50);
  
  lv_obj_t *label = lv_label_create(capture_btn);
  lv_label_set_text(label, "CAPTURE");
  
  // Assign an event handler for the button
  lv_obj_add_event_cb(capture_btn, capture_event_handler, LV_EVENT_CLICKED, NULL);
}

void loop() {
  lv_timer_handler(); // Handle LVGL tasks
  delay(5);
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // Initialize camera
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
  } else {
    Serial.println("Camera init success");
  }
}

void captureAndSaveImage() {
  if (!sdCardInitialized) {
    Serial.println("SD Card not initialized, can't save image");
    return;
  }

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Save the image to the SD card
  String path = "/captured_image.jpg";
  fs::FS &fs = SD_MMC;
  fs::File file = fs.open(path.c_str(), FILE_WRITE);
  
  if (!file) {
    Serial.println("Failed to open file for writing");
  } else {
    file.write(fb->buf, fb->len);  // Write the image buffer to file
    Serial.println("Image saved to: " + path);
  }
  
  file.close();
  esp_camera_fb_return(fb);  // Return the frame buffer back to the driver
}
