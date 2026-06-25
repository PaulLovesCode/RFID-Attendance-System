#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include "esp_wifi.h"

// WIFI CONFIG
const char* ssid     = "testdev";
const char* password = "12345678";
const String SCRIPT_URL = "https://script.google.com/macros/s/AKfycbyelIMImiCMYjayFv0N5-UhS--jyVGfuQXjFJRLwULB1y1esebBaf8itNZTydWw04pY/exec";

// PIN CONFIG
#define SS_PIN     10
#define RST_PIN    4
#define BUZZER_PIN 21
#define GREEN_LED  16
#define RED_LED    17

// SPI PIN CONFIG
#define SCK_PIN    12
#define MOSI_PIN   11
#define MISO_PIN   13

// I2C PIN CONFIG
#define SDA_PIN    8
#define SCL_PIN    9

// STAFF DATA
const int STAFF_COUNT = 4;
String staffNames[] = {"ATUL", "SAYANTAN", "SUDIP", "VENU GOPAL IYER"};
String staffCards[] = {"D5A0D605", "8E370C07", "46D99804", "C3722407"};
bool   isInOffice[] = {false, false, false, false};

MFRC522            rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C  lcd(0x27, 16, 2);
WiFiClientSecure   secureClient;

// NTP Config
const char* ntpServer      = "pool.ntp.org";
const char* ntpServer2     = "time.google.com";
const char* ntpServer3     = "time.cloudflare.com";
const long  gmtOffset      = 19800; // IST UTC+5:30
const int   daylightOffset = 0;

// Non-blocking timing
unsigned long cardDisplayUntil = 0;
bool          displayingResult = false;
bool          displayingError  = false;

// ── WiFi Connect ──────────────────────────────────────────────────────────────
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(1000);

  esp_wifi_set_ps(WIFI_PS_NONE);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.setSleep(false);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  lcd.clear();
  lcd.print("Connecting WiFi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected: " + WiFi.localIP().toString());
    return true;
  } else {
    Serial.println("\nWiFi Failed! Status: " + String(WiFi.status()));
    return false;
  }
}

// ── NTP Sync ─────────────────────────────────────────────────────────────────
bool syncNTP() {
  configTime(gmtOffset, daylightOffset, ntpServer, ntpServer2, ntpServer3);
  Serial.print("Syncing NTP time");

  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (getLocalTime(&timeinfo)) {
    char buf[30];
    strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", &timeinfo);
    Serial.println("\nNTP Synced: " + String(buf));
    return true;
  } else {
    Serial.println("\nNTP Sync Failed!");
    return false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED,  OUTPUT);
  pinMode(RED_LED,    OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Buzzer OFF at start
  digitalWrite(GREEN_LED,  LOW);
  digitalWrite(RED_LED,    LOW);

  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();

  // Connect WiFi
  bool wifiOK = connectWiFi();
  secureClient.setInsecure();

  lcd.clear();
  if (wifiOK) {
    lcd.print("WiFi Connected!");
    delay(600);

    // Sync NTP
    lcd.clear();
    lcd.print("Syncing Time...");
    bool ntpOK = syncNTP();

    lcd.clear();
    if (ntpOK) {
      struct tm timeinfo;
      getLocalTime(&timeinfo);
      char timeBuf[10];
      strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &timeinfo);
      lcd.print("Time Synced!");
      lcd.setCursor(0, 1);
      lcd.print(String("IST: ") + timeBuf);
    } else {
      lcd.print("Time Sync Fail");
      lcd.setCursor(0, 1);
      lcd.print("Check internet");
    }
    delay(1500);

  } else {
    lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Check WPA2/Band");
    delay(2000);
  }

  // Startup beep — confirms buzzer works
  successBeep();

  showIdle();
}

void loop() {
  unsigned long now = millis();

  if (displayingResult && now >= cardDisplayUntil) {
    displayingResult = false;
    displayingError  = false;
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED,   LOW);
    showIdle();
  }

  if (displayingResult || displayingError) return;

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial())   return;

  // Build card ID
  String cardID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) cardID += "0";
    cardID += String(rfid.uid.uidByte[i], HEX);
  }
  cardID.toUpperCase();

  // Find employee
  int empID = -1;
  for (int i = 0; i < STAFF_COUNT; i++) {
    if (staffCards[i] == cardID) { empID = i; break; }
  }

  if (empID == -1) {
    lcd.clear();
    lcd.print("UNKNOWN CARD!");
    lcd.setCursor(0, 1);
    lcd.print(cardID);
    errorBeep();
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    displayingError  = true;
    displayingResult = true;
    cardDisplayUntil = millis() + 2000;
    return;
  }

  String name   = staffNames[empID];
  String status = isInOffice[empID] ? "Check OUT" : "Check IN";
  isInOffice[empID] = !isInOffice[empID];

  if (status == "Check IN") {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED,   LOW);
  } else {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED,   HIGH);
  }

  String timestamp = getDateTime();
  String date      = timestamp.substring(0, 10);
  String time      = timestamp.substring(11);

  lcd.clear();
  lcd.setCursor(0, 0);
  if (name.length() > 16) lcd.print(name.substring(0, 16));
  else lcd.print(name);
  lcd.setCursor(0, 1);
  lcd.print(status + " " + time);
  successBeep();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  displayingResult = true;
  cardDisplayUntil = millis() + 2000;

  sendToGoogleSheets(name, cardID, status, date, time);
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void showIdle() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan Employee");
  lcd.setCursor(0, 1);
  lcd.print("Card...");
}

String getDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time");
    return "00-00-0000 00:00:00";
  }
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

void sendToGoogleSheets(String name, String card,
                        String status, String date, String time) {
  Serial.println("\n==============================");
  Serial.println("Name: "   + name   + "  Card: " + card);
  Serial.println("Status: " + status + "  Date: " + date + "  Time: " + time);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected — skipping upload");
    return;
  }

  HTTPClient https;
  https.begin(secureClient, SCRIPT_URL);
  https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  https.setTimeout(6000);

  String postData = "name="    + name   +
                    "&card="   + card   +
                    "&status=" + status +
                    "&date="   + date   +
                    "&time="   + time;

  int httpCode = https.POST(postData);
  Serial.println("HTTP Code: " + String(httpCode));

  https.end();
}

// ── Buzzer (Active Buzzer — HIGH/LOW only) ────────────────────────────────────

void successBeep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
}

void errorBeep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_PIN, LOW);
}
