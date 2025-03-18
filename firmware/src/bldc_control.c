#include "bldc_control.h"
#include "pid.h"
#include "foc.h"
#include "isr_handler.h"
#include "encoder.h"
#include "pwm.h"
#include "adc.h"
#include <math.h>

// 馬達控制變數
MotorControl_t motor;

// 初始化 BLDC 馬達控制系統
void BLDC_Init() {
    // 初始化 PID 控制參數
    motor.iq_pid.kp = 0.5f;
    motor.iq_pid.ki = 0.01f;
    motor.iq_pid.kd = 0.0f;
    PID_Init(&motor.iq_pid);
    
    motor.id_pid.kp = 0.5f;
    motor.id_pid.ki = 0.01f;
    motor.id_pid.kd = 0.0f;
    PID_Init(&motor.id_pid);
    
    // 初始化 FOC 變數
    motor.theta = 0.0f;
    motor.i_d = 0.0f;
    motor.i_q = 0.0f;
    motor.v_d = 0.0f;
    motor.v_q = 0.0f;
    
    // 設置 PWM 與 ADC
    PWM_Init();
    ADC_Init();
    
    // 初始化編碼器
    Encoder_Init();
}

// 更新 BLDC 控制迴路
void BLDC_Update() {
    // 讀取電流感測數據
    float i_a, i_b, i_c;
    ADC_GetCurrent(&i_a, &i_b, &i_c);
    
    // Clarke 變換: 三相電流轉換為 α-β 座標系
    float i_alpha, i_beta;
    Clarke_Transform(i_a, i_b, i_c, &i_alpha, &i_beta);
    
    // 取得馬達轉子角度
    motor.theta = Encoder_GetAngle();
    
    // Park 變換: 轉換為 dq 座標系
    Park_Transform(i_alpha, i_beta, motor.theta, &motor.i_d, &motor.i_q);
    
    // PID 控制計算
    motor.v_d = PID_Compute(&motor.id_pid, 0.0f, motor.i_d);
    motor.v_q = PID_Compute(&motor.iq_pid, motor.target_speed, motor.i_q);
    
    // 反向 Park 變換: dq → αβ
    float v_alpha, v_beta;
    Inv_Park_Transform(motor.v_d, motor.v_q, motor.theta, &v_alpha, &v_beta);
    
    // 反向 Clarke 變換: αβ → 三相電壓
    float v_a, v_b, v_c;
    Inv_Clarke_Transform(v_alpha, v_beta, &v_a, &v_b, &v_c);
    
    // 更新 PWM
    PWM_Update(v_a, v_b, v_c);
}

// 設定馬達轉速
void BLDC_SetSpeed(float speed) {
    motor.target_speed = speed;
}

// 取得目前馬達轉速
float BLDC_GetSpeed() {
    return motor.current_speed;
}
