#include <Wire.h>
#include <string.h>
#include <RtcDS1302.h>
#include <ThreeWire.h>
#include <Adafruit_SSD1306.h>

#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32

// DEFAULT
//  SDA: GPIO 4 (D2)
//  SCL: GPIO 5 (D1)

#define DS_CLK D0
#define DS_DATA D1
#define DS_RST D2

#define SCL D5
#define SDA D6

ThreeWire tWire(DS_DATA, DS_CLK, DS_RST);

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

RtcDS1302<ThreeWire> Rtc(tWire);

void setup()
{
    pinMode(SCK, OUTPUT);
    pinMode(SDA, OUTPUT);
    Wire.begin(SDA, SCL);
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    display.clearDisplay();
    display.setTextColor(WHITE);
    Serial.begin(9600);

    Rtc.Begin();

    Serial.println(__DATE__);
    Serial.println(__TIME__);

    // INIT Time
    // If u want use compiled time
    //  RtcDateTime now = Rtc.GetDateTime();
    //  if(Rtc.GetIsRunning()&&(Rtc.GetDateTime()<now)){
    //      if (Rtc.GetIsWriteProtected()) {
    //        Rtc.SetIsWriteProtected(false);
    //        SetDs1302Date(compiled);
    //      }else{
    //        SetDs1302Date(compiled);
    //      }

    // }else{
    //   Rtc.SetIsRunning(true);
    //   SetDs1302Date(compiled);
    // }
}

double i = 0;

void loop()
{

    display.clearDisplay();
    display.display();

    RtcDateTime now = Rtc.GetDateTime();
    Serial.printf("Rtc-Get: y:%d m:%d d:%d h:%d m:%d S:%d\n", now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second());
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.printf("Date: %d / %d / %d", now.Year(), now.Month(), now.Day());
    display.setTextSize(2);
    display.setCursor(10, 15);
    display.printf("%d:%d  %d", now.Hour(), now.Minute(), now.Second());
    display.display();

    delay(1000);
}
