/*!
 @file  sample20_MOTOR_PID.cpp
 @brief モータ駆動
 @note
  2chのモータアンプを駆動。
  GPIOにモータと連動した2相エンコーダの信号を入力。
  エンコーダのパルス数をカウントし、それを元にPIDを用いて速度制御を行う。
 */
#include <ud5.h>

CDXIF dx;

CGPIO gpio (
  (const CGPIO::TPinMode[10]) {
    CGPIO::tPinDIN_PU,
    CGPIO::tPinDIN_PU,
    CGPIO::tPinINT0_ENC0A,
    CGPIO::tPinINT1_ENC0B,
    CGPIO::tPinINT2_ENC1A,
    CGPIO::tPinINT3_ENC1B,
    CGPIO::tPinDIN_PU,
    CGPIO::tPinDIN_PU,
    CGPIO::tPinDIN_PU,
    CGPIO::tPinDIN_PU,
  }
);

CEXIO exio;

#define _MAX_MOTOR_VALUE  (1000-20)

//! モータクラスをインスタンス化
CMotor motor (50000, 3, (uint16_t[2]) { _MAX_MOTOR_VALUE, _MAX_MOTOR_VALUE });

//! 速度制御クラスをインスタンス化
CSpeedControl spdc(&motor, &gpio);

CPID::TPIDParam
  Gain0 = { 3.0, 14.0, 0.0, 0, 0.005, -1000, 1000, {0.0, 0.0}, 0.0, 0.0}, //!< 速度制御ゲイン0
  Gain1 = { 3.0, 14.0, 0.0, 0, 0.005, -1000, 1000, {0.0, 0.0}, 0.0, 0.0}; //!< 速度制御ゲイン1

//! タスク1
//! 速度制御はFreeRTOSありきなのでmainではなくタスクで諸々処理
void TASK1 (void *pvParameters) {
  int n = 0;
  exio.set_LED (0);
  motor.set_gate (true);
  spdc.set_biaxial_gain (&Gain0, &Gain1);
  spdc.set_biaxial_ramp (1000.0, 10000.0);
  spdc.begin();
  spdc.start();

  while (1) {
    int32_t speed = sin (UD5_GET_ELAPSEDTIME() * 2.0 * M_PI / 6000.0) * 450.0;
    spdc.set_biaxial_taget_speed (speed, speed);
    dx.printf("\r%4d, %4d, %4d", speed, spdc.get_current_speed(0), spdc.get_current_speed(1));
//    dx.printf("\r%4d, %4d", gpio.get_encoder_count(0), gpio.get_encoder_count(1));

    if (dx.rxbuff()) { if (dx.getc() == '!') UD5_SOFTRESET(); }
    exio.set_LED (n++ % 16);
    UD5_WAIT (5);
  }
}

//! main関数
int main (void) {
  xTaskCreate (TASK1, NULL, 200, NULL, 1, NULL);
  vTaskStartScheduler();
}
