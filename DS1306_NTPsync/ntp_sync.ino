#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <string.h>
#include <TimeLib.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#define DS_CLK 5
#define DS_DATA 4
#define DS_RST 2



const char* WIFI_SSID = "Arch-Chan";
const char* WIFI_PASSWORD = "320724fuck";
const u32_t ZONE = 28800;


const char* NTP_SERVERS[] = {"ntp.pool.org",
                            "cn.pool.ntp.org",
                            "time.windows.com",
                            "ntp.ntsc.ac.cn"};


ThreeWire Wire(DS_DATA,DS_CLK,DS_RST);
RtcDS1302<ThreeWire> Rtc(Wire);

IPAddress ip;
WiFiUDP ntpUDP;
NTPClient tc(ntpUDP,NTP_SERVERS[1],ZONE,6000);

void setup() {
  Serial.begin(9200);
  WIFI_Connect();
  
  tc.begin();
  Rtc.Begin();

  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);


}

void loop() {
  // put your main code here, to run repeatedly:
  tc.update();
  Serial.println(tc.getFormattedTime());
  setTime(tc.getEpochTime());
  Serial.printf("NTP-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n",year(),month(),day(),hour(),minute(),second());
  RtcDateTime current = RtcDateTime(year(),month(),day(),hour(),minute(),second());
   Rtc.SetDateTime(current);
  RtcDateTime now = Rtc.GetDateTime();
  Serial.printf("Rtc-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n",now.Year(),now.Month(),now.Day(),now.Hour(),now.Minute(),now.Second());
 
  delay(1000);
  

}






void WIFI_Connect(){
  int count=0;
  WiFi.mode(WIFI_STA); //STA模式 当然还有其他参数，WIFI_AP，WIFI_OFF，分别表示开启接入点模式和关闭WIFI。
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  Serial.print("connect:");
  while(WiFi.status()!= WL_CONNECTED){
      Serial.print('.');
      delay(1000);
      count++;
  }
        Serial.print("\nConnect to "+String(WIFI_SSID)+" Successful\n");
        Serial.println("\nMAC: " + WiFi.macAddress() + "\nIP: " + WiFi.localIP().toString()+"\nUsing Time:"+count+"s");
        
  
}