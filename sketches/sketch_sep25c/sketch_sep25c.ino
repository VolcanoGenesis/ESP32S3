#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "esp_camera.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "JPEGDecoder.h" // For decoding JPEG images

#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#define TOUCH_INT D7

#include "camera_pins.h"

// Width and height of round display
const int camera_width = 240;
const int camera_height = 240;

// File Counter (not needed if SD card is not used)
int imageCount = 1;
bool camera_sign = false; // Check camera status
bool sd_sign = false;     // Not used, as SD card is not initialized

TFT_eSPI tft = TFT_eSPI();

bool display_is_pressed(void)
{
    if (digitalRead(TOUCH_INT) != LOW)
    {
        delay(3);
        if (digitalRead(TOUCH_INT) != LOW)
            return false;
    }
    return true;
}

void drawJPEG(uint8_t *buf, uint32_t len) {
    // Feed JPEGDecoder library with the JPEG buffer
    JpegDec.decodeArray(buf, len);

    // Retrieve image dimensions
    int16_t *pImg = JpegDec.pImage;
    int jpegWidth = JpegDec.width;
    int jpegHeight = JpegDec.height;

    tft.startWrite();
    tft.setAddrWindow(0, 0, camera_width, camera_height);
    uint16_t *colors = (uint16_t *)pImg; // Each pixel in RGB565 format

    for (int y = 0; y < jpegHeight; y++) {
        tft.pushColors(colors + (y * jpegWidth), jpegWidth);
    }
    tft.endWrite();
}

void setup()
{
    // Initialize Serial for debugging
    Serial.begin(115200);

    // Camera pinout
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

    // Use higher resolution and JPEG format for better image quality
    config.frame_size = FRAMESIZE_240X240;    // Keep the frame size same as display
    config.pixel_format = PIXFORMAT_JPEG;     // Use JPEG format for better quality
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;  // Store frame buffer in PSRAM for larger space
    config.jpeg_quality = 10;                 // Lower value improves quality
    config.fb_count = 2;                      // Double buffer for smoother display

    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err == ESP_OK)
    {
        Serial.println("Camera initialized successfully");
        camera_sign = true;
    }
    else
    {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return; // Stop if camera fails
    }

    // Display initialization
    tft.init();
    tft.setRotation(1);

    // Test display by filling the screen with a solid color (e.g., blue)
    tft.fillScreen(TFT_BLUE);
    delay(2000); // Wait for 2 seconds to verify that the display is working

    // Clear the screen for the camera feed
    tft.fillScreen(TFT_WHITE);
}

void loop()
{
    if (camera_sign)
    {
        // Capture a frame from the camera
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            Serial.println("Failed to get camera frame buffer");
            return;
        }

        // Display some debug information about the frame buffer
        Serial.printf("Frame buffer length: %d bytes\n", fb->len);

        // If the captured frame is JPEG, decode and display it
        if (fb->format == PIXFORMAT_JPEG)
        {
            drawJPEG(fb->buf, fb->len);
        }

        // Check if the display is touched (although this functionality is still here, it doesn't involve SD saving)
        if (display_is_pressed())
        {
            Serial.println("Display is touched, but no SD card to save the image.");
        }

        // Release the image buffer
        esp_camera_fb_return(fb);

        delay(100); // Adjust delay for smoother frame rate
    }
}
