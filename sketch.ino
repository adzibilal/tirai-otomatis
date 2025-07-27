#define BLYNK_TEMPLATE_ID           "TMPL6CUIHFCSn"
#define BLYNK_TEMPLATE_NAME         "Quickstart Template"
#define BLYNK_AUTH_TOKEN            "AfvlYtWG626crUPGLFr4fKsjmbkU3r1Y"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

char ssid[] = "Wokwi-GUEST";
char pass[] = "";

const int ldrPin = A0;
const int servoPin = 4;
const int ledPin = 2;

int ldrValue = 0;
int lightThreshold = 500;
bool curtainOpen = false;
bool autoMode = true;

Servo curtainServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;
BlynkTimer actionTimer;

// Untuk LCD flicker prevention
int prevLdrValue = -1;
bool prevCurtainOpen = !curtainOpen;
bool prevAutoMode = !autoMode;

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Tirai Otomatis");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  pinMode(ldrPin, INPUT);
  pinMode(ledPin, OUTPUT);
  curtainServo.attach(servoPin);
  curtainServo.write(0); // Closed

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(1000L, readSensors);
  timer.setInterval(1000L, updateDisplay);
  timer.setInterval(5000L, sendToBlynk);

  delay(2000);
}

void loop() {
  Blynk.run();
  timer.run();
  actionTimer.run();
  digitalWrite(ledPin, curtainOpen ? HIGH : LOW);
}

void readSensors() {
  ldrValue = analogRead(ldrPin);

  if (autoMode) {
    if (ldrValue > lightThreshold && !curtainOpen) {
      openCurtain();
    } else if (ldrValue <= lightThreshold && curtainOpen) {
      closeCurtain();
    }
  }

  Serial.printf("LDR: %d | Curtain: %s | Mode: %s\n", ldrValue, curtainOpen ? "OPEN" : "CLOSED", autoMode ? "AUTO" : "MANUAL");
}

void sendToBlynk() {
  Blynk.virtualWrite(V0, ldrValue);
  Blynk.virtualWrite(V1, curtainOpen ? 1 : 0);
  Blynk.virtualWrite(V2, autoMode ? 1 : 0);
  Blynk.virtualWrite(V3, lightThreshold);
}

void openCurtain() {
  Serial.println("Opening curtain...");
  curtainServo.write(90);
  curtainOpen = true;
  Blynk.logEvent("curtain_opened", "Curtain opened - bright light detected");
}

void closeCurtain() {
  Serial.println("Closing curtain...");
  curtainServo.write(0);
  curtainOpen = false;
  Blynk.logEvent("curtain_closed", "Curtain closed - low light detected");
}

void updateDisplay() {
  if (ldrValue != prevLdrValue || curtainOpen != prevCurtainOpen || autoMode != prevAutoMode) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LDR:");
    lcd.print(ldrValue);
    lcd.print(" T:");
    lcd.print(lightThreshold);

    lcd.setCursor(0, 1);
    lcd.print(curtainOpen ? "OPEN " : "TUTUP");
    lcd.print(" ");
    lcd.print(autoMode ? "AUTO" : "MANUAL");

    prevLdrValue = ldrValue;
    prevCurtainOpen = curtainOpen;
    prevAutoMode = autoMode;
  }
}

// V4: Manual Toggle Button
BLYNK_WRITE(V4) {
  int state = param.asInt();
  if (state == 1) {
    autoMode = false;
    if (curtainOpen) {
      closeCurtain();
    } else {
      openCurtain();
    }
  }
}

// V5: Mode Toggle (Auto/Manual)
BLYNK_WRITE(V5) {
  autoMode = param.asInt() == 1;
  Serial.println(autoMode ? "Switched to AUTO mode" : "Switched to MANUAL mode");
  Blynk.logEvent("mode_changed", autoMode ? "Auto mode enabled" : "Manual mode enabled");
}

// V6: Light Threshold Adjustment
BLYNK_WRITE(V6) {
  lightThreshold = param.asInt();
  Serial.print("Threshold set to: ");
  Serial.println(lightThreshold);
  Blynk.logEvent("threshold_changed", String("New threshold: ") + lightThreshold);
}

BLYNK_CONNECTED() {
  Serial.println("Connected to Blynk");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Blynk Connected!");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");
  delay(2000);

  Blynk.syncVirtual(V4, V5, V6);
  sendToBlynk();
}