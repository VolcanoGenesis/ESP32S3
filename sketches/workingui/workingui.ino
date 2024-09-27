#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>

// For camera and SD functionality
#include "esp_camera.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define SD_CS_PIN D2
#define TOUCH_INT D7
#include "camera_pins.h"
TFT_eSPI tft = TFT_eSPI();
bool onCameraScreen = false;
bool camera_sign = false;
bool sd_sign = false;

int imageCount = 1;

// WiFi Credentials
const char* ssid = "S340 7049";        // Replace with your WiFi network name
const char* password = "17122004"; // Replace with your WiFi password

// NTP server and timezone
const char* ntpServer = "time.google.com"; // Google's NTP server
const long  gmtOffset_sec = 19800;         // Set your timezone offset (5.5 hours for India)
const int   daylightOffset_sec = 0;        // Set for daylight savings (if applicable)

// Function declarations
void displayTimeScreen();
void displayCameraScreen();
bool display_is_pressed();
void writeFile(fs::FS &fs, const char * path, uint8_t * data, size_t len);
void takePhotoAndSave();
void initWiFi();
void initTime();

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("\n\n--- Starting setup ---");
  
  // Initialize WiFi first
  initWiFi();

  // Initialize display
  Serial.println("Initializing display...");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);
  Serial.println("Display initialized");
  
  // Get the time
  if (WiFi.status() == WL_CONNECTED) {
    initTime();
  } else {
    Serial.println("Skipping time sync due to Wi-Fi failure");
  }

  // Initialize SD card
  Serial.println("Initializing SD card...");
  if(!SD.begin(SD_CS_PIN)){
    Serial.println("Card Mount Failed");
  } else {
    uint8_t cardType = SD.cardType();
    if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
    } else {
      Serial.println("SD card initialized successfully");
      sd_sign = true;
    }
  }

  // Initialize Camera
  Serial.println("Initializing camera...");
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
  config.xclk_freq_hz = 25000000;
  config.frame_size = FRAMESIZE_240X240;
  config.pixel_format = PIXFORMAT_RGB565;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
  } else {
    Serial.println("Camera initialized successfully");
    camera_sign = true;

    // Test camera capture
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      camera_sign = false;
    } else {
      Serial.printf("Captured image. Size: %zu bytes\n", fb->len);
      esp_camera_fb_return(fb);
    }
  }

  Serial.println("--- Setup complete ---");
  displayTimeScreen();  // Start with time screen
}

void loop() {
  if (onCameraScreen) {
    displayCameraScreen();  // Switch to camera functionality
  } else {
    displayTimeScreen();  // Time screen
  }

  delay(1000); // Adjust this to control screen refresh rate
}

// Display the time and button on the first screen
// Global variable to store the time when the text animation started
unsigned long animationStartTime = 0;  
int currentColor = 0; // 0 = Red, 1 = Blue, 2 = Green

void displayTimeScreen() {
  Serial.println("Displaying time screen");

  // Clear the screen and set a white background
  tft.fillScreen(TFT_WHITE);

  // Display "ECLSTAT 3.0" at the top of the screen
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLUE);
  tft.setCursor(53, 40);  // Adjust as needed
  tft.print("ECLStat 3.0");

  // Get the current time and display it
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  char timeStr[9];  // Format HH:MM:SS
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

  // Display the time at the center of the screen
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(68, 100);  // Adjust as needed
  tft.print(timeStr);

  // Draw the "Camera" button at the bottom of the screen
  tft.drawRect(70, 180, 100, 40, TFT_BLUE);  
  tft.setCursor(80, 195);                    
  tft.print("Camera");                       

  // Handle screen press event for camera screen
  if (display_is_pressed()) {
    Serial.println("Display pressed, switching to camera screen");
    onCameraScreen = true;  // Switch to camera screen
  }

  // If this is the first run, store the start time of the animation
  if (animationStartTime == 0) {
    animationStartTime = millis();  // Store the start time
  }

  // Calculate how much time has passed since animation start
  unsigned long elapsedTime = millis() - animationStartTime;

  // Wait for 1 second before starting the color-changing animation
  if (elapsedTime > 1000) {
    // Choose color based on elapsed time (red, blue, green every 1 second)
    if ((elapsedTime / 1000) % 3 == 0) {
      tft.setTextColor(TFT_RED);  // Red after 1 second
    } else if ((elapsedTime / 1000) % 3 == 1) {
      tft.setTextColor(TFT_BLUE);  // Blue after 2 seconds
    } else {
      tft.setTextColor(TFT_GREEN);  // Green after 3 seconds
    }

    // Set the text size to 1 (0.7 approximation of 2)
    tft.setTextSize(2);

    // Simulate bold by printing the text multiple times with slight offsets
    int x = 7.5;
    int y = 140;

    tft.setCursor(x, y);
    tft.print("Lighting the Future");

    // Offset by a small amount to make it bold
    tft.setCursor(x + 1, y);
    tft.print("Lighting the Future");

    tft.setCursor(x, y + 1);
    tft.print("Lighting the Future");

    tft.setCursor(x + 1, y + 1);
    tft.print("Lighting the Future");

    // Reset the animation loop every 3 seconds
    if (elapsedTime > 3000) {
      animationStartTime = millis();  // Reset animation start time
    }
  }
}




// Camera screen implementation
void displayCameraScreen() {
  if(sd_sign && camera_sign){
    Serial.println("Displaying camera screen");
    // Display camera feed
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Failed to get camera frame buffer");
      return;
    }

    // Check for screen touch to capture image
    if(display_is_pressed()){
      Serial.println("Display pressed, taking photo");
      takePhotoAndSave();
    }

    // Display image on screen
    tft.startWrite();
    tft.setAddrWindow(0, 0, 240, 240);
    tft.pushColors(fb->buf, fb->len);
    tft.endWrite();

    esp_camera_fb_return(fb);
    delay(10);
  } else {
    Serial.println("SD card or camera not initialized");
  }
}

// Simulate touch detection
bool display_is_pressed() {
  if (digitalRead(TOUCH_INT) != LOW) {
    delay(3);
    if (digitalRead(TOUCH_INT) != LOW) {
      return false;
    }
  }
  Serial.println("Display press detected");
  return true;
}

// Take photo and save to SD card
void takePhotoAndSave() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  char filename[32];
  sprintf(filename, "/image%d.jpg", imageCount++);

  uint8_t* out_buf = NULL;
  size_t out_len = 0;
  esp_err_t ret = frame2jpg(fb, 12, &out_buf, &out_len);
  if (ret == ESP_OK) {
    writeFile(SD, filename, out_buf, out_len);
    Serial.printf("Saved image to %s\n", filename);
    free(out_buf);
  } else {
    Serial.println("JPEG conversion failed");
  }

  esp_camera_fb_return(fb);
}

// Save image to SD card
void writeFile(fs::FS &fs, const char * path, uint8_t * data, size_t len) {
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.write(data, len) != len){
    Serial.println("Write failed");
  }
  file.close();
}

// Initialize Wi-Fi
void initWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { // 10 second timeout
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed");
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status());
  }
}

// Initialize time via NTP
void initTime() {
  Serial.println("Initializing time...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 5) { // 5 attempts, 2.5 second timeout
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (getLocalTime(&timeinfo)) {
    Serial.println("\nTime synchronized");
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    Serial.println(timeStringBuff);
  } else {
    Serial.println("\nFailed to obtain time");
    Serial.println("Retrying NTP server...");
    configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org"); // Try a different NTP server
    attempts = 0;
    while (!getLocalTime(&timeinfo) && attempts < 5) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if (getLocalTime(&timeinfo)) {
      Serial.println("\nTime synchronized with alternate NTP server");
      char timeStringBuff[50];
      strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
      Serial.println(timeStringBuff);
    } else {
      Serial.println("\nFailed to obtain time after retry");
    }
  }
}