#include <Arduino.h>

// ==========================================
// 1. 终极四驱引脚矩阵 (FR 锁定 15/2)
// ==========================================

#define FL_A 4
#define FL_B 5
#define RL_A 32
#define RL_B 33
#define FR_A 15 // 右前 A
#define FR_B 2  // 右前 B (自带蓝色 LED)
#define RR_A 16
#define RR_B 17

#define TRIG1 18 // 前
#define ECHO1 19
#define TRIG2 12 // 左
#define ECHO2 13
#define TRIG3 22 // 右
#define ECHO3 23

#define LINE_L 34 // ADC
#define LINE_M 21 // 数字读取
#define LINE_R 35 // ADC

// ==========================================
// 2. 全局配置与状态变量
// ==========================================
const int freq = 20000;
const int resolution = 8;

unsigned long noLineStartTime = 0; 
bool forceUltrasoundMode = false;  

const int D0_THRESHOLD = 2000; // ADC 阈值
int lastDirection = 0; // 记忆：1代表最后偏左，2代表最后偏右

// ==========================================
// 3. 全时四驱底层驱动函数
// ==========================================

// 超声波读取函数
float getDistance(int trig, int echo) {
    digitalWrite(trig, LOW); delayMicroseconds(2);
    digitalWrite(trig, HIGH); delayMicroseconds(10);
    digitalWrite(trig, LOW);
    long duration = pulseIn(echo, HIGH, 20000); // 20ms超时
    if (duration == 0) return 999.0;
    return duration / 58.2;
}

// 核心驱动：支持四驱正反转
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
// 4. Setup 初始化
// ==========================================
void setup() {
    Serial.begin(115200);

    pinMode(LINE_L, INPUT);
    pinMode(LINE_M, INPUT);
    pinMode(LINE_R, INPUT);

    // PWM 初始化
    for (int i = 0; i < 8; i++) ledcSetup(i, freq, resolution);
    ledcAttachPin(FL_A, 0); ledcAttachPin(FL_B, 1);
    ledcAttachPin(RL_A, 2); ledcAttachPin(RL_B, 3);
    ledcAttachPin(FR_A, 4); ledcAttachPin(FR_B, 5);
    ledcAttachPin(RR_A, 6); ledcAttachPin(RR_B, 7);

    pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
    pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
    pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);

    Serial.println("🔥 避障+优化回溯 终极全时四驱系统就绪!");
}

// ==========================================
// 5. Loop 主逻辑
// ==========================================
void loop() {
    // 【第1步】读取传感器数据
    int rawL = analogRead(LINE_L);
    int rawR = analogRead(LINE_R);
    bool M  = digitalRead(LINE_M); 

    bool L = (rawL > D0_THRESHOLD); 
    bool R = (rawR > D0_THRESHOLD); 

    // 【第2步】更新最后的记忆方向（用来回溯寻线）
    if (L && !R) lastDirection = 1; // 倒向左边
    if (!L && R) lastDirection = 2; // 倒向右边

    // 【第3步】读取前超声波测距
    float distF = getDistance(TRIG1, ECHO1);

    // 参数定义
    int baseSpeed = 220; 
    int maxSpeed = 255;    
    int softDiff = 120; 
    int hardRev = -180; // 暴力反转

    // ==========================================
    // 🛡️ 模块 A：10cm 紧急超声波避障（最高优先级）
    // ==========================================
    if (distF > 1.0 && distF <= 10.0) { // 过滤0误差，检测到10cm内的障碍
        Serial.println("🚨 [WARNING] 发现前方障碍！执行避障急停！");
        drive(-150, -150); // 先紧急刹车倒车
        delay(150);
        drive(0, 0);       // 彻底停稳
        delay(500); 

        // 读取左右超声波决定往哪拐弯绕障
        float distL = getDistance(TRIG2, ECHO2);
        float distR = getDistance(TRIG3, ECHO3);

        if (distL > distR) {
            drive(-150, 150); // 左边空，原地左转
            delay(300);
        } else {
            drive(150, -150); // 右边空，原地右转
            delay(300);
        }
        return; // 结束本次 loop，不执行底下的巡线，重新进入雷达扫描
    }

    // ==========================================
    // ⏱️ 模块 B：无黑线计时切换超声波导航
    // ==========================================
    if (L || M || R) {
        noLineStartTime = 0; 
        forceUltrasoundMode = false; 
    } else {
        if (noLineStartTime == 0) noLineStartTime = millis(); 
        if (millis() - noLineStartTime >= 10000) forceUltrasoundMode = true; 
    }

    // ==========================================
    // 🤖 模块 C：运动控制状态机
    // ==========================================
    if (forceUltrasoundMode) {
        // 🛰️ 迷宫自主导航
        float distL = getDistance(TRIG2, ECHO2);
        float distR = getDistance(TRIG3, ECHO3);

        if (distF < 25.0) { // 前方有墙
            if (distL > distR) {
                drive(-160, 160); // 左转
            } else {
                drive(160, -160); // 右转
            }
            delay(200);
        } else {
            drive(180, 180); // 直行
        }
    } 
    else {
        // 🛣️ 正常循线
        
        if (!L && M && !R) {
            drive(baseSpeed, baseSpeed); // 直行
        }
        else if (L && M && !R) {
            drive(softDiff, maxSpeed); // 微偏左修正
        }
        else if (!L && M && R) {
            drive(maxSpeed, softDiff); // 微偏右修正
        }
        
        // 🚨 触发直角左弯 (1 0 0)
        else if (L && !M && !R) {
            drive(hardRev, maxSpeed); 
            delay(70); 
        }
        
        // 🚨 触发直角右弯 (0 0 1)
        else if (!L && !M && R) {
            drive(maxSpeed, hardRev); 
            delay(70); 
        }
        
        // ❓ 物理回溯搜线 (0 0 0) - 当车子冲出赛道全白时
        else if (!L && !M && !R) {
            int searchSpeed = 110; // 降低搜线速度，防止惯性划过黑线而识别不到

            if (lastDirection == 1) {
                // 刚才往左边倒，说明黑线其实被甩在了右侧！
                drive(searchSpeed, -searchSpeed); // 原地右转去找线
            } else if (lastDirection == 2) {
                // 刚才往右边倒，说明黑线其实被甩在了左侧！
                drive(-searchSpeed, searchSpeed); // 原地左转去找线
            } else {
                drive(-110, -110); // 如果不知道在哪，开启倒车回溯！
            }
        }
        else {
            drive(baseSpeed, baseSpeed); 
        }
    }

    // 调试打印
    Serial.printf("L(ADC):%d M:%d R(ADC):%d | F:%.1f | %s\n", 
                  rawL, M, rawR, distF, forceUltrasoundMode ? "ULTRA" : "LINE");
    delay(10); 
}