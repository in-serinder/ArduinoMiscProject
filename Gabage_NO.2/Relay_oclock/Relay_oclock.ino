#include <ESP8266WiFi.h>
// #include <ESP8266WebServer.h>
#include <string.h>
#include <stdlib.h>
#include <EEPROM.h>
#include <PubSubClient.h>

#include <ThreeWire.h>
#include <RtcDS1302.h>

// #include <TM1637Display.h>

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

#define Buzzer D6

// #define TM1637_CLK D6
// #define TM1637_DIO D7

// #define Buzzer D8

const char *WIFI_SSID = "2.4GHZ";
const char *WIFI_PASSWORD = "pwd";

const char *MQTT_SERVER_IP = "broker";
const int MQTT_PORT = 1883;
const char *MQTT_USER = "Relay";
const char *MQTT_PASSWORD = "password";

const char *MQTT_TOPIC = "Relay/Server_Master";
const char *DHT11_TOPIC = "Temperature/Node1";

const u32_t ZONE = 28800; // U8

const char *NTP_SERVERS[] = {"ntp.pool.org",
                             "cn.pool.ntp.org",
                             "time.windows.com",
                             "ntp.ntsc.ac.cn"};

IPAddress ip;
WiFiUDP ntpUDP;
NTPClient tc(ntpUDP, NTP_SERVERS[1], ZONE, 6000);

WiFiClient wc;
PubSubClient pc(wc);
// PubSubClient pc_dht(wc);

// 时钟
ThreeWire Wire(DS_DATA, DS_CLK, DS_RST);
RtcDS1302<ThreeWire> Rtc(Wire);
RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

// TM1637Display tmdisplay(TM1637_CLK, TM1637_DIO);

boolean RELAY_STATUS[6];
boolean tempMessage = false;
int timecount = 0;
float temperature, humidity;
boolean warnFlag = false;

void MQTTCallBack(char *topic, byte *payload, unsigned int len)
{
  // Serial.println("data from - " + String(topic) + " ");
  String str = "", msg = "";
  uint8_t pos = 0;
  float dhtvalue = 0;
  boolean istemperature = false;
  for (unsigned int i = 0; i < len; i++)
  {
    // Serial.print((char)payload[i]);
    msg += String((char)payload[i]);
  }
  Serial.println();
  Serial.println("------------------------");

  if (msg[0] == '>' && (!msg.isEmpty()))
  {
    tempMessage = true;
    istemperature = (msg.substring(1, 2) == "T") ? true : false;
    dhtvalue = atoi(msg.substring(2).c_str());
  }
  else
  {
    tempMessage = false;
  }

  if (msg[0] == '~' && (!msg.isEmpty()) && (msg[0] != '>'))
  {
    pos = atoi((msg.substring(1, 2)).c_str()); // 继电器号
    str = msg.substring(2);
  }

  // 温度获取,错误时数值处于253
  if (istemperature)
  {
    if (dhtvalue >= 253)
    {
      pc.publish(DHT11_TOPIC, "~GET_TEMP");
    }
    else
    {
      temperature = dhtvalue;
    }
  }
  else
  {
    if (dhtvalue >= 253)
    {
      pc.publish(DHT11_TOPIC, "~GET_HUM");
    }
    else
    {
      humidity = dhtvalue;
    }
  }

  Serial.println("Massage: " + ((str.isEmpty()) ? String(dhtvalue) : str));
  Serial.println("OPT obj :" + String(pos));

  // 继电器相关
  // Main str -> Command
  if (str == "status" || str == "tatus")
  {
    RelayStatus();
    Serial.println("Command--" + str);
  }

  if (str == "NO")
  {
    Relay_NO_swich(1, pos);
    Serial.println("Command--" + str);
  }

  if (str == "NC")
  {
    Relay_NO_swich(0, pos);
    Serial.println("Command--" + str);
  }
  // 复位开关
  if (str == "SW")
  {
    Relay_NO_swich(1, pos);
    delay(800);
    Relay_NO_swich(0, pos);
    Serial.println("Command--" + str);
  }
}

// void DHTCallBack(char *topic, byte *payload, unsigned int len)
// {
//   Serial.println("data from - " + String(topic) + " ");
//   String str = "";
//   uint8_t pos = 0;
//   for (unsigned int i = 0; i < len; i++)
//   {
//     Serial.print((char)payload[i]);
//     str += String((char)payload[i]);
//   }

// }

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
  NtpUpdate();
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
  pc.subscribe(DHT11_TOPIC);

  Serial.println("Topic: " + String(MQTT_TOPIC));

  pc.publish(MQTT_TOPIC, data.c_str());

  pc.setCallback(MQTTCallBack);

  Serial.println("Connect MQTT Server " + String(MQTT_SERVER_IP) + " Success");

  // 获取
  pc.publish(DHT11_TOPIC, "~GET_TEMP");
  pc.publish(DHT11_TOPIC, "~GET_HUM");

  return 1;
}

void RelayStatus()
{
  String status = "";
  for (int i = 0; i < 6; i++)
  {
    status += String("Relay" + String(i + 1) + "-Device:" + (RELAY_STATUS[i] ? "NO" : "NC") + " Code:" + String(RELAY_STATUS[i]) + " " + ((i > 2) ? "EX\n" : "IN\n"));
  }
  LED_Flash(30);
  Serial.println(status);
  pc.publish(MQTT_TOPIC, status.c_str());
}

// NO 1 NC 0
// Normally Open&Normally Close
void Relay_NO_swich(int sw, uint8_t pos)
{
  Serial.println("sw Submit: " + String(sw));

  switch (pos)
  {
  case 1:
    digitalWrite(Relay1, !sw);
    RELAY_STATUS[0, sw];
    Serial.println("Option : " + String(pos));
    break;
  case 2:
    digitalWrite(Relay2, !sw);
    RELAY_STATUS[1, sw];
    Serial.println("Option : " + String(pos));
    break;
  case 3:
    digitalWrite(Relay3, !sw);
    RELAY_STATUS[2, sw];
    Serial.println("Option : " + String(pos));
  default:
    RELAY_STATUS[pos - 1] = sw ? true : false;
    pc.publish(MQTT_TOPIC, "RELAYEX");
    break;
  }
  // digitalWrite(Relay,!sw);
  digitalWrite(LED, !sw);

  String state = (sw == 1) ? "NO" : "NC";
  pc.publish(MQTT_TOPIC, ("Relay- " + state + String(pos)).c_str());
  setRelayStatus(pos, sw);
  // WriteData(sw);

  if (sw)
  {
    tone(Buzzer, 456);
    delay(200);
    tone(Buzzer, 256);
    delay(200);
    noTone(Buzzer);
  }
  else
  {
    tone(Buzzer, 256);
    delay(200);
    tone(Buzzer, 456);
    delay(200);
    noTone(Buzzer);
  }
}

void WriteData(int sw, uint8_t pos)
{
  EEPROM.write(pos, sw);
  EEPROM.commit();
  Serial.println("Write: " + String(sw) + " Read: " + String(EEPROM.read(0)));
}

void setRelayStatus(uint8_t status, uint8_t pos)
{
  RELAY_STATUS[pos] = status;
  WriteData(pos, status);
}

void getRelayStatus()
{
  for (u8_t i = 0; i < 6; i++)
  {
    RELAY_STATUS[i] = ReadData(i);
  }
}

uint8_t ReadData(uint8_t pos)
{
  return EEPROM.read(pos);
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

void NtpUpdate()
{
  tc.update();
  Serial.println(tc.getFormattedTime());
  setTime(tc.getEpochTime());
  RtcDateTime current = RtcDateTime(year(), month(), day(), hour(), minute(), second());
  Rtc.SetDateTime(current);
  RtcDateTime now = Rtc.GetDateTime();
  Serial.printf("Rtc-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n", now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second());
}

void initialization()
{

  getRelayStatus();

  pinMode(Relay1, OUTPUT);
  pinMode(Relay2, OUTPUT);
  pinMode(Relay3, OUTPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(Relay1, ReadData(1));
  digitalWrite(Relay2, ReadData(2));
  digitalWrite(Relay3, ReadData(3));

  digitalWrite(LED, !(RelayStatus[0] || RELAY_STATUS[1] || RELAY_STATUS[2]));

  // Oclock
  tc.begin();
  // tmdisplay.setBrightness(0x0f);
  Rtc.Begin();
  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);

  sync_relaystatusEtoR();
}

// void show_clock(uint8_t hours, uint8_t minutes, bool Showdot) {
//   uint8_t d1 = hours / 10;    // Tens digit of hours
//   uint8_t d2 = hours % 10;    // Ones digit of hours
//   uint8_t d3 = minutes / 10;  // Tens digit of minutes
//   uint8_t d4 = minutes % 10;  // Ones digit of minutes

//   /*
//   char msg[100] = "";
//   snprintf(msg, 100, "Displaying: %d hours %d minutes (%d%d:%d%d)\n", hours, minutes, d1, d2, d3, d4);
//   Serial.print(msg);
//   */

//   uint8_t digits[4];
//   digits[0] = tmdisplay.encodeDigit(d1);
//   digits[1] = tmdisplay.encodeDigit(d2);
//   digits[2] = tmdisplay.encodeDigit(d3);
//   digits[3] = tmdisplay.encodeDigit(d4);

//   // Turn on the colon
//   if (Showdot) {
//     digits[1] |= 128;
//   } else {
//     digits[1] &= ~128;
//   }

//   // No leading zero on times like 1:34
//   if (!d1) {
//     digits[0] = 0;  // Off
//   }

//   tmdisplay.setSegments(digits, 4, 0);
// }

void sync_relaystatusEtoR()
{
  for (int i = 0; i < 6; i++)
  {
    RELAY_STATUS[i] = ReadData(i);
  }
}

void sync_relaystatusRtoE()
{
  for (int i = 0; i < 6; i++)
  {
    WriteData(i, RELAY_STATUS[i]);
  }
}

String Fixxint(int value)
{
  if (value < 10)
  {
    return "0" + String(value); // 生成独立的String，如"07"
  }
  else
  {
    return String(value); // 生成独立的String，如"12"
  }
}

String Fixxfloat(float value)
{
  if (value < 10)
  {
    return "0" + String(value); // 生成独立的String，如"07"
  }
  else
  {
    return String(value); // 生成独立的String，如"12"
  }
}

// const char *Fixxfloat(float value) {
//   static char buf[5];
//   if (value < 10) {
//     sprintf(buf, "0%2.2f", value);
//   } else {
//     sprintf(buf, "%2.2f", value);
//   }
//   return buf;
// }

////////////////////////////////////////////////////////////
void setup()
{
  EEPROM.begin(16);
  Serial.begin(9600);
  initialization();
  // MQTT
  pc.setServer(MQTT_SERVER_IP, MQTT_PORT);

  //
  Wifi_connect();
  pc.connect(WiFi.macAddress().c_str());
}

void loop()
{
  timecount++;

  if (timecount % 100 == 0)
  {
    pc.publish(DHT11_TOPIC, "~GET_TEMP");
    pc.publish(DHT11_TOPIC, "~GET_HUM");
  }

  sync_relaystatusRtoE();
  RtcDateTime now = Rtc.GetDateTime();
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println();
    Serial.println("WiFi is break reconnect ... ");
    Wifi_connect();
    // show_clock(07, 21, true);
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

    // show_clock(now.Hour(), now.Minute(), true);
    // delay(1000);
    // show_clock(now.Hour(), now.Minute(), false);
    // delay(1000);
  }
  digitalWrite(LED, !(RELAY_STATUS[0] || RELAY_STATUS[1] || RELAY_STATUS[2]));

  // Serial.printf("NTP-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n",year(),month(),day(),hour(),minute(),second());
  Serial.printf("Rtc-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n", now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second());
  //  *完全格式 >20231024^160000^000^00.00^00.00^ 日期^时间^继电器按位^温度^湿度

  // Serial.printf(
  //   ">%d%s%s^%s%s%s^%d%d%d^%s^%s^%d\n",
  //   now.Year(),
  //   Fixxint(now.Month()),
  //   Fixxint(now.Day()),
  //   Fixxint(now.Hour()),
  //   Fixxint(now.Minute()),
  //   Fixxint(now.Second()),  // 秒补0
  //   RELAY_STATUS[3],
  //   RELAY_STATUS[4],
  //   RELAY_STATUS[5],
  //   Fixxfloat(temperature),
  //   Fixxfloat(humidity),
  //   warnFlag );

  Serial.printf(
      ">%d%s%s^%s%s%s^%d%d%d^%5s^%5s^%d\n",
      now.Year(),
      Fixxint(now.Month()),
      Fixxint(now.Day()),
      Fixxint(now.Hour()),
      Fixxint(now.Minute()),
      Fixxint(now.Second()), // 秒补0
      RELAY_STATUS[3],
      RELAY_STATUS[4],
      RELAY_STATUS[5],
      Fixxfloat(temperature),
      Fixxfloat(humidity),
      warnFlag);

  // if(temperature<10||humidity<10){
  //       Serial.printf(">%d%s%d^%d%d%d^%d%d%d^0%2.2f^0%2.2f^%d\n",now.Year(),now.Month(),now.Day(),
  //                                             now.Hour(),now.Minute(),now.Second(),
  //                                             RELAY_STATUS[3],RELAY_STATUS[4],RELAY_STATUS[5],
  //                                             temperature,humidity,
  //                                             warnFlag?1:0);

  // }else{
  //       Serial.printf(">%d%d%d^%d%d%d^%d%d%d^%2.2f^%2.2f^%d\n",now.Year(),now.Month(),now.Day(),
  //                                                     now.Hour(),now.Minute(),now.Second(),
  //                                                     RELAY_STATUS[3],RELAY_STATUS[4],RELAY_STATUS[5],
  //                                                     temperature,humidity,
  //                                                     warnFlag?1:0);
  // }

  // Serial.println(String(String(now.Year()) +
  //               (now.Month() < 10) ? ("0" + String(now.Month())) : String(now.Month())+
  //               (now.Day()<10)?("0"+String(now.Day())):String(now.Day())+
  //               "^"+
  //               (now.Hour()<10)?("0"+String(now.Hour())):String(now.Hour())+
  //               (now.Minute()<10)?("0"+String(now.Minute())):String(now.Minute())+
  //               (now.Second()<10)?("0"+String(now.Second())):String(now.Second())+
  //               "^"+
  //               String(RELAY_STATUS[3])+
  //               String(RELAY_STATUS[4])+
  //               String(RELAY_STATUS[5])+
  //               "^"+
  //               (temperature<10)?"0"+String(temperature):String(temperature)+
  //               (humidity<10)?"0"+String(humidity):String(humidity)+
  //               "^"+
  //               String(warnFlag ? 1 : 0)));

  delay(1000);
}

///////////////////////////////////////////////////////////
