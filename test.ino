#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <Adafruit_BMP085.h>

// --- PIN DEFINITIONS ---
#define PIR_PIN 0      
#define TRIG_PIN 1     
#define ECHO_PIN 2     
#define BUZZER 3       
#define LED_RED 4      
#define LED_YELLOW 5   
#define LED_GREEN 6    

#define OLED_RESET -1
Adafruit_SH1106 display(OLED_RESET);
Adafruit_BMP085 bmp;

// --- SYSTEM STATUS ---
struct SystemStatus {
  bool oledOK = false;
  bool bmpOK = false;
} sysStatus;

void setup() {
  Serial.begin(115200);
  
  // Initialize Pins
  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  // Initialize I2C Hardware
  Wire.begin();
  delay(100);

  // FIX: Call display.begin() separately (not in an 'if')
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  sysStatus.oledOK = true; // Set manually since library is void
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(20, 25);
  display.println("SENTINEL ACTIVE");
  display.display();

  if (bmp.begin()) {
    sysStatus.bmpOK = true;
  }
}
