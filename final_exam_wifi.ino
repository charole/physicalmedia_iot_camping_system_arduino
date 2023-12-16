#include <WiFi.h>
#include <WebServer.h>
#include <ESP32_Servo.h>
#include <Adafruit_NeoPixel.h>
#include <DFRobot_DHT11.h>
#include <ArduinoJson.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define NEO_PIXEL_PIN 2      // NeoPixel이 연결된 ESP32의 핀 번호
#define GAS_PIN 34           // 일산화탄소 센서가 연결된 ESP32의 핀 번호
#define TEMP_PIN 32          // 온습도 센서가 연결된 ESP32의 핀 번호
#define BUZZER_PIN 5         // 부저 센서가 연결된 ESP32의 핀 번호
#define SERVO_WINDOW_PIN 27  // 창문 서브 모터가 연결된 ESP32의 핀 번호
#define SERVO_DOOR_PIN 26    // 문 서브 모터가 연결된 ESP32의 핀 번호
#define NUMPIXELS 8          // NeoPixel 스트립에 있는 LED의 개수
#define PORT 80

const char* ssid = "";
const char* password = "";
bool isTone = false;
int doorAngle = 0;
int windowAngle = 0;

Servo servoDoor;
Servo servoWindow;
WebServer server(PORT);
Adafruit_NeoPixel pixels(NUMPIXELS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);
DFRobot_DHT11 dht11;

uint32_t parseColor(String hexColor) {
  String rgb = hexColor.substring(1);  // '#' 제거
  long number = strtol(rgb.c_str(), NULL, 16);
  int r = number >> 16;
  int g = (number >> 8) & 0xFF;
  int b = number & 0xFF;

  return pixels.Color(r, g, b);
}

void aroundTone() {
  tone(BUZZER_PIN, 262, 500);
  delay(500);
  noTone(BUZZER_PIN);
  delay(5000);
};

void checkStatusNetworks() {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    // Print SSID and RSSI for each network found
    Serial.println(WiFi.SSID(i));
    // Serial.println(WiFi.RSSI(i));
  }
};

void handleLedOn() {
  server.send(200, "text/plain", "LED is ON");
  String color = server.arg("color");
  Serial.print("color? ");
  Serial.println(color);
  uint32_t rgbColor = parseColor(color);
  Serial.println(rgbColor);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, rgbColor);
    pixels.show();
  }
}

void handleLedOff() {
  server.send(200, "text/plain", "LED is OFF");
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    pixels.show();
  }
  Serial.println("LED OFF.");
}

void handleGas() {
  int gasValue = analogRead(GAS_PIN);
  Serial.print("gas? ");
  Serial.println(gasValue);
  server.send(200, "text/plain", String(gasValue));
};

void handleTemp() {
  dht11.read(TEMP_PIN);
  DynamicJsonDocument doc(1024);
  doc["temp"] = dht11.temperature;
  doc["humi"] = dht11.humidity;
  Serial.print("temperature? ");
  Serial.println(dht11.temperature);
  Serial.print("humidity? ");
  Serial.println(dht11.humidity);
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
};

void handleBuzzerOn() {
  isTone = true;
  server.send(200, "text/plain", "buzzer on success");
};

void handleBuzzerOff() {
  isTone = false;
  server.send(200, "text/plain", "buzzer off success");
};

void handleWindowOpen() {
  servoWindow.write(180);
  server.send(200, "text/plain", "window open success");
}

void handleWindowClose() {
  servoWindow.write(0);
  server.send(200, "text/plain", "window close success");
}

void handleDoorOpen() {
  servoDoor.write(180);
  server.send(200, "text/plain", "door open success");
}

void handleDoorClose() {
  servoDoor.write(0);
  server.send(200, "text/plain", "door close success");
}

void wifiSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting To WiFi....");
  }

  Serial.println("Connected to WiFi.");
  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());
};

void serverSetup() {
  server.on("/led/on", HTTP_GET, handleLedOn);
  server.on("/led/off", HTTP_GET, handleLedOff);
  server.on("/gas", HTTP_GET, handleGas);
  server.on("/temp", HTTP_GET, handleTemp);
  server.on("/buzzer/on", HTTP_GET, handleBuzzerOn);
  server.on("/buzzer/off", HTTP_GET, handleBuzzerOff);
  server.on("/window/open", HTTP_GET, handleWindowOpen);
  server.on("/window/close", HTTP_GET, handleWindowClose);
  server.on("/door/open", HTTP_GET, handleDoorOpen);
  server.on("/door/close", HTTP_GET, handleDoorClose);

  server.begin();
  server.enableCORS(true);
};

void setup() {
  Serial.begin(115200);
  pixels.begin();
  pinMode(GAS_PIN, INPUT);
  pinMode(TEMP_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  servoWindow.attach(SERVO_WINDOW_PIN);
  servoDoor.attach(SERVO_DOOR_PIN);
  wifiSetup();
  serverSetup();
}

void loop() {
  // checkStatusNetworks();
  server.handleClient();
  if (isTone) {
    aroundTone();
  }
}