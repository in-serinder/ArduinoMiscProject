#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <Servo.h>


#define ServoPWM D4

const char *WIFI_SSID = "2.4GHZ";
const char *WIFI_PASSWORD = "password";
const uint8_t WS_PORT = 81;
const uint16_t SERVO_ANGLE = 180;



WiFiClient wc;
WebSocketsServer ws = WebSocketsServer(WS_PORT);  //指定对象端口
Servo servo;

void Wifi_connect() {
  int count = 0;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connect to: \n\tSSID:" + String(WIFI_SSID) + "\n\tPassWord:" + String(WIFI_PASSWORD));
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
    count++;
  }
  Serial.println("\nConnect to " + String(WIFI_SSID) + " Successful\n");
  Serial.println("\nMAC: " + WiFi.macAddress() + "\nIP: " + WiFi.localIP().toString() + "\nUsing Time:" + count + "s");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  Serial.printf("Event Type : [%d]\n", type);
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("Ws Connected\n");
      break;
    case WStype_DISCONNECTED:
      Serial.println("Ws Disconnected\n");
      break;
    case WStype_ERROR:
      Serial.println("ws Error happen\n");
      break;
    case WStype_BIN:
      Serial.printf("Got binary length:[%d]\n", length);
      break;
    case WStype_TEXT:
      {
        Serial.printf("Got a Text Message [%s]\n", payload);
        // 直接使用文本
        // int Angle = (atoi(payload) == SERVO_ANGLE) ? SERVO_ANGLE : (atoi(payload) % SERVO_ANGLE);
        int Angle = (atoi((const char *)payload) % SERVO_ANGLE + 1) - 1;
        Serial.printf("Servo [%d]\n", Angle);
        servo.write(Angle);
        // servo.writeMicroseconds(2700);
      }
      break;
    case WStype_PING:
      break;
    case WStype_PONG:
      break;
    default: Serial.printf("Undefine in case type [%d]\n", type);
  }
}


void setup() {
  // put your setup code here, to run once:
  // init
  Serial.begin(9600);
  servo.attach(ServoPWM);
  Wifi_connect();
  ws.begin();
  ws.onEvent(webSocketEvent);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFi Connect Lost,Reconnecting");
    Wifi_connect();
  }
  ws.loop();
}