#include <Servo.h>

#define PWM D4
// #define Rvo D4

Servo servo;

int se=0;
bool dir = true;

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
    // se++;

    
    servo.write(se%180);
    Serial.println(se%180);
    se++;
delay(10);

}