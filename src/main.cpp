#include <Arduino.h>

// ==========================================
// 📋 A. 动力系统引脚矩阵
// ==========================================
#define FL_A 4    
#define FL_B 5
#define RL_A 33   
#define RL_B 32
#define FR_A 15   
#define FR_B 2
#define RR_A 26   
#define RR_B 27

const int CH_FL_A = 0; const int CH_FL_B = 1;
const int CH_RL_A = 2; const int CH_RL_B = 3;
const int CH_FR_A = 4; const int CH_FR_B = 5;
const int CH_RR_A = 6; const int CH_RR_B = 7;

const int freq = 20000;
const int resolution = 8;

// ==========================================
// 📋 B. 感知矩阵引脚定义
// ==========================================
#define TRIG_F 18  
#define ECHO_F 19
#define TRIG_L 12  
#define ECHO_L 13
#define TRIG_R 22  
#define ECHO_R 23

// ==========================================
// 🚀 四驱狂暴狂飙参数【转向大幅度强化版】
// ==========================================
const int CRUISE_SPEED = 240;   // 空旷全速前冲
const int BASE_SPEED   = 180;   // 靠墙巡航基速

// 【核心修改】将转向电机的反转与正转全部压榨到硬件极限 255！大力出奇迹！
const int MAX_SPEED    = 255;   
const int REV_SPEED    = -255;  

// 物理界限定义
const float LIMIT_FRONT = 7.0;  // 前方障碍红线
const float LIMIT_SIDE  = 2.0;  // 侧边擦碰红线

// ==========================================
// 📏 超声波单路高频测距
// ==========================================
float getDistance(int trig, int echo) {
    digitalWrite(trig, LOW); delayMicroseconds(2);
    digitalWrite(trig, HIGH); delayMicroseconds(10);
    digitalWrite(trig, LOW);
    
    long duration = pulseIn(echo, HIGH, 15000); 
    if (duration == 0) return -1.0;            
    return duration * 0.034 / 2.0;
}

// ==========================================
// 🏎️ 4WD 电机驱动
// ==========================================
void drive4WD(int leftSpeed, int rightSpeed) {
    leftSpeed  = constrain(leftSpeed, -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);

    int l_fwd = leftSpeed > 0 ? leftSpeed : 0;
    int l_rev = leftSpeed < 0 ? -leftSpeed : 0;
    ledcWrite(CH_FL_A, l_fwd); ledcWrite(CH_FL_B, l_rev);
    ledcWrite(CH_RL_A, l_fwd); ledcWrite(CH_RL_B, l_rev);

    int r_fwd = rightSpeed > 0 ? rightSpeed : 0;
    int r_rev = rightSpeed < 0 ? -rightSpeed : 0;
    ledcWrite(CH_FR_A, r_fwd); ledcWrite(CH_FR_B, r_rev);
    ledcWrite(CH_RR_A, r_fwd); ledcWrite(CH_RR_B, r_rev);
}

void setup() {
    Serial.begin(115200);
    for(int i=0; i<8; i++) { ledcSetup(i, freq, resolution); }
    ledcAttachPin(FL_A, CH_FL_A); ledcAttachPin(FL_B, CH_FL_B);
    ledcAttachPin(RL_A, CH_RL_A); ledcAttachPin(RL_B, CH_RL_B);
    ledcAttachPin(FR_A, CH_FR_A); ledcAttachPin(FR_B, CH_FR_B);
    ledcAttachPin(RR_A, CH_RR_A); ledcAttachPin(RR_B, CH_RR_B);

    pinMode(TRIG_F, OUTPUT); pinMode(ECHO_F, INPUT);
    pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
    pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);

    drive4WD(0, 0);
    delay(1000);
    Serial.println("🦁 [4WD LARGE-TURN] 大摆角爆发版点火！");
}

void loop() {
    float dist1 = getDistance(TRIG_F, ECHO_F); 
    float dist2 = getDistance(TRIG_L, ECHO_L); 
    float dist3 = getDistance(TRIG_R, ECHO_R); 

    // ==================================================
    // 🟩 1. 绝对空旷判定：直接全速前冲
    // ==================================================
    if ((dist1 > LIMIT_FRONT || dist1 < 0) && 
        (dist2 > LIMIT_SIDE || dist2 < 0) && 
        (dist3 > LIMIT_SIDE || dist3 < 0)) {
        
        drive4WD(CRUISE_SPEED, CRUISE_SPEED);
        delay(10);
        return; 
    }

    // ==================================================
    // 🚨 2. 前方极限贴脸（F <= 7cm）：大幅度原地坦克转向
    // ==================================================
    if (dist1 > 0 && dist1 <= LIMIT_FRONT) {
        Serial.println("🛑 [CRITICAL FRONT] 极限贴脸！极限大角度原地转向！");
        
        bool turnLeft = true;
        if (dist2 > 0 && dist3 > 0) {
            turnLeft = (dist2 > dist3);
        } else if (dist2 < 0) { 
            turnLeft = true;
        } else if (dist3 < 0) { 
            turnLeft = false;
        }

        // 【大摆角修改 A】：速度拉满到绝对极限 ±255，同时强制执行时间从 200ms 延长到 380ms！
        // 这一刀下去，车头会有一个非常极其暴力且明显的甩头大动作
        unsigned long startTime = millis();
        while(millis() - startTime < 380) { 
            if (turnLeft) {
                drive4WD(REV_SPEED, MAX_SPEED); // 左后退，右前进 -> 大力左转
            } else {
                drive4WD(MAX_SPEED, REV_SPEED); // 左前进，右后退 -> 大力右转
            }
            delay(5);
        }
        drive4WD(0, 0); delay(40); // 刹车死锁稳定
        return;
    }

    // ==================================================
    // 🚨 3. 左右侧极限贴墙（L/R <= 2cm）：大动作弹开避让
    // ==================================================
    // 【大摆角修改 B】：侧边急救闪避动作幅度加大，内侧轮直接给倒车电压（180 vs -100）
    if (dist2 > 0 && dist2 <= LIMIT_SIDE) {
        Serial.println("⚠️ [ESCAPE] 左侧太近！强力扭头向右弹开！");
        drive4WD(MAX_SPEED, -100); 
        delay(150); // 持续时间延长，确保大幅度弹开
        return;
    }
    if (dist3 > 0 && dist3 <= LIMIT_SIDE) {
        Serial.println("⚠️ [ESCAPE] 右侧太近！强力扭头向左弹开！");
        drive4WD(-100, MAX_SPEED); 
        delay(150); 
        return;
    }

    // ==================================================
    // 🏎️ 4. 正常靠墙纠偏：高灵敏度大动作微调
    // ==================================================
    if (dist2 > 0 && dist3 > 0) {
        float error = dist2 - dist3;
        // 【大摆角修改 C】：系数从 10 暴增到 20！
        // 只要两边有一点不对等，四驱两侧立刻拉开高达 100+ 的差速，动作极其敏感利落
        int diff = error * 20; 
        diff = constrain(diff, -100, 100); // 允许更大的修正差速上限
        drive4WD(BASE_SPEED + diff, BASE_SPEED - diff);
    } else {
        drive4WD(BASE_SPEED, BASE_SPEED);
    }

    delay(15);
}