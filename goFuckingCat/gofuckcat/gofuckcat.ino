
#define Buzzer D0
#define rayDec D2

const int minFreq = 4000; // 最低频率4kHz
const int maxFreq = 8000; // 最高频率8kHz
const int freqStep = 500; 
const int beepDuration = 50; 
const int delayStep = 60; 


void setup() {
  // put your setup code here, to run once:
  pinMode(Buzzer, OUTPUT);
   pinMode(rayDec, INPUT);
   noTone(Buzzer);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(200);
  if(digitalRead(rayDec)){
    playRepellentTone();
  }
}


void playRepellentTone() {

  for (int freq = minFreq; freq <= maxFreq; freq += freqStep) {
    tone(Buzzer, freq, beepDuration); /
    delay(delayStep);
    if (!digitalRead(rayDec)) {
      noTone(Buzzer);
      return;
    }
  }
  for (int freq = maxFreq; freq >= minFreq; freq -= freqStep) {
    tone(Buzzer, freq, beepDuration);
    delay(delayStep);
    if (!digitalRead(rayDec)) {
      noTone(Buzzer);
      return;
    }
  }
}
