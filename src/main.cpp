#include <Arduino.h>

// ==========================================
// 1. 引脚物理定义 (C 部分：巡线传感器)
// ==========================================
const int PIN_L = 34; // 左传感器 DO
const int PIN_M = 13; // 中传感器 DO 
const int PIN_R = 12; // 右传感器 DO 

// ==========================================
// 2. 引脚物理定义 (A 部分：四驱电机控制引脚)
// ==========================================
// 前左 FL
const int FL_A = 27;
const int FL_B = 26;
// 后左 RL
const int RL_A = 4;
const int RL_B = 5;
// 前右 FR
const int FR_A = 16;
const int FR_B = 17;
// 后右 RR
const int RR_A = 32;
const int RR_B = 33;

// ==========================================
// 3. PWM 通道映射 (旧版 ESP32 库必须手动绑定通道 0~7)
// ==========================================
const int CH_FL_A = 0;
const int CH_FL_B = 1;
const int CH_RL_A = 2;
const int CH_RL_B = 3;
const int CH_FR_A = 4;
const int CH_FR_B = 5;
const int CH_RR_A = 6;
const int CH_RR_B = 7;

const int PWM_FREQ = 5000;  // 5 kHz 频率
const int PWM_RES = 8;      // 8位分辨率 (0-255)

// ==========================================
// 4. 速度控制与巡线状态记忆
// ==========================================
const int BASE_SPEED = 200;    // 直线基础速度 (0~255)
const int TURN_SPEED_HI = 180; // 转弯时，外侧轮高速
const int TURN_SPEED_LO = 50;  // 转弯时，内侧轮低速

// 状态记忆：0=未知，1=曾偏右（需向左修正），2=曾偏左（需向右修正）
int last_state = 0; 

// ==========================================
// 5. 电机底层驱动函数 (基于通道控制)
// ==========================================
void setMotor(int chA, int chB, int speed) {
  if (speed >= 0) {
    ledcWrite(chA, speed); // 正转，通道 A 输出 PWM，通道 B 为 0
    ledcWrite(chB, 0);
  } else {
    ledcWrite(chA, 0);
    ledcWrite(chB, -speed); // 反转，通道 A 为 0，通道 B 输出 PWM
  }
}

// 四驱差速控制：传入左侧、右侧轮子的目标速度
void drive(int leftSpeed, int rightSpeed) {
  setMotor(CH_FL_A, CH_FL_B, leftSpeed);  // 前左
  setMotor(CH_RL_A, CH_RL_B, leftSpeed);  // 后左
  setMotor(CH_FR_A, CH_FR_B, rightSpeed); // 前右
  setMotor(CH_RR_A, CH_RR_B, rightSpeed); // 后右
}

// ==========================================
// 6. 系统初始化
// ==========================================
void setup() {
  Serial.begin(115200);

  // 初始化传感器
  pinMode(PIN_L, INPUT);
  pinMode(PIN_M, INPUT);
  pinMode(PIN_R, INPUT);

  // 初始化 ESP32 旧版 PWM 硬件定时器和引脚绑定
  ledcSetup(CH_FL_A, PWM_FREQ, PWM_RES); ledcAttachPin(FL_A, CH_FL_A);
  ledcSetup(CH_FL_B, PWM_FREQ, PWM_RES); ledcAttachPin(FL_B, CH_FL_B);

  ledcSetup(CH_RL_A, PWM_FREQ, PWM_RES); ledcAttachPin(RL_A, CH_RL_A);
  ledcSetup(CH_RL_B, PWM_FREQ, PWM_RES); ledcAttachPin(RL_B, CH_RL_B);

  ledcSetup(CH_FR_A, PWM_FREQ, PWM_RES); ledcAttachPin(FR_A, CH_FR_A);
  ledcSetup(CH_FR_B, PWM_FREQ, PWM_RES); ledcAttachPin(FR_B, CH_FR_B);

  ledcSetup(CH_RR_A, PWM_FREQ, PWM_RES); ledcAttachPin(RR_A, CH_RR_A);
  ledcSetup(CH_RR_B, PWM_FREQ, PWM_RES); ledcAttachPin(RR_B, CH_RR_B);

  Serial.println("4WD Line Follower Initialized (V2 API)!");
}

// ==========================================
// 7. 主循环巡线逻辑
// ==========================================
void loop() {
  // 读取传感器
  bool L = digitalRead(PIN_L);
  bool M = digitalRead(PIN_M);
  bool R = digitalRead(PIN_R);

  // 状态机处理：假设传感器读取到黑线为 1，白纸为 0
  if (!L && M && !R) {
    // 0 1 0 : 正中，四轮全速前进
    drive(BASE_SPEED, BASE_SPEED);
    last_state = 0; 
  } 
  else if ((L && M && !R) || (L && !M && !R)) {
    // 1 1 0 或 1 0 0 : 偏右了，需要向左拉
    drive(TURN_SPEED_LO, TURN_SPEED_HI);
    last_state = 1; 
  } 
  else if ((!L && M && R) || (!L && !M && R)) {
    // 0 1 1 或 0 0 1 : 偏左了，需要向右拉
    drive(TURN_SPEED_HI, TURN_SPEED_LO);
    last_state = 2; 
  } 
  else if (!L && !M && !R) {
    // 0 0 0 : 三个都悬空白纸，利用上一次的记忆往回拽
    if (last_state == 1) {
      drive(TURN_SPEED_LO - 20, TURN_SPEED_HI); 
    } else if (last_state == 2) {
      drive(TURN_SPEED_HI, TURN_SPEED_LO - 20); 
    } else {
      drive(0, 0); // 若连记忆都没有，停车防跑丢
    }
  } 
  else if (L && M && R) {
    // 1 1 1 : 踩到横线（终点或T字路口）
    drive(0, 0); 
  }

  delay(5); // 快速执行闭环计算
}