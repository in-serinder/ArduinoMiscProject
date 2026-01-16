#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <string.h>
#include <TimeLib.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#define DS_CLK D3
#define DS_DATA D4
#define DS_RST D5

const char *WIFI_SSID = "2.4GHZ";
const char *WIFI_PASSWORD = "320724fuck";
const u32_t ZONE = 28800; // U8

const char *NTP_SERVERS[] = {"ntp.pool.org",
                             "cn.pool.ntp.org",
                             "time.windows.com",
                             "ntp.ntsc.ac.cn",
                             "203.107.6.88"};

ThreeWire Wire(DS_DATA, DS_CLK, DS_RST);
RtcDS1302<ThreeWire> Rtc(Wire);

IPAddress ip;
WiFiUDP ntpUDP;
NTPClient tc(ntpUDP, NTP_SERVERS[4], ZONE, 60000);

void setup()
{
  Serial.begin(9600);
  WIFI_Connect();
    delay(3000);

  // digitalWrite(Relay, uint8_t val)
  tc.begin();
  Rtc.Begin();

 bool initNtpOk = tc.forceUpdate();
  Serial.printf("首次NTP同步结果：%s\n", initNtpOk ? "成功" : "失败");
  Serial.printf("NTP时间戳：%lu\n", tc.getEpochTime());

  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);
}

void loop()
{
  tc.forceUpdate();
  Serial.println(tc.getFormattedTime());
  setTime(tc.getEpochTime());
  Serial.printf("NTP-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n", year(), month(), day(), hour(), minute(), second());
  RtcDateTime current = RtcDateTime(year(), month(), day(), hour(), minute(), second());
  Rtc.SetDateTime(current);
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