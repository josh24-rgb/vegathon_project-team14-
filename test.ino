#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <Adafruit_BMP085.h>

// Pin Definitions
#define PIR_PIN 0      // IR Sensor (HW201)
#define TRIG_PIN 1     // Ultrasonic TRIG
#define ECHO_PIN 2     // Ultrasonic ECHO
#define BUZZER 3       // Piezo Buzzer
#define LED_RED 4      // Red LED
#define LED_YELLOW 5   // Yellow LED
#define LED_GREEN 6    // Green LED

// OLED Setup (SH1106 - 128x64)
#define OLED_RESET -1
Adafruit_SH1106 display(OLED_RESET);

// Sensor Objects
Adafruit_BMP085 bmp;

// Timing Variables
unsigned long lastDisplayUpdate = 0;
unsigned long lastWebOutput = 0;
unsigned long systemStartTime = 0;
const unsigned long displayInterval = 500;    // 0.5 seconds
const unsigned long webInterval = 1000;       // 1 second (for web dashboard)

// Configuration Thresholds
struct Config {
  float tempDanger = 38.0;
  int distCritical = 15;
  int riskAlarmLevel = 70;
  unsigned long distanceTimeout = 30000; // 30ms timeout
} config;

// System Status Flags
struct SystemStatus {
  bool oledOK = false;
  bool bmpOK = false;
} sysStatus;

void setup() {
  Serial.begin(115200);
  
  delay(1000);
  Serial.println("\n\n=== SENTINEL SECURITY SYSTEM ===");
  Serial.println("Initializing...");
  
  // Configure Pins
  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  
  // Initial safe state
  digitalWrite(BUZZER, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN, HIGH); // Green on by default
  
  // Initialize I2C
  Wire.begin();
  delay(100);
  
  // Initialize OLED Display (SH1106)
  Serial.print("SH1106 Display... ");
  if(display.begin(SH1106_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OK");
    sysStatus.oledOK = true;
    
    // Splash Screen
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(20, 25);
    display.println("SENTINEL ACTIVE");
    display.display();
  } else {
    Serial.println("FAILED!");
  }
  
  // Initialize BMP085 Temperature/Pressure Sensor
  Serial.print("BMP085 Sensor... ");
  if(bmp.begin()) {
    Serial.println("OK");
    sysStatus.bmpOK = true;
  } else {
    Serial.println("FAILED!");
  }
  
  // Print System Status
  Serial.println("\n--- System Status ---");
  Serial.print("OLED: "); Serial.println(sysStatus.oledOK ? "OK" : "FAIL");
  Serial.print("BMP085: "); Serial.println(sysStatus.bmpOK ? "OK" : "FAIL");
  Serial.println("---------------------\n");
  Serial.println("WEB_READY");
  
  systemStartTime = millis();
  delay(2000);
}

long getDistance() {
  // Send ultrasonic pulse
  digitalWrite(TRIG_PIN, LOW); 
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); 
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Measure echo with timeout
  long duration = pulseIn(ECHO_PIN, HIGH, config.distanceTimeout);
  
  if (duration == 0) {
    return 999; // Out of range
  }
  
  // Calculate distance in cm
  long distance = duration * 0.034 / 2;
  
  // Sanity check (2-400cm range)
  if (distance < 2 || distance > 400) {
    return 999;
  }
  
  return distance;
}

// Get uptime as timestamp
String getUptime() {
  unsigned long uptimeSeconds = (millis() - systemStartTime) / 1000;
  unsigned long hours = uptimeSeconds / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;
  unsigned long seconds = uptimeSeconds % 60;
  
  char timeStr[9];
  sprintf(timeStr, "%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(timeStr);
}

void updateDisplay(String uptime, float temp, int motion, long dist, int risk_score) {
  if (!sysStatus.oledOK) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // Uptime Display
  display.setCursor(0, 0);
  display.print("UPTIME: ");
  display.print(uptime);
  
  // Separator line
  display.drawLine(0, 10, 128, 10, WHITE);
  
  // Sensor Readings
  display.setCursor(0, 15);
  display.print("TEMP: ");
  if (sysStatus.bmpOK) {
    display.print(temp, 1);
    display.println(" C");
  } else {
    display.println("-- C");
  }
  
  display.print("DIST: ");
  if (dist == 999) {
    display.println("--- CM");
  } else {
    display.print(dist);
    display.println(" CM");
  }
  
  display.print("PIR:  ");
  display.println(motion ? "DETECTED" : "CLEAR");
  
  // Risk Score (Large)
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.print("RISK:");
  if (risk_score >= config.riskAlarmLevel) {
    display.setTextColor(BLACK, WHITE); // Inverted
  }
  display.print(risk_score);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  
  display.display();
}

// Send JSON data to web interface
void sendWebData(String uptime, float temp, int motion, long dist, int risk_score, bool alarm) {
  Serial.print("{");
  
  // Uptime
  Serial.print("\"timestamp\":\"");
  Serial.print(uptime);
  Serial.print("\",");
  
  // Temperature
  Serial.print("\"temperature\":");
  Serial.print(temp, 1);
  Serial.print(",");
  
  // Distance
  Serial.print("\"distance\":");
  if (dist == 999) {
    Serial.print("null");
  } else {
    Serial.print(dist);
  }
  Serial.print(",");
  
  // Motion
  Serial.print("\"motion\":");
  Serial.print(motion ? "true" : "false");
  Serial.print(",");
  
  // Risk Score
  Serial.print("\"riskScore\":");
  Serial.print(risk_score);
  Serial.print(",");
  
  // Alarm Status
  Serial.print("\"alarm\":");
  Serial.print(alarm ? "true" : "false");
  Serial.print(",");
  
  // System Status
  Serial.print("\"system\":{");
  Serial.print("\"oled\":");
  Serial.print(sysStatus.oledOK ? "true" : "false");
  Serial.print(",\"bmp085\":");
  Serial.print(sysStatus.bmpOK ? "true" : "false");
  Serial.print("}");
  
  Serial.println("}");
}

void setLEDs(int risk_score) {
  // Three-tier LED system
  if (risk_score >= config.riskAlarmLevel) {
    // HIGH RISK - Red only
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
  } else if (risk_score >= 40) {
    // MEDIUM RISK - Yellow only
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_GREEN, LOW);
  } else {
    // LOW RISK - Green only
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, HIGH);
  }
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Get uptime
  String uptime = getUptime();
  
  // Read Temperature
  float temp = 0.0;
  if (sysStatus.bmpOK) {
    temp = bmp.readTemperature();
    // Sanity check
    if (temp < -40 || temp > 85) {
      temp = 0.0;
    }
  }
  
  // Read IR Motion Sensor
  int motion = digitalRead(PIR_PIN);
  
  // Read Ultrasonic Distance
  long dist = getDistance();
  
  // Calculate Risk Score
  int risk_score = 0;
  
  if (sysStatus.bmpOK && temp > config.tempDanger) {
    risk_score += 30;    // High temperature
  }
  
  if (motion == HIGH) {
    risk_score += 40;    // Motion detected
  }
  
  if (dist != 999 && dist < config.distCritical) {
    risk_score += 50;    // Object too close
  }
  
  // Determine Alarm State
  bool alarm = (risk_score >= config.riskAlarmLevel);
  
  // Control Outputs
  setLEDs(risk_score);
  
  // Buzzer - pulse when alarm
  if (alarm) {
    digitalWrite(BUZZER, (currentMillis / 200) % 2);
  } else {
    digitalWrite(BUZZER, LOW);
  }
  
  // Update OLED Display
  if (currentMillis - lastDisplayUpdate >= displayInterval) {
    updateDisplay(uptime, temp, motion, dist, risk_score);
    lastDisplayUpdate = currentMillis;
  }
  
  // Send JSON data for web interface
  if (currentMillis - lastWebOutput >= webInterval) {
    sendWebData(uptime, temp, motion, dist, risk_score, alarm);
    lastWebOutput = currentMillis;
  }
  
  delay(50);
}

