#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <string.h>
#include <EEPROM.h>
#include <PubSubClient.h>

#define Relay 0
#define LED 2

const char *WIFI_SSID = "ssid";
const char *WIFI_PASSWORD = "password";

const char *MQTT_SERVER_IP = "Server IP";
const int MQTT_PORT = 1883;
const char *MQTT_USER = "Relay";
const char *MQTT_PASSWORD = "password";

const char *MQTT_TOPIC = "Relay/Server_Master";

WiFiClient wc;
PubSubClient pc(wc);
void Relay_NO_swich(int sw);

void MQTTCallBack(char *topic, byte *payload, unsigned int len)
{
  Serial.println("data from - " + String(topic) + " ");
  String str = "";
  for (unsigned int i = 0; i < len; i++)
  {
    Serial.print((char)payload[i]);
    str += String((char)payload[i]);
  }
  Serial.println();
  Serial.println("------------------------");

  if (str[0] == '~' && (!str.isEmpty()))
  {
    str = str.substring(1);
  }

  Serial.println("Massage: " + str);

  // Main str -> Command
  if (str == "status")
  {
    RelayStatus();
    Serial.println("Command--" + str);
  }

  if (str == "NO")
  {
    Relay_NO_swich(1);
    Serial.println("Command--" + str);
  }

  if (str == "NC")
  {
    Relay_NO_swich(0);
    Serial.println("Command--" + str);
  }

  if (str == "SW")
  {
    Relay_NO_swich(1);
    delay(800);
    Relay_NO_swich(0);
    Serial.println("Command--" + str);
  }
}

void Wifi_connect()
{
  int count = 0;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connect to: \n\tSSID:" + String(WIFI_SSID) + "\n\tPassWord:" + String(WIFI_PASSWORD));
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
    count++;
  }
  MQTT_connect();
  Serial.println("\nConnect to " + String(WIFI_SSID) + " Successful\n");
  Serial.println("\nMAC: " + WiFi.macAddress() + "\nIP: " + WiFi.localIP().toString() + "\nUsing Time:" + count + "s");
  LED_Flash(20);
}

uint8_t MQTT_connect()
{
  String data = "Hello Relay " + WiFi.macAddress() + " Started";

  if (WiFi.status() != WL_CONNECTED)
    return -1;
  if (!pc.connect(WiFi.macAddress().c_str(), MQTT_USER, MQTT_PASSWORD))
  {
    Serial.println("MQTT Readly");
    return -1;
  }
  // String topic=WiFi.macAddress()+" -Relay_Receive";
  pc.subscribe(MQTT_TOPIC);
  Serial.println("Topic: " + String(MQTT_TOPIC));
  pc.publish(MQTT_TOPIC, data.c_str());
  pc.setCallback(MQTTCallBack);
  Serial.println("Connect MQTT Server " + String(MQTT_SERVER_IP) + " Success");
  return 1;
}

void RelayStatus()
{
  String status = "Power: " + String(!digitalRead(Relay) ? "NO" : "NC") + "  Last: " + (ReadData() ? "NO" : "NC") + " Status Code: " + ReadData();
  LED_Flash(30);
  Serial.println(status);
  pc.publish(MQTT_TOPIC, status.c_str());
}

// NO 1 NC 0
// Normally Open&Normally Close
void Relay_NO_swich(int sw)
{
  Serial.println("sw Submit: " + String(sw));
  digitalWrite(Relay, !sw);
  digitalWrite(LED, !sw);
  String state = (sw == 1) ? "NO" : "NC";
  pc.publish(MQTT_TOPIC, ("Relay- " + state).c_str());
  WriteData(sw);
}

void WriteData(int sw)
{
  EEPROM.write(0, sw);
  EEPROM.commit();
  Serial.println("Write: " + String(sw) + " Read: " + String(EEPROM.read(0)));
}

uint8_t ReadData()
{
  return EEPROM.read(0);
}

void LED_Flash(int hz)
{
  uint8_t Ti = (hz / 60) * 1000;
  for (uint8_t i = 0; i <= hz; i++)
  {
    digitalWrite(LED, 1);
    delay(Ti);
    digitalWrite(LED, 0);
    delay(Ti);
  }
}

void initialization()
{
  pinMode(Relay, OUTPUT);
  pinMode(LED, OUTPUT);

  int initialState = ReadData();
  digitalWrite(Relay, !initialState);
  digitalWrite(LED, !initialState);
  Serial.begin(9600);
}

////////////////////////////////////////////////////////////
void setup()
{
  EEPROM.begin(1024);
  initialization();
  // MQTT
  pc.setServer(MQTT_SERVER_IP, MQTT_PORT);

  //
  Wifi_connect();
  pc.connect(WiFi.macAddress().c_str());
}

void loop()
{

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println();
    Serial.println("WiFi is break reconnect ... ");
    Wifi_connect();
    // MQTT_connect();
  }
  else
  {
    if (!pc.connected())
    {
      MQTT_connect();
      // String topic=WiFi.macAddress()+" -Relay_Receive";
      // pc.publish(topic.c_str(),"Hello Guys");
    }
    else
    {
      pc.loop();
    }
  }
  digitalWrite(LED, !ReadData());
  delay(2000);
}

///////////////////////////////////////////////////////////