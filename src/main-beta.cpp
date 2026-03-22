#include <Arduino.h>

// --- 电机引脚定义 ---
#define MOTOR_FL_A 4
#define MOTOR_FL_B 5
#define MOTOR_RL_A 6
#define MOTOR_RL_B 7
#define MOTOR_FR_A 8
#define MOTOR_FR_B 9
#define MOTOR_RR_A 10
#define MOTOR_RR_B 11

// --- 超声波传感器引脚定义 ---
#define TRIG1 16
#define ECHO1 15
#define TRIG2 18
#define ECHO2 17

// --- PWM 通道分配 (老版本 ESP32 需要手动分配 0-15 通道) ---
const int ch_FL_A = 0; const int ch_FL_B = 1;
const int ch_RL_A = 2; const int ch_RL_B = 3;
const int ch_FR_A = 4; const int ch_FR_B = 5;
const int ch_RR_A = 6; const int ch_RR_B = 7;

const int freq = 20000;    // PWM 频率 20kHz
const int resolution = 8;  // 8位分辨率 (0-255)

// --- 无阻塞定时读取超声波参数 ---
unsigned long lastCheckTime = 0;
const int checkInterval = 50; // 每 50ms (20Hz) 采样一次
int currentSensor = 1;       // 轮流读取传感器

float dist1 = 999.0; // 传感器1距离
float dist2 = 999.0; // 传感器2距离

// --- 测距函数：限制最大量程为 100cm，拒绝无谓的死等 ---
float getDistanceEfficient(int trig, int echo) {
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    
    // 超时设为 5800us (约等于 100cm)。超过1米不算，防止卡死
    long duration = pulseIn(echo, HIGH, 5800); 
    if (duration == 0) return 999.0; 
    return duration / 58.2;
}

// --- 电机单轮控制抽象函数 ---
void setMotor(int ch_A, int ch_B, int speed) {
    if (speed > 0) { // 正转
        ledcWrite(ch_A, speed);
        ledcWrite(ch_B, 0);
    } else if (speed < 0) { // 反转
        ledcWrite(ch_A, 0);
        ledcWrite(ch_B, -speed);
    } else { // 停止
        ledcWrite(ch_A, 0);
        ledcWrite(ch_B, 0);
    }
}

// --- 4WD 底盘控制函数（普通直行/转弯） ---
void moveCar(int speedFL, int speedRL, int speedFR, int speedRR) {
    setMotor(ch_FL_A, ch_FL_B, speedFL);
    setMotor(ch_RL_A, ch_RL_B, speedRL);
    setMotor(ch_FR_A, ch_FR_B, speedFR);
    setMotor(ch_RR_A, ch_RR_B, speedRR);
}

void setup() {
    Serial.begin(115200);

    // --- 1. 初始化超声波引脚 ---
    pinMode(TRIG1, OUTPUT);
    pinMode(ECHO1, INPUT);
    pinMode(TRIG2, OUTPUT);
    pinMode(ECHO2, INPUT);

    // --- 2. 配置 ESP32 PWM 通道 (v2.x 旧版本标准写法) ---
    for (int i = 0; i < 8; i++) {
        ledcSetup(i, freq, resolution);
    }

    // --- 3. 绑定物理引脚到 PWM 通道 ---
    ledcAttachPin(MOTOR_FL_A, ch_FL_A); ledcAttachPin(MOTOR_FL_B, ch_FL_B);
    ledcAttachPin(MOTOR_RL_A, ch_RL_A); ledcAttachPin(MOTOR_RL_B, ch_RL_B);
    ledcAttachPin(MOTOR_FR_A, ch_FR_A); ledcAttachPin(MOTOR_FR_B, ch_FR_B);
    ledcAttachPin(MOTOR_RR_A, ch_RR_A); ledcAttachPin(MOTOR_RR_B, ch_RR_B);
    
    Serial.println("系统初始化完毕，开始运行！");
}

void loop() {
    unsigned long currentMillis = millis();

    // --- 状态机：轮流、无阻塞读取超声波，规避互相串扰 ---
    if (currentMillis - lastCheckTime >= checkInterval) {
        lastCheckTime = currentMillis;

        if (currentSensor == 1) {
            dist1 = getDistanceEfficient(TRIG1, ECHO1);
            currentSensor = 2; 
        } else {
            dist2 = getDistanceEfficient(TRIG2, ECHO2);
            currentSensor = 1; 
        }

        // 打印传感器数据查看状态
        Serial.print("D1 (Front): "); Serial.print(dist1);
        Serial.print(" cm | D2 (Back): "); Serial.print(dist2);
        Serial.println(" cm");
    }
\
    // --- 底盘运动控制（全无阻塞） ---
    if (dist1 < 25.0) { 
        // 前方障碍物小于25cm，全员停车倒退
        moveCar(-150, -150, -150, -150); 
    } else {
        // 前方安全，平稳直行（8位精度的PWM速度最高255，给个180够用了）
        moveCar(180, 180, 180, 180); 
    }
}