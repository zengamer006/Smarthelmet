/*
  SMART HELMET: ESP32 + Hall/Reed + IR + MQ-3 -> Vercel API -> Firebase RTDB
  Board: ESP32 Dev Module (typical ESP32-WROOM-32)

  Pins carried forward from your prototype:
    Hall/Reed OUT GPIO25 (assumed LOW when detected)
    IR OUT         GPIO26 (assumed LOW when detected)
    Green LED      GPIO27 (use 220 ohm resistor)
    Red LED        GPIO14 (use 220 ohm resistor)
    Buzzer         GPIO13 (use suitable driver if buzzer current is high)
    Relay IN       GPIO33 (active-LOW relay module)
    MQ-3 AO        GPIO34 (ADC input-only pin)

  MQ-3 NOTE: Power the MQ-3 module as required by its manufacturer. Its AO output
  must NEVER exceed 3.3V at ESP32 GPIO34. If powered from 5V, use a correctly
  calculated voltage divider/level conditioning. Common MQ-3 modules need warm-up;
  raw ADC is not a calibrated alcohol concentration or breathalyzer result.
*/
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* WIFI_SSID = "osinet kekudha";
const char* WIFI_PASSWORD = "500rubakudusoldren";
const char* API_URL = "http://192.168.1.10:3000/api/telemetry";
const char* DEVICE_API_KEY = "SGNHelmet_2026_A7x9K2mP8vR4qL6wT3nB";

constexpr int HALL_PIN = 25;
constexpr int IR_PIN = 26;
constexpr int GREEN_LED = 27;
constexpr int RED_LED = 14;
constexpr int BUZZER_PIN = 13;
constexpr int RELAY_PIN = 33;
constexpr int MQ3_PIN = 34;

// Adjust these if your sensor modules report HIGH when detected.
constexpr int HALL_DETECTED_LEVEL = LOW;
constexpr int IR_DETECTED_LEVEL = LOW;
// Relay module is active-LOW: LOW=energized/ON, HIGH=OFF.
constexpr int RELAY_ON_LEVEL = LOW;
constexpr int RELAY_OFF_LEVEL = HIGH;
const unsigned long SEND_INTERVAL_MS = 3000;
unsigned long lastSend = 0;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 20000) {
    delay(400); Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi connected. IP: "); Serial.println(WiFi.localIP());
  } else Serial.println("Wi-Fi connection timed out; will retry later.");
}

void setup() {
  Serial.begin(115200);
  pinMode(HALL_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  analogReadResolution(12); // ESP32 ADC range typically 0-4095

  // Safe startup state: relay off. Active-LOW relay needs HIGH for OFF.
  digitalWrite(RELAY_PIN, RELAY_OFF_LEVEL);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  connectWiFi();
}

void sendTelemetry(bool hallDetected, bool irDetected, int mq3Raw,
                   float mq3Voltage, bool green, bool red,
                   bool buzzer, bool relay) {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (WiFi.status() != WL_CONNECTED) { Serial.println("No Wi-Fi; telemetry not sent."); return; }

  WiFiClientSecure client;
  // For a production deployment, validate the server certificate instead of setInsecure().
  // This keeps initial prototype setup simple but does not provide certificate verification.
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, API_URL)) { Serial.println("Could not start HTTPS request."); return; }
  http.setTimeout(6000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-device-key", DEVICE_API_KEY);

  String payload = "{";
  payload += "\"hallDetected\":" + String(hallDetected ? "true" : "false") + ",";
  payload += "\"irDetected\":" + String(irDetected ? "true" : "false") + ",";
  payload += "\"mq3Raw\":" + String(mq3Raw) + ",";
  payload += "\"mq3Voltage\":" + String(mq3Voltage, 3) + ",";
  payload += "\"greenLed\":" + String(green ? "true" : "false") + ",";
  payload += "\"redLed\":" + String(red ? "true" : "false") + ",";
  payload += "\"buzzerOn\":" + String(buzzer ? "true" : "false") + ",";
  payload += "\"relayOn\":" + String(relay ? "true" : "false") + ",";
  payload += "\"deviceTimestamp\":" + String(millis());
  payload += "}";

  int code = http.POST(payload);
  Serial.printf("Firebase telemetry via Vercel: HTTP %d\n", code);
  if (code > 0) Serial.println(http.getString());
  else Serial.println(http.errorToString(code));
  http.end();
}

void loop() {
  const bool hallDetected = digitalRead(HALL_PIN) == HALL_DETECTED_LEVEL;
  const bool irDetected = digitalRead(IR_PIN) == IR_DETECTED_LEVEL;
  const bool bothDetected = hallDetected && irDetected;
  const bool neitherDetected = !hallDetected && !irDetected;

  // Preserve confirmed behavior: neither -> buzzer; one -> red; both -> green + relay.
  const bool green = bothDetected;
  const bool red = (hallDetected != irDetected);
  const bool buzzer = neitherDetected;
  const bool relay = bothDetected;

  digitalWrite(GREEN_LED, green ? HIGH : LOW);
  digitalWrite(RED_LED, red ? HIGH : LOW);
  digitalWrite(BUZZER_PIN, buzzer ? HIGH : LOW);
  digitalWrite(RELAY_PIN, relay ? RELAY_ON_LEVEL : RELAY_OFF_LEVEL);

  const int mq3Raw = analogRead(MQ3_PIN);
  // Approximate ADC-pin voltage only; calibrate ADC if accurate voltage is needed.
  const float mq3Voltage = (mq3Raw / 4095.0f) * 3.3f;

  Serial.printf("Hall=%s | IR=%s | MQ3 raw=%d | ADC V=%.3f | Green=%d | Red=%d | Buzzer=%d | Relay=%s\n",
    hallDetected ? "DETECTED" : "NOT DETECTED",
    irDetected ? "DETECTED" : "NOT DETECTED", mq3Raw, mq3Voltage,
    green, red, buzzer, relay ? "ON" : "OFF");

  if (millis() - lastSend >= SEND_INTERVAL_MS) {
    lastSend = millis();
    sendTelemetry(hallDetected, irDetected, mq3Raw, mq3Voltage, green, red, buzzer, relay);
  }
  delay(100);
}
