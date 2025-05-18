#include <Servo.h>

#define PWM D4
#define Rvo D3

Servo servo;

void setup()
{
    // put your setup code here, to run once:
    servo.attach(PWM);
    Serial.begin(9600);
}

void loop()
{
    // put your main code here, to run repeatedly:
    // servo.write(0);
    // delay(2000);

    // servo.write(90);
    // delay(2000);

    // servo.write(180);
    // delay(2000);
    servo.write((uint8_t)analogRead(Rvo));
    Serial.println((uint8_t)analogRead(Rvo));
}
