#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include "esp_camera.h"
#include <TimeLib.h>  // Library to handle time functions

// Define screen dimensions for the TFT display
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);  // Adjust for your display

// Variables for tracking screens
enum Screen { HOME_SCREEN, CAMERA_SCREEN };
Screen currentScreen = HOME_SCREEN;

// LVGL Objects
lv_obj_t *clock_label, *image_widget, *intensity_label;

// Real-time update tasks
lv_timer_t *clock_task, *camera_task;

void setup() {
  // Initialize the display and LVGL
  tft.begin();
  tft.setRotation(0);
  lv_init();
  lv_disp_draw_buf_t draw_buf;
  lv_disp_drv_t disp_drv;
  static lv_color_t buf[SCREEN_WIDTH * 10];                         // Buffer for display
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 10);    // Initialize buffer
  lv_disp_drv_init(&disp_drv);
  disp_drv.flush_cb = my_disp_flush;                                // Function to flush the buffer
  disp_drv.draw_buf = &draw_buf;
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HEIGHT;
  lv_disp_drv_register(&disp_drv);

  // Initialize time to a default value, or sync with RTC if available
  setTime(12, 0, 0, 1, 1, 2023);  // Set to 12:00:00 on Jan 1, 2023
  
  // Load home screen with retro clock
  loadHomeScreen();
}

void loop() {
  lv_timer_handler();  // Handle LVGL tasks
  delay(5);            // Small delay to avoid overload
}

// Display flush callback for LVGL
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
  tft.pushColors((uint16_t *)&color_p->full, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1), true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

// Handle swipe events
void swipe_event_handler(lv_event_t *e) {
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  if (dir == LV_DIR_LEFT) {
    switch (currentScreen) {
      case HOME_SCREEN:
        loadCameraScreen();
        break;
      case CAMERA_SCREEN:
        loadHomeScreen();
        break;
    }
  }
}

// Load the home screen with the clock
void loadHomeScreen() {
  lv_obj_clean(lv_scr_act());  // Clear current screen
  currentScreen = HOME_SCREEN;

  // Retro-style clock
  clock_label = lv_label_create(lv_scr_act());
  lv_label_set_text(clock_label, "00:00:00");
  lv_obj_align(clock_label, LV_ALIGN_CENTER, 0, 0);

  // Create a task to update the clock every second
  if (!clock_task) {
    clock_task = lv_timer_create(updateClock, 1000, NULL);
  }
}

// Update clock label
void updateClock(lv_timer_t *task) {
  char timeString[10];
  sprintf(timeString, "%02d:%02d:%02d", hour(), minute(), second());  // Get system time
  lv_label_set_text(clock_label, timeString);
}

// Load camera screen
void loadCameraScreen() {
  lv_obj_clean(lv_scr_act());  // Clear current screen
  currentScreen = CAMERA_SCREEN;

  // Create image and intensity widgets
  image_widget = lv_img_create(lv_scr_act());
  lv_obj_align(image_widget, LV_ALIGN_CENTER, 0, 0);

  intensity_label = lv_label_create(lv_scr_act());
  lv_obj_align(intensity_label, LV_ALIGN_BOTTOM_MID, 0, -10);

  // Create a task to capture images and update intensity every 15 seconds
  if (!camera_task) {
    camera_task = lv_timer_create(captureImageAndCalculateIntensity, 15000, NULL);
  }
}

// Capture image and calculate mean intensity
void captureImageAndCalculateIntensity(lv_timer_t *task) {
  camera_fb_t *fb = esp_camera_fb_get();  // Capture image

  // Calculate mean intensity
  int meanIntensity = calculateMeanIntensity(fb);
  char intensityString[20];
  sprintf(intensityString, "Intensity: %d", meanIntensity);
  lv_label_set_text(intensity_label, intensityString);

  // Display captured image
  lv_img_set_src(image_widget, fb->buf);

  esp_camera_fb_return(fb);  // Return frame buffer to release memory
}

// Calculate mean intensity of the image
int calculateMeanIntensity(camera_fb_t *fb) {
  long sum = 0;
  for (int i = 0; i < fb->len; i++) {
    sum += fb->buf[i];
  }
  return sum / fb->len;
}
