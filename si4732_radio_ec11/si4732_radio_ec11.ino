#include <SI4735.h>
#include <RobustQuadrature.h>

#define EC11_A D0
#define EC11_B D1
#define EC11_EN D2

#define SI4732_SDA D4
#define SI4732_SCL D5
#define SI4732_RST D6

#define LCD1602_SDA D4
#define LCD1602_SCL D5

#define AM_FUNCTION 1
#define FM_FUNCTION 0


uint8_t encoderMode; //0-波段选择 1：10kHz单位调节  2：1mHz单位调节 3：音量调节 
uint8_t BandFlag =0 ; //0-FM 1-SW 2-AM 3-LW
uint16_t currentFrequency;
uint16_t previousFrequency;
uint16_t currentSetFrequency;
int EC11_post_before;

/*
lcd显示
-波段修改
xxx.xx xHz Band - 15占用
SNR:xdb Sig:xx - 14占用

-处于频率模式
xxx.xx xHz  Frq - 15占用
SNR:xdb Sig:xx - 14占用

-音量调节
Vol:xxx%    Vol - 12占用
SNR:xdb Sig:xx - 14占用

*/

SI4735 si4732;
LiquidCrystal_I2C lcd(0x27,16,2); 
RobustQuadrature::One<EC11_A, EC11_B> encoder;

void init() {
  Serial.begin(9600);

  lcd.init();  
  delay(500);
  lcd.backlight();
  lcd.setCursor(0,0);

  lcd.print("initialization...");
  Serial.println("initialization...");

  pinMode(EC11_EN, INPUT_PULLUP);

  digitalWrite(SI4732_RST, 1);
  Wire.begin(SI4732_SDA, SI4732_SCL);
  delay(300);
  int si4732Addr = si4732.getDeviceI2CAddress(SI4732_RST);
  si4732.setup(SI4732_RST, FM_FUNCTION);
  delay(300);
  si4732.setFM(6400, 10800, 9070, 10);
  delay(300);
  currentFrequency = previousFrequency = si4732.getFrequency();
  currentSetFrequency = currentFrequency;
  si4732.getStatus();
  si4732.getCurrentReceivedSignalQuality();

  // 状态
  if (si4732.isCurrentTuneFM()) {
    Serial.printf("Tuned on %s Mhz\n", String(currentFrequency / 100.0, 2))
  } else {
    Serial.printf("%s kHz", String(currentFrequency));
  }
}

// false - khz,10k true -mhz,1m
void displayFrq(float frq,bool feq_sig,bool feq_adj){
  lcd.clear();
  setCursor(0,0);
  if(feq_sig){
  lcd.printf("%3.2f kHz  %3s",frq,feq_adj ? "10K":"1Mx");
  }else{
    lcd.printf("%3.2f mHz  %3s",frq,feq_adj ? "10K":"1Mx");
  }
  
}

//true+,false -
void EC11_ADJ(bool adj){
  if(adj){
  if(encoderMode == 0) 
  if(encoderMode == 1) currentSetFrequency+=10;
  if(encoderMode == 2) currentSetFrequency+=100;
  if(encoderMode == 3) si4732.volumeUp()
  return;
  }
    if(encoderMode == 1) currentSetFrequency-=10;
  if(encoderMode == 2) currentSetFrequency-=100;
  if(encoderMode == 3) si4732.volumeDown();
}

void setup() {
  // put your setup code here, to run once:
  init();
}

void loop() {
  // put your main code here, to run repeatedly:
  int EC11_pos = encoder.position();
  if (EC11_pos > EC11_post_before) {
    // 顺时针情况
    EC11_ADJ(true);


    EC11_post_before = EC11_pos;
  } else if (EC11_pos < EC11_post_before) {
    // 逆时针
    EC11_post_before = EC11_pos;
    EC11_ADJ(false);
  }

  if(digitalRead(EC11_EN,0)){ 
    delay(10);
    if(encoderMode>=3&&digitalRead(EC11_EN,0)){
        encoderMode=0;
    }else if(digitalRead(EC11_EN,0)){
    encoderMode++;
    }
  }
  /*常处于*/
  setCursor(1,0);
  lcd.printf("SNR:%2ddB Sig:%2d",si4732.getCurrentSNR(),si4732.getCurrentRSSI());
  // 调节频段范围 显示到lcd
  // FM - 64-108mhz
  // SW 2.3 -26.1mhz
  // AM 520-1710Khz
  // LW 153-279 khz
  switch(BandFlag){
    case 0: si4732.setFM(6400, 10800, 10390, 10);
    case 1: si4732.setSSB(1730, 30000, 14270, 1,AM_FUNCTION);
    case 2: si4732.setAM(153, 279, 180, 1);
  }

}
