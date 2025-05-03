#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <string.h>
#include<EEPROM.h>
#include <PubSubClient.h>

#define Relay 0
#define LED 2

const char* WIFI_SSID="Arch-Chan";
const char* WIFI_PASSWORD="320724fuck";

const char* MQTT_SERVER_IP="8.130.191.142";
const int MQTT_PORT=1883;
const char* MQTT_USER="Relay";
const char* MQTT_PASSWORD="320724fuck";

const char* MQTT_TOPIC="Relay/Server_Master";

WiFiClient wc;
PubSubClient pc(wc);






void MQTTCallBack(char* topic , byte* payload,unsigned int len){
  Serial.println("data from - "+String(topic) + " ");
  String str="";
  for(unsigned int i=0;i<len;i++){
    Serial.print((char)payload[i]);
    str+=String((char)payload[i]);
  }
  Serial.println();
  Serial.println("------------------------");

  if(str[0]=='~'&&(!str.isEmpty())){
    str = str.substring(1);
  }


  Serial.println("Massage: "+str);

  // Main str -> Command
  if(str == "status"){
    RelayStatus();
    Serial.println("Command--"+str);
  }

  if (str == "NO") {
    Relay_NO_swich(true);
    Serial.println("Command--"+str);
  }

  if (str == "NC") {
    Relay_NO_swich(false);
    Serial.println("Command--"+str);
  }

}


void Wifi_connect(){
  int count=0;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  Serial.println("Connect to: \n\tSSID:"+String(WIFI_SSID)+"\n\tPassWord:"+String(WIFI_PASSWORD));
  while (WiFi.status()!=WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
    count++;
  }
  MQTT_connect();
  Serial.println("\nConnect to "+String(WIFI_SSID)+" Successful\n");
  Serial.println("\nMAC: " + WiFi.macAddress() + "\nIP: " + WiFi.localIP().toString()+"\nUsing Time:"+count+"s");
  LED_Flash(20);
}


uint8_t MQTT_connect(){
  String data = "Hello Relay " + WiFi.macAddress()+" Started";


  if(WiFi.status()!=WL_CONNECTED) return -1;
  if(!pc.connect(WiFi.macAddress().c_str(),MQTT_USER,MQTT_PASSWORD)){
    Serial.println("MQTT Readly");
    return -1;
  }
  // String topic=WiFi.macAddress()+" -Relay_Receive";
  pc.subscribe(MQTT_TOPIC);
  Serial.println("Topic: "+ String(MQTT_TOPIC));
  pc.publish(MQTT_TOPIC,data.c_str());
  pc.setCallback(MQTTCallBack);
  Serial.println("Connect MQTT Server "+ String(MQTT_SERVER_IP)+" Success");
  return 1;
}




void RelayStatus(){
  String status = "Power: " + String(digitalRead(Relay)? "NC" : "NO") + " /n/t Last:" + ReadData();
  Serial.println(status);
  pc.publish(MQTT_TOPIC,status.c_str());
}


//NO 0 NC 1
//Normally Open&Normally Close
void Relay_NO_swich(bool sw){
  digitalWrite(Relay, sw);
  WriteData(sw? 1 : 0);
}

void WriteData(int sw){
  EEPROM.write(0,sw);
  EEPROM.commit();
}

uint8_t ReadData(){
  return EEPROM.read(0);
}

void LED_Flash(int hz){
  uint8_t Ti=(hz/60)*1000;
  for(uint8_t i=0;i<=hz;i++){
    digitalWrite(LED,1);
    delay(Ti);
    digitalWrite(LED,0);
    delay(Ti);
  }
}

////////////////////////////////////////////////////////////
void setup() {
  // MQTT
  pc.setServer(MQTT_SERVER_IP,MQTT_PORT);
  
  //
  pinMode(Relay, OUTPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
  //
  Wifi_connect();
  pc.connect(WiFi.macAddress().c_str());
  //
  EEPROM.begin(1024);
  

}

void loop() {

  if(WiFi.status()!=WL_CONNECTED){
    Serial.println();
    Serial.println("WiFi is break reconnect ... ");
    Wifi_connect();
    // MQTT_connect();
  }else{
    if(!pc.connected()){
      MQTT_connect();
      // String topic=WiFi.macAddress()+" -Relay_Receive";
      // pc.publish(topic.c_str(),"Hello Guys");
    }else{
      pc.loop();
    }
  }

  delay(2000);
}

///////////////////////////////////////////////////////////




