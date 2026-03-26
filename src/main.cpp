#include <Arduino.h>

// ==========================================
// 1. 引脚定义
// ==========================================

#define FL_A 4
#define FL_B 5
#define RL_A 32
#define RL_B 33
#define FR_A 15 // 右前 A
#define FR_B 2  // 右前 B (自带蓝色 LED)
#define RR_A 16
#define RR_B 17

#define TRIG1 18 // 前超声波
#define ECHO1 19
#define TRIG2 12 // 左超声波
#define ECHO2 13
#define TRIG3 22 // 右超声波
#define ECHO3 23

#define LINE_L 34 // 循线传感器左 (ADC)
#define LINE_M 21 // 循线传感器中 (数字)
#define LINE_R 35 // 循线传感器右 (ADC)

// ==========================================
// 2. 全局配置与状态变量
// ==========================================
const int freq = 20000;
const int resolution = 8;

unsigned long noLineStartTime = 0; 
bool forceUltrasoundMode = false;  

const int D0_THRESHOLD = 2000; // ADC 阈值
int lastDirection = 0; // 记忆最后偏离方向：1为左，2为右

// ==========================================
// 3. 底层驱动函数
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

// 四驱核心驱动函数
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
// 4. 初始化配置
// ==========================================
void setup() {
    Serial.begin(115200);

    pinMode(LINE_L, INPUT);
    pinMode(LINE_M, INPUT);
    pinMode(LINE_R, INPUT);

    // PWM 引脚绑定
    for (int i = 0; i < 8; i++) ledcSetup(i, freq, resolution);
    ledcAttachPin(FL_A, 0); ledcAttachPin(FL_B, 1);
    ledcAttachPin(RL_A, 2); ledcAttachPin(RL_B, 3);
    ledcAttachPin(FR_A, 4); ledcAttachPin(FR_B, 5);
    ledcAttachPin(RR_A, 6); ledcAttachPin(RR_B, 7);

    pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
    pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
    pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);

    Serial.println("System Ready: Obstacle Avoidance and Line Tracking");
}

// ==========================================
// 5. 主循环逻辑
// ==========================================
void loop() {
    // 1. 读取传感器数据
    int rawL = analogRead(LINE_L);
    int rawR = analogRead(LINE_R);
    bool M  = digitalRead(LINE_M); 

    bool L = (rawL > D0_THRESHOLD); 
    bool R = (rawR > D0_THRESHOLD); 

    // 2. 更新方向记忆
    if (L && !R) lastDirection = 1; // 偏左
    if (!L && R) lastDirection = 2; // 偏右

    // 3. 前方避障判断
    float distF = getDistance(TRIG1, ECHO1);

    int baseSpeed = 220; 
    int maxSpeed = 250;    // 微调时的外侧轮最高速
    int turnSlow = 50;     // 微调时的内侧轮速度（降低此值可以增大转弯幅度）
    int hardRev = -180;    // 直角弯暴力反转速度

    // A. 紧急超声波避障 (10cm 优先级最高)
    if (distF > 1.0 && distF <= 10.0) { 
        Serial.println("Warning: Obstacle detected! Emergency braking!");
        drive(-150, -150); 
        delay(150);
        drive(0, 0);       
        delay(500); 

        float distL = getDistance(TRIG2, ECHO2);
        float distR = getDistance(TRIG3, ECHO3);

        if (distL > distR) {
            drive(-150, 150); // 左侧空旷，原地左转
            delay(300);
        } else {
            drive(150, -150); // 右侧空旷，原地右转
            delay(300);
        }
        return; 
    }

    // B. 无线计时切换模式
    if (L || M || R) {
        noLineStartTime = 0; 
        forceUltrasoundMode = false; 
    } else {
        if (noLineStartTime == 0) noLineStartTime = millis(); 
        if (millis() - noLineStartTime >= 10000) forceUltrasoundMode = true; 
    }

    // C. 运动控制状态机
    if (forceUltrasoundMode) {
        // 迷宫自主超声波导航
        float distL = getDistance(TRIG2, ECHO2);
        float distR = getDistance(TRIG3, ECHO3);

        if (distF < 25.0) { 
            if (distL > distR) {
                drive(-160, 160); 
            } else {
                drive(160, -160); 
            }
            delay(200);
        } else {
            drive(180, 180); 
        }
    } 
    else {
        // 循线模式
        
        // 直行 (0 1 0)
        if (!L && M && !R) {
            drive(baseSpeed, baseSpeed); 
        }
        // 偏左修正 (1 1 0)：拉大差速
        else if (L && M && !R) {
            drive(turnSlow, maxSpeed); 
        }
        // 偏右修正 (0 1 1)：拉大差速
        else if (!L && M && R) {
            drive(maxSpeed, turnSlow); 
        }
        // 直角左弯 (1 0 0)
        else if (L && !M && !R) {
            drive(hardRev, maxSpeed); 
            delay(70); 
        }
        // 直角右弯 (0 0 1)
        else if (!L && !M && R) {
            drive(maxSpeed, hardRev); 
            delay(70); 
        }
        // 脱线物理回溯 (0 0 0)
        else if (!L && !M && !R) {
            int searchSpeed = 110; 

            if (lastDirection == 1) {
                drive(searchSpeed, -searchSpeed); // 原地右转寻线
            } else if (lastDirection == 2) {
                drive(-searchSpeed, searchSpeed); // 原地左转寻线
            } else {
                drive(-110, -110); // 盲倒退回溯
            }
        }
        else {
            drive(baseSpeed, baseSpeed); 
        }
    }

    // 串口状态输出
    Serial.printf("L:%d M:%d R:%d | F:%.1f | Mode:%s\n", 
                  rawL, M, rawR, distF, forceUltrasoundMode ? "ULTRA" : "LINE");
    delay(10); 
}
