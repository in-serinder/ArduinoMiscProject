#include <ThreeWire.h>
#include <TM1637Display.h>
#include <RtcDS1302.h>
#include <string.h>

#define DS_CLK 5
#define DS_DATA 4
#define DS_RST 2

#define TM1637_CLK 0
#define TM1637_DIO 12

ThreeWire Wire(DS_DATA, DS_CLK, DS_RST);
RtcDS1302<ThreeWire> Rtc(Wire);

TM1637Display tmdisplay(TM1637_CLK, TM1637_DIO);

RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  tmdisplay.setBrightness(0x0f);
  Rtc.Begin();

  Serial.println(__DATE__);
  Serial.println(__TIME__);

  // INIT Time
  RtcDateTime now = Rtc.GetDateTime();
  if (Rtc.GetIsRunning() && (Rtc.GetDateTime() < now))
  {
    if (Rtc.GetIsWriteProtected())
    {
      Rtc.SetIsWriteProtected(false);
      SetDs1302Date(compiled);
    }
    else
    {
      SetDs1302Date(compiled);
    }
  }
  else
  {
    Rtc.SetIsRunning(true);
    SetDs1302Date(compiled);
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  RtcDateTime now = Rtc.GetDateTime();
  // uint16_t time = now.Hour()*100 + now.Minute();
  // tmdisplay.showNumberDecEx(time, 0b00000010);
  // // tmdisplay.showNumberDec()
  // Serial.println(time);
  // delay(1000);

  // tmdisplay.showNumberDecEx(time, 0b00001000);
  Serial.println("Date:" + String(now.Hour()) + String(now.Minute()));
  show_clock(now.Hour(), now.Minute(), true);
  delay(1000);
  show_clock(now.Hour(), now.Minute(), false);
  delay(1000);
}

void SetDs1302Date(const RtcDateTime &dt)
{
  Rtc.SetDateTime(dt);
  Serial.println("Set Rtc Time");
}

String printDateTime(const RtcDateTime &dt)
{
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second());
  return String(datestring);
}

// By @scottchiefbaker
void show_clock(uint8_t hours, uint8_t minutes, bool Showdot)
{
  uint8_t d1 = hours / 10;   // Tens digit of hours
  uint8_t d2 = hours % 10;   // Ones digit of hours
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
  if (Showdot)
  {
    digits[1] |= 128;
  }
  else
  {
    digits[1] &= ~128;
  }

  // No leading zero on times like 1:34
  if (!d1)
  {
    digits[0] = 0; // Off
  }

  tmdisplay.setSegments(digits, 4, 0);
}