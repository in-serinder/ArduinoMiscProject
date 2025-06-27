#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <string.h>
#include <stdlib.h>
#include<EEPROM.h>
#include <PubSubClient.h>

#include <ThreeWire.h>
#include <RtcDS1302.h>

#include <TM1637Display.h>



#include <TimeLib.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#define Relay1 D0
#define Relay2 D1
#define Relay3 D2
#define LED 2

#define DS_CLK D3
#define DS_DATA D4
#define DS_RST D5

#define TM1637_CLK D6
#define TM1637_DIO D7

#define Buzzer D8





const char* WIFI_SSID="2.4GHZ";
const char* WIFI_PASSWORD="320724fuck";

const char* MQTT_SERVER_IP="8.130.191.142";
const int MQTT_PORT=1883;
const char* MQTT_USER="Relay";
const char* MQTT_PASSWORD="320724fuck";

const char* MQTT_TOPIC="Relay/Server_Master";

const u32_t ZONE = 28800;  //U8

const char* NTP_SERVERS[] = {"ntp.pool.org",
                            "cn.pool.ntp.org",
                            "time.windows.com",
                            "ntp.ntsc.ac.cn"};

IPAddress ip;
WiFiUDP ntpUDP;
NTPClient tc(ntpUDP,NTP_SERVERS[1],ZONE,6000);                            

WiFiClient wc;
PubSubClient pc(wc);

// 时钟
ThreeWire Wire(DS_DATA,DS_CLK,DS_RST);
RtcDS1302<ThreeWire> Rtc(Wire);
RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

TM1637Display tmdisplay(TM1637_CLK,TM1637_DIO);



boolean RELAY_STATUS[3];




void MQTTCallBack(char* topic , byte* payload,unsigned int len){
  Serial.println("data from - "+String(topic) + " ");
  String str="";
  uint8_t pos = 0;
  for(unsigned int i=0;i<len;i++){
    Serial.print((char)payload[i]);
    str+=String((char)payload[i]);
  }
  Serial.println();
  Serial.println("------------------------");

  if(str[0]=='~'&&(!str.isEmpty())){
    pos = atoi((str.substring(1,2)).c_str());
    str = str.substring(2);
  }


  Serial.println("Massage: "+str);
  Serial.println("OPT obj :" + String(pos));

  // Main str -> Command
  if(str == "status"||str == "tatus"){
    RelayStatus();
    Serial.println("Command--"+str);
  }

  if (str == "NO") {
    Relay_NO_swich(1,pos);
    Serial.println("Command--"+str);
  }

  if (str == "NC") {
    Relay_NO_swich(0,pos);
    Serial.println("Command--"+str);
  }
// 复位开关
    if (str == "SW") {
    Relay_NO_swich(1,pos);
    delay(800);
    Relay_NO_swich(0,pos);
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
  NtpUpdate();
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
  String status = "Power: " + '\n' 
      + String(!digitalRead(Relay1)? "NO" : "NC") + "  Last: " + (ReadData(1) ? "NO" : "NC" ) + " Status Code: " + (RELAY_STATUS[0]? "NO" : "NC") +'\n'
      + String(!digitalRead(Relay2)? "NO" : "NC") + "  Last: " + (ReadData(2) ? "NO" : "NC" ) + " Status Code: " + (RELAY_STATUS[1]? "NO" : "NC")+'\n'
      + String(!digitalRead(Relay3)? "NO" : "NC") + "  Last: " + (ReadData(3) ? "NO" : "NC" ) + " Status Code: " + (RELAY_STATUS[2]? "NO" : "NC")+'\n';
  LED_Flash(30);
  Serial.println(status);
  pc.publish(MQTT_TOPIC,status.c_str());
}


//NO 1 NC 0
//Normally Open&Normally Close
void Relay_NO_swich(int sw,uint8_t pos){
  Serial.println("sw Submit: " + String(sw));

  switch (pos) {
  case 1:digitalWrite(Relay1, !sw);Serial.println("Option : " +String(pos));break;
  case 2:digitalWrite(Relay2, !sw);Serial.println("Option : " + String(pos));break;
  case 3:digitalWrite(Relay3, !sw);Serial.println("Option : " + String(pos));break;
  }
  // digitalWrite(Relay,!sw);
  digitalWrite(LED,!sw);


  String state = (sw == 1) ? "NO" : "NC";
  pc.publish(MQTT_TOPIC, ("Relay- " + state + String(pos)).c_str());
  setRelayStatus(pos, sw);
  // WriteData(sw);

  if (sw) {
    tone(Buzzer,456);
    delay(200);
    tone(Buzzer,256);
    delay(200);
    noTone(Buzzer);
  }else{
    tone(Buzzer,256);
    delay(200);
    tone(Buzzer,456);
    delay(200);
    noTone(Buzzer);
  }
}

void WriteData(int sw,uint8_t pos){
  EEPROM.write(pos,sw);
  EEPROM.commit();
  Serial.println("Write: " + String(sw) + " Read: " + String(EEPROM.read(0)));
}

void setRelayStatus(uint8_t status,uint8_t pos){
  RELAY_STATUS[pos] = status ;
  WriteData(pos, status);
}

void getRelayStatus(){
  for (u8_t i =0; i<3; i++) {
    RELAY_STATUS[i] = ReadData(i);
  }
}

uint8_t ReadData(uint8_t pos){
  return EEPROM.read(pos);
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

void NtpUpdate(){
  tc.update();
  Serial.println(tc.getFormattedTime());
  setTime(tc.getEpochTime());
    RtcDateTime current = RtcDateTime(year(),month(),day(),hour(),minute(),second());
   Rtc.SetDateTime(current);
  RtcDateTime now = Rtc.GetDateTime();
  Serial.printf("Rtc-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n",now.Year(),now.Month(),now.Day(),now.Hour(),now.Minute(),now.Second());
}

void initialization(){
  
  getRelayStatus();

  pinMode(Relay1, OUTPUT);
  pinMode(Relay2, OUTPUT);
  pinMode(Relay3, OUTPUT);
  pinMode(LED, OUTPUT);
 
  digitalWrite(Relay1, ReadData(1));
  digitalWrite(Relay2, ReadData(2));
  digitalWrite(Relay3, ReadData(3));

  digitalWrite(LED, !(RelayStatus[0]||RELAY_STATUS[1]||RELAY_STATUS[2]));
  

  // Oclock
  tc.begin();
  tmdisplay.setBrightness(0x0f);
  Rtc.Begin();
  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);




}



void show_clock(uint8_t hours, uint8_t minutes, bool Showdot) {
    uint8_t d1 = hours   / 10; // Tens digit of hours
    uint8_t d2 = hours   % 10; // Ones digit of hours
    uint8_t d3 = minutes / 10; // Tens digit of minutes
    uint8_t d4 = minutes % 10; // Ones digit of minutes
    
    /*
    char msg[100] = "";
    snprintf(msg, 100, "Displaying: %d hours %d minutes (%d%d:%d%d)\n", hours, minutes, d1, d2, d3, d4);
    Serial.print(msg);
    */

    uint8_t digits[4];
    digits[0] = tmdisplay.encodeDigit(d1);
    digits[1] = tmdisplay.encodeDigit(d2); 
    digits[2] = tmdisplay.encodeDigit(d3);
    digits[3] = tmdisplay.encodeDigit(d4);

    // Turn on the colon
    if (Showdot) {
      digits[1] |= 128 ;
    }else{
      digits[1] &= ~128;
    }



    // No leading zero on times like 1:34
    if (!d1) {
        digits[0] = 0; // Off
    }

    tmdisplay.setSegments(digits, 4, 0);
}

////////////////////////////////////////////////////////////
void setup() {
  EEPROM.begin(16);
  Serial.begin(9600);
  initialization();
  // MQTT
  pc.setServer(MQTT_SERVER_IP,MQTT_PORT);

  
  //
  Wifi_connect();
  pc.connect(WiFi.macAddress().c_str());
  


}

void loop() {

  RtcDateTime now = Rtc.GetDateTime();
  if(WiFi.status()!=WL_CONNECTED){
    Serial.println();
    Serial.println("WiFi is break reconnect ... ");
    Wifi_connect();
    show_clock(07, 21,true);
    // MQTT_connect();
  }else{
    if(!pc.connected()){
      MQTT_connect();
      // String topic=WiFi.macAddress()+" -Relay_Receive";
      // pc.publish(topic.c_str(),"Hello Guys");
    }else{
      pc.loop();
      
    }

  show_clock(now.Hour(), now.Minute(),true);
  delay(1000);
  show_clock(now.Hour(), now.Minute(),false);
  delay(1000);
  }
  digitalWrite(LED,!(RelayStatus[0]||RELAY_STATUS[1]||RELAY_STATUS[2]));
  
  
  // Serial.printf("NTP-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n",year(),month(),day(),hour(),minute(),second());
  // Serial.printf("Rtc-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n",now.Year(),now.Month(),now.Day(),now.Hour(),now.Minute(),now.Second());

}

///////////////////////////////////////////////////////////




