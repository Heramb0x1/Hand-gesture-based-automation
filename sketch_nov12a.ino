#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <MPU6050.h>

const char* ssid = "nothing";
const char* password = "8459452881";

WebSocketsServer webSocket = WebSocketsServer(81);
MPU6050 mpuIndex;
MPU6050 mpuThumb;

const int MPU_ADDR_INDEX = 0x68;  // AD0 low
const int MPU_ADDR_THUMB = 0x69;  // AD0 high

const int PIN_AD0_INDEX = 12;  // D6
const int PIN_AD0_THUMB = 13;  // D7

const int FLASH_BUTTON_PIN = 0;  // GPIO0 is typically the flash button on ESP8266

bool automationEnabled = false;
bool tempLock = true;  // Start with lock enabled
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(PIN_AD0_INDEX, OUTPUT);
  pinMode(PIN_AD0_THUMB, OUTPUT);
  pinMode(FLASH_BUTTON_PIN, INPUT_PULLUP);

  initMPU(mpuIndex, MPU_ADDR_INDEX, PIN_AD0_INDEX);
  initMPU(mpuThumb, MPU_ADDR_THUMB, PIN_AD0_THUMB);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Print IP address prominently
  Serial.println("\n\n");
  Serial.println("==========================");
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("==========================");
  Serial.println("Please copy this IP address to your notepad.");
  Serial.println("Then press the flash button on your ESP8266");
  Serial.println("to enable gesture detection.");
  Serial.println("==========================\n\n");

  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
  checkFlashButton();

  if (!tempLock) {
    checkGestures();
  }

  delay(10);
}

void initMPU(MPU6050 &mpu, int addr, int pinAD0) {
  digitalWrite(pinAD0, addr == MPU_ADDR_THUMB ? HIGH : LOW);
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
  }
}

void checkGestures() {
  int16_t ax1, ay1, az1, ax2, ay2, az2;

  mpuIndex.getAcceleration(&ax1, &ay1, &az1);
  mpuThumb.getAcceleration(&ax2, &ay2, &az2);

  // Zoom in: Both fingers stretched out
  if (ay1 > 8000 && ay2 > 8000) {
    Serial.println("Zoom In");
    webSocket.broadcastTXT("ZOOM_IN");
  } 
  // Zoom out: Both fingers curled in
  else if (ay1 < -8000 && ay2 < -8000) {
    Serial.println("Zoom Out");
    webSocket.broadcastTXT("ZOOM_OUT");
  }
  // You can add more gestures here if needed
}

void checkFlashButton() {
  int reading = digitalRead(FLASH_BUTTON_PIN);

  if (reading == LOW) { // Button is pressed (active low)
    if ((millis() - lastDebounceTime) > debounceDelay) {
      tempLock = !tempLock;
      Serial.println(tempLock ? "Gesture Detection Disabled" : "Gesture Detection Enabled");
      webSocket.broadcastTXT(tempLock ? "LOCK_ON" : "LOCK_OFF");
    }
    lastDebounceTime = millis();
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      // Handle incoming messages if needed
      break;
  }
}