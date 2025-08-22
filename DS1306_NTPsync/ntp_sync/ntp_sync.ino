#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <string.h>
#include <TimeLib.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#define DS_CLK D0
#define DS_DATA D1
#define DS_RST D2

const char *WIFI_SSID = "2.4GHZ";
const char *WIFI_PASSWORD = "320724fuck";
const u32_t ZONE = 28800; // U8

const char *NTP_SERVERS[] = {"ntp.pool.org",
                             "cn.pool.ntp.org",
                             "time.windows.com",
                             "ntp.ntsc.ac.cn"};

ThreeWire Wire(DS_DATA, DS_CLK, DS_RST);
RtcDS1302<ThreeWire> Rtc(Wire);

IPAddress ip;
WiFiUDP ntpUDP;
NTPClient tc(ntpUDP, NTP_SERVERS[2], ZONE, 6000);

void setup()
{
  Serial.begin(9200);
  // WIFI_Connect();
  digitalWrite(Relay, uint8_t val)
  // tc.begin();
  Rtc.Begin();

  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);
}

void loop()
{
  // tc.update();
  // Serial.println(tc.getFormattedTime());
  // setTime(tc.getEpochTime());
  // Serial.printf("NTP-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n", year(), month(), day(), hour(), minute(), second());
  // RtcDateTime current = RtcDateTime(year(), month(), day(), hour(), minute(), second());
  // Rtc.SetDateTime(current);
  RtcDateTime now = Rtc.GetDateTime();
  Serial.printf("Rtc-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n", now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second());

  if (!Rtc.IsDateTimeValid())
  {
    Serial.println("Error Date!");
  }

  delay(1000);

  Serial.println("---------------------------------------------");
}

void WIFI_Connect()
{
  int count = 0;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connect:");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
    count++;
  }
  Serial.print("\nConnect to " + String(WIFI_SSID) + " Successful\n");
  Serial.println("\nMAC: " + WiFi.macAddress() + "\nIP: " + WiFi.localIP().toString() + "\nUsing Time:" + count + "s");
}