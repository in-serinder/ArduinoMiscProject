#include <Wire.h>
#include <Arduino.h>
#include <string.h>
#include <Adafruit_SSD1306.h>
#include <DHT11.h>

#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32

#define SCL D5
#define SDA D6

#define DHT D0
DHT11 dht11(DHT);

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

void setup()
{

    pinMode(SCK, OUTPUT);
    pinMode(SDA, OUTPUT);
    Wire.begin(SDA, SCL);
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    display.clearDisplay();
    display.setTextColor(WHITE);
    Serial.begin(9600);
}

void loop()
{

    display.clearDisplay();

    int temperature = 0;
    int humidity = 0;

    display.setTextSize(1);

    display.setCursor(0, 0);
    display.printf("DHT11:");

    int result = dht11.readTemperatureHumidity(temperature, humidity);
    if (result == 0)
    {

        display.setCursor(5, 10);
        display.printf("Temperature: %d C", temperature);
        display.setCursor(5, 20);
        Serial.printf("T:%d H: %d", temperature, humidity);
        display.printf("Humidity: %d %%", humidity);
        Serial.println("T:" + String(temperature) + " H:" + String(humidity));
    }

    delay(400);

    display.display();
}
