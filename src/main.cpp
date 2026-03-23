#include <Arduino.h>

// ==========================================
// 1. 终极四驱引脚矩阵 (FR 锁定 15/2)
// ==========================================

#define FL_A 27
#define FL_B 26
#define RL_A 4
#define RL_B 5
#define FR_A 15 // 右前 A
#define FR_B 2  // 右前 B (自带蓝色 LED)
#define RR_A 32
#define RR_B 33

#define TRIG1 18 // 前
#define ECHO1 19
#define TRIG2 12 // 左
#define ECHO2 13
#define TRIG3 22 // 右
#define ECHO3 23

#define LINE_L 34 // D0 接 34 (用 ADC 读)
#define LINE_M 21 // D0 接 21 (数字读取)
#define LINE_R 35 // D0 接 35 (用 ADC 读)

// ==========================================
// 2. 配置与倒计时
// ==========================================
const int freq = 20000;
const int resolution = 8;

unsigned long noLineStartTime = 0; 
bool forceUltrasoundMode = false;  

// 🎯 D0 信号的判定阈值。高于 2000 判定为高电平，低于 2000 为低电平。
const int D0_THRESHOLD = 2000; 

int lastDirection = 0;

// ==========================================
// 3. 全时四驱底层函数
// ==========================================

float getDistance(int trig, int echo) {
    digitalWrite(trig, LOW); delayMicroseconds(2);
    digitalWrite(trig, HIGH); delayMicroseconds(10);
    digitalWrite(trig, LOW);
    long duration = pulseIn(echo, HIGH, 20000); 
    if (duration == 0) return 999.0;
    return duration / 58.2;
}

void drive(int leftSpeed, int rightSpeed) {
    int l_fwd = leftSpeed > 0 ? leftSpeed : 0;
    int l_rev = leftSpeed < 0 ? -leftSpeed : 0;
    ledcWrite(0, l_fwd); ledcWrite(1, l_rev); 
    ledcWrite(2, l_fwd); ledcWrite(3, l_rev); 

    int r_fwd = rightSpeed > 0 ? rightSpeed : 0;
    int r_rev = rightSpeed < 0 ? -rightSpeed : 0;
    ledcWrite(4, r_fwd); ledcWrite(5, r_rev); 
    ledcWrite(6, r_fwd); ledcWrite(7, r_rev); 
}

// ==========================================
// 4. Setup
// ==========================================
void setup() {
    Serial.begin(115200);

    pinMode(LINE_L, INPUT);
    pinMode(LINE_M, INPUT);
    pinMode(LINE_R, INPUT);

    for (int i = 0; i < 8; i++) ledcSetup(i, freq, resolution);
    ledcAttachPin(FL_A, 0); ledcAttachPin(FL_B, 1);
    ledcAttachPin(RL_A, 2); ledcAttachPin(RL_B, 3);
    ledcAttachPin(FR_A, 4); ledcAttachPin(FR_B, 5);
    ledcAttachPin(RR_A, 6); ledcAttachPin(RR_B, 7);

    pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
    pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
    pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);

    Serial.println("🔥 D0转模拟 终极全时四驱系统 Ready!");
}

// ==========================================
// 5. Loop 主逻辑
// ==========================================

void loop() {
    int rawL = analogRead(LINE_L);
    int rawR = analogRead(LINE_R);
    bool M  = digitalRead(LINE_M); 

    bool L = (rawL > D0_THRESHOLD); 
    bool R = (rawR > D0_THRESHOLD); 

    if (L) lastDirection = 1; 
    else if (R) lastDirection = 2; 

    float distF = getDistance(TRIG1, ECHO1);

    int baseSpeed = 220; 
    int maxSpeed = 255;    
    int softDiff = 120; 
    
    // 🌪️ 方案 A 暴力反转参数！
    int hardRev = -180; // 内侧轮直接挂倒挡扯后腿！

    if (L || M || R) {
        noLineStartTime = 0; 
        forceUltrasoundMode = false; 
    } else {
        if (noLineStartTime == 0) noLineStartTime = millis(); 
        if (millis() - noLineStartTime >= 10000) forceUltrasoundMode = true; 
    }

    if (forceUltrasoundMode) {
        // 超声波逻辑（略）...
    } 
    else {
        // 🛣️ 方案 A：暴力真·差速巡线状态机
        
        if (!L && M && !R) {
            drive(baseSpeed, baseSpeed); 
        }
        else if (L && M && !R) {
            drive(softDiff, maxSpeed); 
        }
        else if (!L && M && R) {
            drive(maxSpeed, softDiff); 
        }
        
        // 🚨 大幅偏右 (1 0 0) —— 方案 A 暴力直角左弯！
        else if (L && !M && !R) {
            drive(hardRev, maxSpeed); // 左轮倒车，右轮冲锋！
            delay(70); // ⏱️ 暴力甩尾时间（单位ms，如果转过头了就调小到 50，如果转不够就调大到 90）
        }
        
        // 🚨 大幅偏左 (0 0 1) —— 方案 A 暴力直角右弯！
        else if (!L && !M && R) {
            drive(maxSpeed, hardRev); 
            delay(70); 
        }
        
        // ❓ 脱轨搜线 (0 0 0) —— 记忆自旋
        else if (!L && !M && !R) {
            if (lastDirection == 1) {
                drive(-130, 130); 
            } else if (lastDirection == 2) {
                drive(130, -130); 
            } else {
                drive(-100, -100); 
            }
        }
        else {
            drive(baseSpeed, baseSpeed); 
        }
    }

    Serial.printf("L(ADC):%d M:%d R(ADC):%d | F:%.1f | %s\n", 
                  rawL, M, rawR, distF, forceUltrasoundMode ? "ULTRA" : "LINE");
    delay(10); 
}