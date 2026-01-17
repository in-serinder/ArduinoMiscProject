#include <ESP8266WiFi.h>
#include <string.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTpin 2
#define LED 2

#define DHTTYPE DHT22

const char *WIFI_SSID = "2.4GHZ";
const char *WIFI_PASSWORD = "pwd";

const char *MQTT_SERVER_IP = "broker";
const int MQTT_PORT = 1883;
const char *MQTT_USER = "Relay";
const char *MQTT_PASSWORD = "password";

const char *MQTT_TOPIC = "Temperature/Node1";

WiFiClient wc;
PubSubClient pc(wc);
DHT dht(DHTpin, DHTTYPE);

void MQTTCallBack(char *topic, byte *payload, unsigned int len) {
  digitalWrite(LED, 1);
  Serial.println("data from - " + String(topic) + " ");
  String str = "";
  for (unsigned int i = 0; i < len; i++) {
    Serial.print((char)payload[i]);
    str += String((char)payload[i]);
  }
  Serial.println();
  Serial.println("------------------------");

  if (str[0] == '~' && (!str.isEmpty())) {
    str = str.substring(1);
  }

  Serial.println("Massage: " + str);
  // Main str -> Command
  if (str == "GET") {
    float temp =dht.readTemperature();
    while(isnan(temp)) {temp =dht.readTemperature();delay(2000);};

    pc.publish(MQTT_TOPIC, (">T" + String(temp) + " H" + String(dht.readHumidity())).c_str());
    Serial.println("Command--" + str);
  }

  if (str == "GET_TEMP") {
    float temp =dht.readTemperature();
    while(isnan(temp)) {temp =dht.readTemperature();delay(2000);};
    pc.publish(MQTT_TOPIC, (">T" + String(temp)).c_str());
    Serial.println("Command--" + str);
  }

  if (str == "GET_HUM") {
    float hum =dht.readHumidity();
    while(isnan(hum)) {hum =dht.readHumidity();delay(2000);};
    pc.publish(MQTT_TOPIC, (">H" + String(hum)).c_str());
    Serial.println("Command--" + str);
  }

  digitalWrite(LED, 0);
}

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
  MQTT_connect();
  Serial.println("\nConnect to " + String(WIFI_SSID) + " Successful\n");
  Serial.println("\nMAC: " + WiFi.macAddress() + "\nIP: " + WiFi.localIP().toString() + "\nUsing Time:" + count + "s");
}

uint8_t MQTT_connect() {
  String data = "Hello DHT_NODE2 " + WiFi.macAddress() + " Started";

  if (WiFi.status() != WL_CONNECTED)
    return -1;
  if (!pc.connect(WiFi.macAddress().c_str(), MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("MQTT Readly");
    return -1;
  }
  pc.subscribe(MQTT_TOPIC);
  Serial.println("Topic: " + String(MQTT_TOPIC));
  pc.publish(MQTT_TOPIC, data.c_str());
  pc.setCallback(MQTTCallBack);
  Serial.println("Connect MQTT Server " + String(MQTT_SERVER_IP) + " Success");
  return 1;
}

void setup() {
  // put your setup code here, to run once:
  pinMode(DHTpin, OUTPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);

  pc.setServer(MQTT_SERVER_IP, MQTT_PORT);
  Wifi_connect();
  pc.connect(WiFi.macAddress().c_str());
}

void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi is break reconnect ... ");
    Wifi_connect();
    // MQTT_connect();
  } else {
    if (!pc.connected()) {
      MQTT_connect();
      // String topic=WiFi.macAddress()+" -Relay_Receive";
      // pc.publish(topic.c_str(),"Hello Guys");
    } else {
      pc.loop();
    }
  }
  delay(5000);
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (isnan(temp) || isnan(hum)) {
    return;
  }
  pc.publish(MQTT_TOPIC, (">T" + String((temp < 100 && temp > -50) ? temp : 0.0)).c_str());
  pc.publish(MQTT_TOPIC, (">H" + String((hum < 100 && hum > 0) ? hum : 0.0)).c_str());
}
