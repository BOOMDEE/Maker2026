#include <Arduino.h>
#include <Wire.h>
#include <DFRobot_LIS2DW12.h>

// ==========================================
// 1. 终极无冲突引脚映射
// ==========================================

// --- A. 动力系统 (用可输出引脚) ---
#define FL_A 27
#define FL_B 26
#define RL_A 4
#define RL_B 5
#define FR_A 15// 👈 右前轮 A (可输出)
#define FR_B 2 // 👈 右前轮 B (可输出)
#define RR_A 32
#define RR_B 33

// --- B. 感知矩阵 ---
#define TRIG1 18 
#define ECHO1 19
#define TRIG2 12 
#define ECHO2 13
#define TRIG3 22 
#define ECHO3 23

#define I2C_SDA 14 
#define I2C_SCL 25 

// --- C. 巡线系统 (用仅输入引脚) ---
#define LINE_L 34 // 👈 左巡线 (仅输入，完美！)
#define LINE_M 21 
#define LINE_R 35 // 👈 右巡线 (仅输入，完美！)

// ==========================================
// 2. 实例与变量
// ==========================================
DFRobot_LIS2DW12_I2C lis(&Wire, 0x19); 

const int freq = 20000;
const int resolution = 8;

unsigned long noLineStartTime = 0; 
bool forceUltrasoundMode = false;  

// ==========================================
// 3. 核心功能
// ==========================================
float getDistance(int trig, int echo) {
    digitalWrite(trig, LOW); delayMicroseconds(2);
    digitalWrite(trig, HIGH); delayMicroseconds(10);
    digitalWrite(trig, LOW);
    long duration = pulseIn(echo, HIGH, 20000); 
    if (duration == 0) return 999.0;
    return duration / 58.2;
}

float getPitch() {
    float ay = lis.readAccY();
    float az = lis.readAccZ();
    return atan2(ay, az) * 180.0 / PI;
}

void drive(int leftSpeed, int rightSpeed) {
    ledcWrite(0, leftSpeed > 0 ? leftSpeed : 0);
    ledcWrite(1, leftSpeed < 0 ? -leftSpeed : 0);
    ledcWrite(2, leftSpeed > 0 ? leftSpeed : 0);
    ledcWrite(3, leftSpeed < 0 ? -leftSpeed : 0);

    ledcWrite(4, rightSpeed > 0 ? rightSpeed : 0);
    ledcWrite(5, rightSpeed < 0 ? -rightSpeed : 0);
    ledcWrite(6, rightSpeed > 0 ? rightSpeed : 0);
    ledcWrite(7, rightSpeed < 0 ? -rightSpeed : 0);
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

    Wire.begin(I2C_SDA, I2C_SCL); 
    lis.begin();

    Serial.println("🏆 硬件引脚无冲突矩阵，全系统上线！");
}

// ==========================================
// 5. Loop (巡线紧凑型状态机)
// ==========================================
void loop() {
    bool L = digitalRead(LINE_L); 
    bool M = digitalRead(LINE_M); 
    bool R = digitalRead(LINE_R); 

    float distF = getDistance(TRIG1, ECHO1);
    float distL = getDistance(TRIG2, ECHO2);
    float distR = getDistance(TRIG3, ECHO3);
    float pitch = getPitch();

    int baseSpeed = 160; 
    if (pitch > 10.0) baseSpeed = 220; 
    else if (pitch < -10.0) baseSpeed = 100; 

    int fast = baseSpeed;
    int mid  = baseSpeed - 40;
    int slow = baseSpeed - 80;

    // 🕒 10秒倒计时逻辑
    if (L || M || R) {
        noLineStartTime = 0; 
        forceUltrasoundMode = false; 
    } else {
        if (noLineStartTime == 0) noLineStartTime = millis(); 
        if (millis() - noLineStartTime >= 10000) forceUltrasoundMode = true; 
    }

    if (forceUltrasoundMode) {
        // 纯超声波
        if (distF < 25.0) {
            drive(0, 0); delay(100);
            if (distL > distR) drive(-150, 150); 
            else drive(150, -150); 
            delay(450); 
        } else if (distL < 15.0) {
            drive(baseSpeed, slow);
        } else if (distR < 15.0) {
            drive(slow, fast);
        } else {
            drive(baseSpeed, baseSpeed);
        }
    } else {
        // 巡线模式
        if (!L && M && !R) drive(baseSpeed, baseSpeed);
        else if (L && M && !R) drive(mid, fast); 
        else if (!L && M && R) drive(fast, mid);
        else if (L && !M && !R) drive(slow, fast);
        else if (!L && !M && R) drive(fast, slow);
        else if (!L && !M && !R) drive(-110, 110); 
        else drive(baseSpeed, baseSpeed);
    }

    Serial.printf("L:%d M:%d R:%d | F:%.1f | P:%.1f | %s\n", 
                  L, M, R, distF, pitch, forceUltrasoundMode ? "ULTRA" : "LINE");
    delay(10); 
}