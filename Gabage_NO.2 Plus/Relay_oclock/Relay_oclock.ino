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

// 天气显示器相关
#include <ESP8266HTTPClient.h> //开发板版本头文件名不一
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define Relay1 D0
#define Relay2 D1
#define Relay3 D1 // 停用3继电器，D1占位被控
// #define Relay3 D2
#define LED 2

#define DS_CLK D3
#define DS_DATA D4
#define DS_RST D5

#define Buzzer D2 // 替换为

#define OLED_ADDR 0x3C
#define OLED_HEIGHT 64
#define OLED_WIDTH 128
#define OLED_RESET -1

#define SDA D6
#define SCL D7

#define WEATHERTEXTLEN 6 // 原为6
#define CHINESEARRLEN 27
#define WEATHERUPDATEHOURSDELAY 3 // 更新间隔(小时)

const char *API_REQUESTURL = "http://restapi.amap.com/v3/weather/weatherInfo?";
const char *API_KEY = "key";
const char *CITY_CODE = "citykey";

// #define TM1637_CLK D6
// #define TM1637_DIO D7

// #define Buzzer D8

const char *WIFI_SSID = "2.4GHZ";
const char *WIFI_PASSWORD = "passwd";

const char *MQTT_SERVER_IP = "broker";
const int MQTT_PORT = 1883;
const char *MQTT_USER = "Relay";
const char *MQTT_PASSWORD = "password";

const char *MQTT_TOPIC = "Relay/Server_Master";
const char *DHT11_TOPIC = "Temperature/Node1";

const u32_t ZONE = 28800; // U8

const u8_t RELAY_LEN = 6;

const char *NTP_SERVERS[] = {"ntp.pool.org",
                             "cn.pool.ntp.org",
                             "time.windows.com",
                             "ntp.ntsc.ac.cn"};

const unsigned char Chinese_Arr[CHINESEARRLEN][32] = {
    {0x00, 0x00, 0x00, 0x20, 0x03, 0xFE, 0x78, 0x20, 0x49, 0xFC, 0x48, 0x20, 0x4B, 0xFE, 0x78, 0x00,
     0x49, 0xFC, 0x49, 0x04, 0x49, 0xFC, 0x79, 0x04, 0x49, 0xFC, 0x41, 0x04, 0x01, 0x0C, 0x01, 0x08}, /*"晴",0*/
    {0x00, 0x00, 0x00, 0x00, 0x1F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFE, 0x7F, 0xFE,
     0x03, 0x00, 0x02, 0x00, 0x04, 0x20, 0x0C, 0x30, 0x18, 0x18, 0x3F, 0xFC, 0x18, 0x04, 0x00, 0x00}, /*"云",1*/
    {0x00, 0x00, 0x00, 0x00, 0x1F, 0xF8, 0x10, 0x08, 0x10, 0x48, 0x14, 0x48, 0x12, 0x48, 0x13, 0x88,
     0x11, 0x88, 0x31, 0x88, 0x23, 0xC8, 0x26, 0x48, 0x2C, 0x6A, 0x60, 0x0E, 0x40, 0x06, 0x40, 0x00}, /*"风",2*/
    {0x00, 0x00, 0x00, 0x00, 0x7E, 0xFC, 0x44, 0x84, 0x44, 0x84, 0x4C, 0xFC, 0x48, 0xFC, 0x48, 0x84,
     0x44, 0x84, 0x44, 0xFC, 0x44, 0x84, 0x5C, 0x84, 0x49, 0x04, 0x43, 0x04, 0x42, 0x1C, 0x02, 0x00}, /*"阴",3*/
    {0x00, 0x00, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x80, 0x00, 0x80, 0x3F, 0xFC, 0x20, 0x84, 0x2C, 0xA4,
     0x26, 0x94, 0x20, 0x84, 0x20, 0xA4, 0x24, 0xB4, 0x20, 0x84, 0x20, 0x84, 0x20, 0x8C, 0x00, 0x00}, /*"雨",4*/
    {0x00, 0x00, 0x3F, 0xFC, 0x00, 0x80, 0x7F, 0xFE, 0x60, 0x82, 0x78, 0xF2, 0x1E, 0xF8, 0x04, 0x00,
     0x1F, 0xF0, 0x33, 0x60, 0x07, 0xE0, 0x39, 0x1C, 0x1F, 0xF8, 0x02, 0x18, 0x3C, 0x70, 0x00, 0x00}, /*"雾",5*/
    {0x00, 0x00, 0x3F, 0xFC, 0x01, 0x00, 0x7F, 0xFE, 0x5D, 0x3E, 0x41, 0x06, 0x09, 0x18, 0x18, 0xFC,
     0x2A, 0xA4, 0x1C, 0xFC, 0x74, 0xFC, 0x1A, 0x20, 0x27, 0xFC, 0x1A, 0x20, 0x2F, 0xFE, 0x00, 0x00}, /*"霾",6*/
    {0x00, 0x00, 0x3F, 0xFC, 0x01, 0x80, 0x7F, 0xFE, 0x41, 0x82, 0x5D, 0xBA, 0x01, 0x80, 0x1D, 0xB8,
     0x00, 0x00, 0x1F, 0xF8, 0x11, 0x88, 0x1F, 0xF8, 0x11, 0x88, 0x11, 0x88, 0x1F, 0xF8, 0x10, 0x08}, /*"雷",7*/
    {0x00, 0x00, 0x00, 0x40, 0x00, 0x40, 0x60, 0x40, 0x30, 0x44, 0x17, 0x4C, 0x01, 0x78, 0x01, 0x70,
     0x11, 0x70, 0x13, 0x50, 0x22, 0x58, 0x66, 0x4C, 0x4C, 0x46, 0x08, 0x40, 0x00, 0xC0, 0x00, 0x00}, /*"冰",8*/
    {0x00, 0x00, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x80, 0x7F, 0xFE, 0x60, 0x82, 0x6E, 0xFA, 0x1E, 0xB8,
     0x00, 0x00, 0x20, 0x00, 0x3F, 0xFC, 0x00, 0x0C, 0x1F, 0xFC, 0x00, 0x0C, 0x1F, 0xFC, 0x00, 0x0C}, /*"雪",9*/
    {0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x19, 0x90, 0x11, 0x88, 0x31, 0x8C, 0x61, 0x86, 0x00, 0x00,
     0x01, 0x80, 0x01, 0x80, 0x3F, 0xFC, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x7F, 0xFE, 0x00, 0x00}, /*"尘",10*/
    {0x00, 0x00, 0x08, 0x20, 0x08, 0x20, 0x69, 0xFE, 0x48, 0x20, 0x7E, 0x20, 0x49, 0xFE, 0x48, 0x08,
     0x08, 0x08, 0x1F, 0xFE, 0x68, 0x08, 0x08, 0x88, 0x08, 0xC8, 0x08, 0x08, 0x08, 0x38, 0x08, 0x10}, /*"特",11*/
    {0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x7F, 0xFE, 0x7F, 0xFE, 0x01, 0x80,
     0x01, 0x80, 0x03, 0xC0, 0x02, 0x60, 0x04, 0x20, 0x0C, 0x10, 0x38, 0x1C, 0x60, 0x06, 0x00, 0x00}, /*"大",12*/
    {0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x3F, 0xFC, 0x21, 0x84, 0x21, 0x84, 0x21, 0x84,
     0x21, 0x84, 0x3F, 0xFC, 0x21, 0x84, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80}, /*"中",13*/
    {0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x09, 0x90, 0x19, 0x98, 0x11, 0x88,
     0x11, 0x8C, 0x31, 0x84, 0x21, 0x84, 0x61, 0x84, 0x01, 0x80, 0x01, 0x80, 0x03, 0x80, 0x00, 0x00}, /*"小",14*/
    {0x00, 0x00, 0x18, 0x60, 0x18, 0x40, 0x7F, 0xF8, 0x7E, 0x98, 0x19, 0xF0, 0x7E, 0x3C, 0x00, 0x34,
     0x7E, 0xFF, 0x42, 0x34, 0x7E, 0x34, 0x42, 0xFC, 0x7E, 0x30, 0x42, 0x30, 0x46, 0x60, 0x00, 0x00}, /*"静",15*/
    {0x00, 0x00, 0x1F, 0xF8, 0x10, 0x08, 0x1F, 0xF8, 0x10, 0x08, 0x1F, 0xF8, 0x04, 0x20, 0x3F, 0xFC,
     0x7F, 0xFE, 0x04, 0x20, 0x19, 0x98, 0x6D, 0xA6, 0x07, 0xE0, 0x19, 0x98, 0x03, 0x8C, 0x00, 0x00}, /*"暴",16*/
    {0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x3F, 0xFC, 0x3F, 0xFC, 0x19, 0x8C, 0x09, 0x98, 0x0D, 0x90,
     0x01, 0x90, 0x7F, 0xFE, 0x01, 0x40, 0x03, 0x60, 0x06, 0x30, 0x0C, 0x18, 0x70, 0x0E, 0x00, 0x00}, /*"夹",17*/
    {0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x02, 0x00, 0x3F, 0xFC, 0x04, 0x00, 0x08, 0x80, 0x18, 0x80,
     0x3F, 0xFC, 0x00, 0x80, 0x00, 0x80, 0x0C, 0x98, 0x18, 0x8C, 0x30, 0x86, 0x23, 0x80, 0x02, 0x00}, /*"东",18*/
    {0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x7F, 0xFE, 0x01, 0x00, 0x7F, 0xFC, 0x20, 0x04, 0x26, 0x64,
     0x22, 0x44, 0x2F, 0xF4, 0x21, 0x84, 0x2F, 0xF4, 0x21, 0x84, 0x21, 0x84, 0x21, 0x8C, 0x60, 0x00}, /*"南",19*/
    {0x00, 0x00, 0x00, 0x00, 0x7F, 0xFE, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x3F, 0xFC, 0x32, 0x4C,
     0x32, 0x4C, 0x34, 0x4C, 0x34, 0x7C, 0x38, 0x0C, 0x30, 0x0C, 0x3F, 0xFC, 0x30, 0x0C, 0x00, 0x00}, /*"西",20*/
    {0x00, 0x00, 0x06, 0x40, 0x06, 0x40, 0x06, 0x40, 0x06, 0x40, 0x06, 0x46, 0x7E, 0x58, 0x06, 0x60,
     0x06, 0x40, 0x06, 0x40, 0x06, 0x40, 0x06, 0x40, 0x3E, 0x42, 0x66, 0x42, 0x06, 0x7E, 0x00, 0x00}, /*"北",21*/
    {0x00, 0x00, 0x00, 0x00, 0x1F, 0xF8, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7F, 0xFE,
     0x03, 0x40, 0x02, 0x40, 0x02, 0x40, 0x06, 0x40, 0x0C, 0x42, 0x18, 0x46, 0x70, 0x7C, 0x00, 0x00}, /*"无",22*/
    {0x00, 0x00, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x80, 0x01, 0x00, 0x03, 0x00, 0x07, 0x60, 0x05, 0x30,
     0x19, 0x18, 0x31, 0x0C, 0x61, 0x04, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00}, /*"不",23*/
    {0x00, 0x00, 0x01, 0x00, 0x01, 0x80, 0x3F, 0xFC, 0x20, 0x04, 0x20, 0x04, 0x1F, 0xF8, 0x00, 0x80,
     0x00, 0x80, 0x08, 0x80, 0x18, 0xF8, 0x18, 0x80, 0x3C, 0x80, 0x27, 0x80, 0x61, 0xFE, 0x00, 0x00}, /*"定",24*/
    {0x00, 0x00, 0x02, 0x00, 0x06, 0x00, 0x0F, 0xF0, 0x14, 0x30, 0x22, 0x60, 0x01, 0x80, 0x07, 0x60,
     0x38, 0xFE, 0x03, 0x04, 0x07, 0x0C, 0x18, 0x98, 0x00, 0x70, 0x00, 0xC0, 0x3F, 0x00, 0x30, 0x00}, /*"多",0*/
    {0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x0D, 0xB0, 0x19, 0x88, 0x11, 0x86, 0x21, 0x80,
     0x61, 0x80, 0x01, 0x98, 0x01, 0xB0, 0x00, 0x60, 0x00, 0xC0, 0x07, 0x00, 0x3C, 0x00, 0x00, 0x00}, /*"少",1*/

};

const unsigned char Symbol_Arr[3][32] = {
    {0x00, 0x00, 0x20, 0x00, 0x50, 0x00, 0x50, 0xF0, 0x23, 0x0C, 0x02, 0x04, 0x06, 0x00, 0x04, 0x00,
     0x04, 0x00, 0x04, 0x00, 0x04, 0x04, 0x06, 0x04, 0x03, 0x0C, 0x01, 0xF8, 0x00, 0x00, 0x00, 0x00}, /*"℃",0*/
    {0x00, 0x00, 0x20, 0x00, 0x50, 0x00, 0x53, 0xFE, 0x23, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00,
     0x03, 0xF8, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*"℉",1*/
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x38, 0x01, 0xC0, 0x0E, 0x00, 0x30, 0x00, 0x0E, 0x00,
     0x01, 0xC0, 0x20, 0x38, 0x1C, 0x04, 0x03, 0x80, 0x00, 0x70, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00}, /*"≤",2*/

};

const unsigned char Weather_IconArr[19][32] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf0, 0x0f, 0xf8, 0x1f, 0xf8, 0x3f, 0xfc, 0x7f, 0xfe,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xfe, 0x3f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 多云
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x0f, 0xc0, 0x0f, 0xc0, 0x1f, 0xf0, 0x3f, 0xfc,
        0x3f, 0xfc, 0x3f, 0xfc, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 闪电
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x0f, 0xc0, 0x0f, 0xc0, 0x1f, 0xf0, 0x3f, 0xfc,
        0x3f, 0xfc, 0x3f, 0xfc, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 大雪
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f, 0xfc,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x09, 0x10, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00}, // 雾霾
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x0f, 0xc0, 0x0f, 0xc0, 0x1f, 0xf0, 0x3f, 0xfc,
        0x3f, 0xfc, 0x3f, 0xfc, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 雾
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x0f, 0xc0, 0x0f, 0xc0, 0x1f, 0xf8, 0x3f, 0xfc,
        0x3f, 0xfc, 0x3f, 0xfc, 0x1f, 0xf8, 0x00, 0x00, 0x08, 0x10, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00}, // 雨夹雪
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xe0, 0x00, 0xf0, 0x0f, 0x70, 0x1f, 0xbe,
        0x3f, 0x8f, 0x7f, 0xf7, 0xff, 0xfb, 0xff, 0xf8, 0xff, 0xf8, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00}, // 阴
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x0f, 0xc0, 0x0f, 0xc0, 0x1f, 0xf8, 0x3f, 0xfc,
        0x3f, 0xfc, 0x3f, 0xfc, 0x1f, 0xf8, 0x00, 0x00, 0x09, 0x90, 0x09, 0x90, 0x00, 0x00, 0x00, 0x00}, // 大雨
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x07, 0xc0, 0x0f, 0xc0, 0x1f, 0xf8, 0x3f, 0xfc,
        0x3f, 0xfc, 0x3f, 0xfc, 0x1f, 0xf8, 0x00, 0x00, 0x09, 0x80, 0x0d, 0xb0, 0x00, 0x00, 0x00, 0x00}, // 冰雹
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 0x34, 0x18, 0x2c, 0x30, 0x24, 0x60, 0x00, 0xc0,
        0x01, 0x80, 0x03, 0x00, 0x06, 0x18, 0x0c, 0x18, 0x18, 0x3c, 0x10, 0x2c, 0x00, 0x00, 0x00, 0x00}, // 未知
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x04, 0x00, 0x7c, 0x00, 0x78, 0x0f, 0x7f, 0x1f, 0xb8,
        0x3f, 0x88, 0x7f, 0xf0, 0xff, 0xf8, 0xff, 0xf8, 0xff, 0xf8, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00}, // 多云
    {
        0x00, 0x00, 0x00, 0x10, 0x00, 0x04, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x3f, 0xc2, 0x00, 0x04,
        0x0f, 0xf0, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x40, 0x00, 0x00}, // 风
    {
        0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0x10, 0x08, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x0f, 0xf0,
        0x0f, 0xf0, 0x0f, 0xf0, 0x07, 0xe0, 0x03, 0xc0, 0x10, 0x08, 0x20, 0x04, 0x01, 0x00, 0x00, 0x00}, // 晴
    {
        0x00, 0x00, 0x01, 0x00, 0x03, 0x80, 0x03, 0x80, 0x07, 0xc0, 0x07, 0xc0, 0x0f, 0xe0, 0x0f, 0xe0,
        0x0f, 0xf0, 0x1f, 0xf0, 0x1f, 0xf8, 0x3f, 0xf8, 0x3c, 0x7c, 0x70, 0x1c, 0x60, 0x06, 0x00, 0x00}, // 风力
    {
        0xc0, 0x00, 0x70, 0x00, 0x7c, 0x0e, 0x7f, 0xfe, 0x78, 0x4c, 0x60, 0x40, 0x00, 0x40, 0x00, 0x40,
        0x00, 0xc0, 0x00, 0xe0, 0x00, 0xe0, 0x00, 0xe0, 0x00, 0xe0, 0x00, 0xe0, 0x00, 0xe0, 0x00, 0xc0}, // 风向
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x78, 0x00, 0xd0, 0x01, 0xa0, 0x03, 0x40,
        0x0e, 0x80, 0x1d, 0x10, 0x12, 0x38, 0x1e, 0x28, 0x0c, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 湿度
    {
        0x03, 0xc0, 0x03, 0xc0, 0x06, 0x60, 0x06, 0x60, 0x06, 0x60, 0x06, 0x60, 0x06, 0x60, 0x06, 0x60,
        0x0c, 0x20, 0x0d, 0xb0, 0x19, 0x90, 0x1b, 0xd8, 0x19, 0x98, 0x0c, 0x30, 0x07, 0xe0, 0x03, 0xc0}, // 温度
    {
        0x00, 0x00, 0x01, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x07, 0x60, 0x0f, 0x70, 0x0f, 0x70, 0x1f, 0x78,
        0x1f, 0x78, 0x3f, 0x7c, 0x7f, 0xfe, 0x7f, 0xfe, 0xff, 0x7f, 0xff, 0xff, 0x7f, 0xff, 0x00, 0x00}, // 警告
    {
        0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x0a, 0xc0, 0x00, 0x70, 0x00, 0x0e,
        0x0a, 0x06, 0x00, 0xe0, 0x00, 0xc0, 0x0e, 0xe0, 0x0a, 0xc0, 0x0a, 0x00, 0x7f, 0xfe, 0x00, 0x00 // 城市
    }

};

const String Chinese_index[CHINESEARRLEN] = {"晴", "云", "风", "阴", "雨", "雾", "霾", "雷", "冰", "雪", "尘", "特", "大", "中", "小", "静", "暴", "夹",
                                             "东", "南", "西", "北", "无", "不", "定", "多", ""};

const String Symbol_index[3] = {"℃", "℉", "≤"};

IPAddress ip;
WiFiUDP ntpUDP;
NTPClient tc(ntpUDP, NTP_SERVERS[1], ZONE, 6000);

WiFiClient wc;
PubSubClient pc(wc);
// PubSubClient pc_dht(wc);

// 时钟
ThreeWire Wiret(DS_DATA, DS_CLK, DS_RST);
RtcDS1302<ThreeWire> Rtc(Wiret);
RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

// TM1637Display tmdisplay(TM1637_CLK, TM1637_DIO);

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

boolean RELAY_STATUS[6];
boolean tempMessage = false;
int timecount = 0;
float temperature, humidity;
boolean warnFlag = false;

// 天气
uint8_t pageflag = 0;
uint8_t weather_textlen = 0;
int Weather_updateCount;
bool WeatherFlag = false;
uint8_t weather_iconindex;
uint8_t weather_statusindexarr[WEATHERTEXTLEN], weather_directionarr[4];

String Weather_Status;
String Weather_Temperature;
String Weather_Humidity;
String Weather_WindPower;
String Weather_WindDirection;
String Weather_ReportTime;

String RequestURL = String(API_REQUESTURL) + "city=" + String(CITY_CODE) + "&key=" + String(API_KEY);

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
  Serial.println("Topic: " + String(topic));
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

// void Wifi_connect() {
//   int count = 0;
//   WiFi.mode(WIFI_STA);
//   WiFi.disconnect();
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//   Serial.println("Connect to: \n\tSSID:" + String(WIFI_SSID) + "\n\tPassWord:" + String(WIFI_PASSWORD));
//   while (WiFi.status() != WL_CONNECTED) {
//     Serial.print('.');
//     delay(1000);
//     count++;
//   }
//   MQTT_connect();
//   Serial.println("\nConnect to " + String(WIFI_SSID) + " Successful\n");
//   Serial.println("\nMAC: " + WiFi.macAddress() + "\nIP: " + WiFi.localIP().toString() + "\nUsing Time:" + count + "s");
//   NtpUpdate();
//   LED_Flash(20);
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

  Serial.println("\nConnect to " + String(WIFI_SSID) + " Successful\n");
  Serial.println("\nMAC: " + WiFi.macAddress() + "\nIP: " + WiFi.localIP().toString() + "\nUsing Time:" + count + "s");
  display.setCursor(0, 10);
  display.printf("IP: %s \n ", WiFi.localIP().toString().c_str()); // lol
  display.setCursor(0, 20);
  display.printf("Using: %d S", count);
  display.display();
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

/*天气相关修改START*/
void displayDrawCharacter(int x, int y, const unsigned char *data)
{
  for (int i = 0; i < 16; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      if (data[i * 2] & (0x80 >> j))
      {
        display.drawPixel(x + j, y + i, WHITE);
      }
    }
    for (int j = 0; j < 8; j++)
    {
      if (data[i * 2 + 1] & (0x80 >> j))
      {
        display.drawPixel(x + j + 8, y + i, WHITE);
      }
    }
  }
}

// 目标字符串，映射数组，映射数组长度，中文是否，索引数组，索引长度
uint8_t getMapArr_index(String ch, const String arr[], uint8_t arrSize, bool ischinese, uint8_t *indices, uint8_t maxIndices)
{
  uint8_t foundCount = 0;
  int i = 0;

  while (i < ch.length() && foundCount < maxIndices)
  {
    // 提取当前字符（处理UTF-8多字节字符）
    char c = ch.charAt(i);
    int charBytes = 1; // 中文占位

    if ((c & 0xF8) == 0xF0)
      charBytes = 4;
    else if ((c & 0xF0) == 0xE0)
      charBytes = 3;
    else if ((c & 0xE0) == 0xC0)
      charBytes = 2;

    // 提取完整的字符
    String currentChar = "";
    for (int j = 0; j < charBytes && i + j < ch.length(); j++)
    {
      currentChar += ch.charAt(i + j);
    }
    // Serial.println("当前提取：");
    // Serial.println(currentChar);

    bool found = false;
    for (uint8_t idx = 0; idx < arrSize; idx++)
    {
      if (arr[idx] == currentChar)
      {
        indices[foundCount++] = idx;
        found = true;
        break;
      }
    }

    if (!found)
    {
      // Serial.print("UnFound: ");
      // Serial.println(currentChar);
    }

    // 移动到下一个字符
    i += charBytes;
  }

  return foundCount;
}

void getAPIData(String url)
{
  Serial.println(url);
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    // 关键：使用WiFiClientSecure和HTTPS URL初始化
    Serial.println("http begin");
    http.begin(wc, url);
    Serial.println("Get api");
    int Rcode = http.GET(); // get方式 修改http非ssl

    if (Rcode > 0)
    {
      Serial.println("Status code: " + String(Rcode));
      if (Rcode = HTTP_CODE_OK)
      { // 200
        Serial.printf("Code: %d Get Weather Successfully\n", Rcode);
        String payload = http.getString();
        Serial.println(payload);
        getWeatherInfofromJson(payload);
      }
      else
      {
        Serial.printf("err %s\n", http.errorToString(Rcode).c_str());
      }
      http.end();
    }
    else
    {
      // Serial.println("Fuck");
      Serial.println(Rcode);
      Serial.println(http.getString());
    }
  }
}

void getWeatherInfofromJson(String returndata)
{
  const size_t capacity = 9 * JSON_ARRAY_SIZE(9) + 9 * JSON_OBJECT_SIZE(9) + 64;
  // const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(12) + 1024;
  // 最多九个数组，每个数组9个元素
  // 如果九个对象，每个对象包含一个键值
  // 64预留
  DynamicJsonDocument doc(capacity); // 从堆中分配
  deserializeJson(doc, returndata);  // 解析
  JsonObject obj = doc["lives"][0];
  Weather_Status = obj["weather"].as<String>();
  Weather_Temperature = obj["temperature"].as<String>();
  Weather_WindDirection = obj["winddirection"].as<String>();
  Weather_WindPower = obj["windpower"].as<String>();
  Weather_Humidity = obj["humidity"].as<String>();
  Weather_ReportTime = obj["reporttime"].as<String>();

  // 此处获取文字与图片索引
  for (uint8_t i = 0; i < WEATHERTEXTLEN; i++)
  {
    uint8_t count = getMapArr_index(Weather_Status, Chinese_index, CHINESEARRLEN, Chinese_Arr, weather_statusindexarr, WEATHERTEXTLEN);
    if (count > CHINESEARRLEN)
    {
      weather_statusindexarr[i] = 255; // 错误位
    }
  }

  for (uint8_t i = 0; i < 4; i++)
  {
    uint8_t count = getMapArr_index(Weather_WindDirection, Chinese_index, CHINESEARRLEN, Chinese_Arr, weather_directionarr, 4);
    if (count > 4)
    {
      weather_directionarr[i] = 255; // 错误位
    }
  }

  WeatherFlag = true;
}

void RelayStatus()
{
  String status = "";
  for (int i = 0; i < RELAY_LEN; i++)
  {
    status += String("Relay" + String(i + 1) + "-Device:" + (RELAY_STATUS[i] ? "NO" : "NC") + " Code:" + String(RELAY_STATUS[i]) + " " + ((i > 2) ? "EX\n" : "IN\n"));
  }
  LED_Flash(30);
  Serial.println(status);
  pc.publish(MQTT_TOPIC, status.c_str());
}

/*天气相关修改END*/
// NO 1 NC 0
// Normally Open&Normally Close
void Relay_NO_swich(int sw, uint8_t pos)
{
  Serial.println("sw Submit: " + String(sw));

  switch (pos)
  {
  case 0:
    for (uint8_t i = 0; i < 6; i++)
      RELAY_STATUS[i] = sw;
    break; // 全部修改
  case 1:
    RELAY_STATUS[0] = sw ? true : false;
    // digitalWrite(Relay1, !sw);
    Serial.println("Option : " + String(pos));
    break;
  case 2:
    RELAY_STATUS[1] = sw ? true : false;
    // digitalWrite(Relay2, !sw);
    Serial.println("Option : " + String(pos));
    break;
  case 3:
    // digitalWrite(Relay3, !sw);
    RELAY_STATUS[2] = sw ? true : false;
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
  setRelayStatus(pos, sw); // 向eeprom写入
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

  // 同步
  sync_relaystatusRtoE();
}

void WriteData(int sw, uint8_t pos)
{
  EEPROM.write(pos, sw);
  EEPROM.commit();
  // Serial.println("Write: " + String(sw) + " Read: " + String(EEPROM.read(0)));
}

/*获取状态和写入状态*/

void setRelayStatus(uint8_t status, uint8_t pos)
{
  // RELAY_STATUS[pos-1] = status;
  WriteData(pos - 1, status);
}

void getRelayStatus()
{
  for (u8_t i = 0; i < RELAY_LEN; i++)
  {
    RELAY_STATUS[i] = ReadData(i);
  }
}

/*基本读写*/

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
  Serial.printf("NTP-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n", year(), month(), day(), hour(), minute(), second());
  Rtc.SetDateTime(current);
  RtcDateTime now = Rtc.GetDateTime();
  Serial.printf("Rtc-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n", now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second());
}

/*查表*/
void setRelay(uint8_t pos, boolean status)
{
  switch (pos)
  {
  case 1:
    digitalWrite(Relay1, !status);
    break;
  case 2:
    digitalWrite(Relay2, !status);
    break;
  case 3:
    digitalWrite(Relay3, !status);
    break;
  default:
    RELAY_STATUS[pos - 1, status];
    break; // 外定义继电器
  }
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

/*继电器同步*/
void sync_relaystatusEtoR()
{
  for (uint8_t i = 0; i < RELAY_LEN; i++)
  {
    RELAY_STATUS[i] = ReadData(i);
  }
}

void sync_relaystatusRtoE()
{
  for (uint8_t i = 0; i < RELAY_LEN; i++)
  {
    WriteData(i, RELAY_STATUS[i]);
  }
}

void sync_relaystatusRtoIO()
{
  for (uint8_t i = 0; i < RELAY_LEN; i++)
  {
    setRelay(i + 1, RELAY_STATUS[i]);
  }
}

/*字符处理*/

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

void initialization()
{

  getRelayStatus();

  pinMode(Relay1, OUTPUT);
  pinMode(Relay2, OUTPUT);
  pinMode(Relay3, OUTPUT);
  pinMode(LED, OUTPUT);

  // digitalWrite(Relay1, ReadData(1));
  // digitalWrite(Relay2, ReadData(2));
  // digitalWrite(Relay3, ReadData(3));

  digitalWrite(LED, !(RelayStatus[0] || RELAY_STATUS[1] || RELAY_STATUS[2]));

  // 天气
  Wire.begin(SDA, SCL);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  //
  display.setTextColor(WHITE);
  display.setTextSize(1);

  // Oclock
  tc.begin();
  // tmdisplay.setBrightness(0x0f);
  Rtc.Begin();
  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);

  sync_relaystatusEtoR();
}

////////////////////////////////////////////////////////////
void setup()
{
  EEPROM.begin(16);
  Serial.begin(9600);
  initialization();
  // MQTT
  pc.setServer(MQTT_SERVER_IP, MQTT_PORT);
  //
  display.setCursor(0, 0);
  display.printf("Connect to: %s", WIFI_SSID);

  display.display();
  //
  Wifi_connect();
  pc.connect(WiFi.macAddress().c_str());
  display.setCursor(0, 30);
  display.printf("Exiting:");
  display.setCursor(25, 40);
  for (uint8_t i = 6; i > 0; i--)
  {
    display.printf("%d ", i);

    display.display();
    delay(1000);
    // display.clearDisplay();
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(25, 20);
  display.print("Booting.....");
  display.display();
  tone(Buzzer, 456);
  delay(200);
  tone(Buzzer, 256);
  delay(200);
  noTone(Buzzer);
  tone(Buzzer, 500);
  delay(200);
  tone(Buzzer, 256);
  delay(200);
  noTone(Buzzer);
  getAPIData(RequestURL);
  NtpUpdate();
}

void loop()
{
  /*天气相关模块*/

  // 更新与计时器
  timecount++;
  Serial.printf("\nTimeCount:%d\n", timecount);
  if (timecount % 5 == 0)
  {

    pageflag++;
    if (pageflag > 3)
      pageflag = 1;
  }

  if (timecount >= 3600 * WEATHERUPDATEHOURSDELAY)
  {
    timecount = 0;
    getAPIData(RequestURL);
  }
  // display

  if (pageflag == 1 && WeatherFlag)
  {
    display.clearDisplay();
    display.setCursor(3, 3);
    display.printf("%s", Weather_ReportTime.c_str());
    displayDrawCharacter(2, 15, Weather_IconArr[10]); // 天气
    display.setCursor(22, 20);
    display.printf("Weather:");
    for (uint8_t i = 0; i < WEATHERTEXTLEN; i++)
    {
      // if(weather_statusindexarr[i]=255)break;//跳出错误存储
      if (weather_statusindexarr[i] < CHINESEARRLEN)
      {
        if (weather_statusindexarr[i] == weather_statusindexarr[i + 1])
          i++;                                                                           // 在这里跳过重复查找索引
        displayDrawCharacter(70 + (i * 18), 15, Chinese_Arr[weather_statusindexarr[i]]); // 查询
      }
    }

    displayDrawCharacter(2, 33, Weather_IconArr[16]); // 温度
    display.setCursor(22, 35);
    display.printf("Temper: %s", Weather_Temperature);
    displayDrawCharacter(90, 30, Symbol_Arr[0]); // 摄氏度

    displayDrawCharacter(2, 48, Weather_IconArr[15]); // 湿度
    display.setCursor(22, 55);
    display.printf("Humidity: %s %%", Weather_Humidity);
    display.fillRect(123, 0, 5, (timecount % 5) * 12.8, WHITE); // 显示低于预期
  }
  /*Page 2*/
  if (pageflag == 2 && WeatherFlag)
  {
    display.clearDisplay();
    display.setCursor(3, 3);
    display.printf("%s", Weather_ReportTime.c_str());
    displayDrawCharacter(2, 15, Weather_IconArr[13]); // 风力
    display.setCursor(22, 20);
    String WindPower = "";
    for (uint8_t i = 0; i < Weather_WindPower.length(); i++)
    {
      if (Weather_WindPower.charAt(i) >= '0' && Weather_WindPower.charAt(i) <= '9')
      {
        WindPower += Weather_WindPower.charAt(i);
      }
    }
    // display.print("WindPower:"+(WindPower.charAt(0)<'3')?"<=");
    if (WindPower.charAt(0) <= '3')
    {
      // displayDrawCharacter(75, 15, Symbol_Arr[2]); //小于等于 //但是过大影响观感
      display.printf("WindPower: <=%s", WindPower);
    }
    else
    {
      display.printf("WindPower: %s", WindPower);
    }

    displayDrawCharacter(2, 33, Weather_IconArr[14]); // 风向
    display.setCursor(22, 38);
    display.printf("Wind.D:");
    for (uint8_t i = 0; i < 4; i++)
    {
      // if(weather_directionarr[i]=255)break;//跳出错误存储
      if (weather_directionarr[i] < CHINESEARRLEN)
      {
        if (weather_directionarr[i] == weather_directionarr[i + 1])
          i++;                                                                         // 在这里跳过重复查找索引
        displayDrawCharacter(65 + (i * 18), 33, Chinese_Arr[weather_directionarr[i]]); // 查询
      }
    }

    displayDrawCharacter(2, 48, Weather_IconArr[18]); // CITY
    display.setCursor(22, 55);
    display.printf("CityCode: %s", CITY_CODE);
    display.fillRect(123, 0, 5, (timecount % 5) * 12.8, WHITE);
  }

  if (pageflag == 3)
  {
    display.clearDisplay();
    // display.setCursor(3, 3);
    // display.printf("RelayStatus");

    for (uint8_t i = 0; i < 6; i++)
    {
      display.setCursor(3, 3 + (i * 10));
      display.printf("Relay%d W:%s %d %s", i + 1, RELAY_STATUS[i] ? "NO" : "NC", RELAY_STATUS[i] ? 1 : 0, (i + 1 > 3) ? "EX" : ((i + 1 == 3) ? "UNUSED" : "IN"));
    }
    display.fillRect(123, 0, 5, (timecount % 5) * 12.8, WHITE);
  }

  if (!WeatherFlag)
  {
    displayDrawCharacter(2, 25, Weather_IconArr[17]); // 无天气数据
    display.setCursor(25, 28);
    display.print("No Weather Data!");
  }
  display.display();

  /*旧模块*/

  if (timecount % 100 == 0)
  {
    pc.publish(DHT11_TOPIC, "~GET_TEMP");
    pc.publish(DHT11_TOPIC, "~GET_HUM");
  }

  sync_relaystatusRtoIO();

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

  // 同步状态
  for (uint8_t i = 0; i < RELAY_LEN; i++)
  {
  }

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
  Serial.printf("%d,%d,%d,%d,%d,%d", RELAY_STATUS[0], RELAY_STATUS[1], RELAY_STATUS[2], RELAY_STATUS[3], RELAY_STATUS[4], RELAY_STATUS[5]);

  delay(1000);
}

///////////////////////////////////////////////////////////
