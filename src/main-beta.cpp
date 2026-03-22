#include <Arduino.h>

// --- 电机引脚 ---
#define MOTOR_FL_A 4
#define MOTOR_FL_B 5
#define MOTOR_RL_A 6
#define MOTOR_RL_B 7
#define MOTOR_FR_A 8
#define MOTOR_FR_B 9
#define MOTOR_RR_A 10
#define MOTOR_RR_B 11

// --- 超声波传感器引脚 ---
#define TRIG1 16
#define ECHO1 15
#define TRIG2 18
#define ECHO2 17

// PWM 通道定义 (0-15)
const int channel_FL_A = 0, channel_FL_B = 1;
const int channel_RL_A = 2, channel_RL_B = 3;
const int channel_FR_A = 4, channel_FR_B = 5;
const int channel_RR_A = 6, channel_RR_B = 7;

const int freq = 20000;
const int resolution = 8;

// 获取距离的函数 (单位: cm)
float getDistance(int trig, int echo) {
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    // 这里的 58.2 是根据声速计算的换算常数
    long duration = pulseIn(echo, HIGH, 30000); // 30ms 超时，防止阻塞
    if (duration == 0) return 999; // 超时返回远距离
    return duration / 58.2;
}

void setup() {
    Serial.begin(115200);

    // --- 初始化超声波引脚 ---
    pinMode(TRIG1, OUTPUT);
    pinMode(ECHO1, INPUT);

    // --- 初始化所有电机通道 ---
    for (int i = 0; i < 8; i++) {
        ledcSetup(i, freq, resolution);
    }

    // --- 绑定引脚 ---
    ledcAttachPin(MOTOR_FL_A, channel_FL_A); ledcAttachPin(MOTOR_FL_B, channel_FL_B);
    ledcAttachPin(MOTOR_RL_A, channel_RL_A); ledcAttachPin(MOTOR_RL_B, channel_RL_B);
    ledcAttachPin(MOTOR_FR_A, channel_FR_A); ledcAttachPin(MOTOR_FR_B, channel_FR_B);
    ledcAttachPin(MOTOR_RR_A, channel_RR_A); ledcAttachPin(MOTOR_RR_B, channel_RR_B);
}

void loop() {
    float dist1 = getDistance(TRIG1, ECHO1);
    float dist2 = getDistance(TRIG1, ECHO1);
    float dist3 = getDistance(TRIG1, ECHO1);
    float dist4 = getDistance(TRIG1, ECHO1);
    delay(100); // 10Hz 的采样频率，兼顾实时性与串口稳定性
}