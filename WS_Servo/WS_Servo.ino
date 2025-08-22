#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <Servo.h>
#include <string.h>

#define ServoPWM 0

const char *WIFI_SSID = "2.4GHZ";
const char *WIFI_PASSWORD = "320724fuck";
const uint8_t WS_PORT = 81;
const uint16_t SERVO_ANGLE = 180;



WiFiClient wc;
WebSocketsServer ws = WebSocketsServer(WS_PORT);  //指定对象端口
Servo servo;

int Angle = 0;
String Servo_CurrentAnglestr = "";

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
      delay(800);
      for (uint8_t i = 0; i <= 6; i++) {
        Serial.printf("Send Txt [%s]", ws.sendTXT(i, Servo_CurrentAnglestr) ? "Successful" : "Failed");
      }
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
        Angle = (atoi((const char *)payload) % SERVO_ANGLE + 1) - 1;
        Serial.printf("Servo [%d]\n", Angle);
        servo.write(Angle);
        Servo_CurrentAnglestr = String(Angle);
        for (uint8_t i = 0; i <= 6; i++) {
          Serial.printf("Send Txt [%s]", ws.sendTXT(i, Servo_CurrentAnglestr) ? "Successful" : "Failed");
        }
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
  // 舵机
  servo.write(Angle);
  Servo_CurrentAnglestr = String(Angle);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFi Connect Lost,Reconnecting");
    Wifi_connect();
  }
  ws.loop();
}