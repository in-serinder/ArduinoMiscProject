/*
  ESP8266 纯软件高频波形生成（无暂停）
  持续输出20~35kHz随机扫频方波 | 仅保留核心波形功能
  100%编译通过 | 全版本兼容 | D1(GPIO5)输出
*/
#define PWM_PIN     D1          // 输出引脚D1(GPIO5)
#define MIN_FREQ    20000       // 最低20kHz
#define MAX_FREQ    35000       // 最高35kHz
#define FREQ_INTERVAL 300       // 300ms切换一次频率（防猫适应）
unsigned int usDelay = 16;      // 单电平延时微秒

// 计算对应频率的微秒延时（50%占空比）
void calcUsDelay(int freq) {
  usDelay = 1000000 / (2 * freq);
}

void setup() {
  pinMode(PWM_PIN, OUTPUT);
  Serial.begin(115200);
  randomSeed(analogRead(A0));
  // 初始频率30kHz
  calcUsDelay(30000);
  Serial.println("✅ 波形生成启动！持续输出20~35kHz随机扫频");
}

void loop() {
  static unsigned long lastFreqTime = 0;
  // 每300ms随机切换频率（核心扫频逻辑）
  if (millis() - lastFreqTime >= FREQ_INTERVAL) {
    int currentFreq = random(MIN_FREQ, MAX_FREQ);
    calcUsDelay(currentFreq);
    lastFreqTime = millis();
    Serial.print("当前频率："); Serial.print(currentFreq/1000); Serial.println("kHz");
  }
  // 持续输出高频方波（核心波形生成）
  noInterrupts();
  digitalWrite(PWM_PIN, HIGH);
  delayMicroseconds(usDelay);
  digitalWrite(PWM_PIN, LOW);
  delayMicroseconds(usDelay);
  interrupts();
}