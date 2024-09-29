/*---------------------------------------------
  自立用サンプル
  ---------------------------------------------*/
/*
  センサ(相手センサx3・土俵センサx2)の入力に応じてそれなりに反応して動く。
*/

#include <ud5.h>

CDXIF dx;

CGPIO gpio (
  (const CGPIO::TPinMode[10]) {
    CGPIO::tPinDIN_PU,  // 非常停止
    CGPIO::tPinDIN_PU,
    CGPIO::tPinDIN_PU,
    CGPIO::tPinDIN_PU,
    CGPIO::tPinDIN_PU,  // DIP0
    CGPIO::tPinDIN_PU,  // DIP1
    CGPIO::tPinDIN_PU,  // DIP2
    CGPIO::tPinDIN_PU,  // DAC
    CGPIO::tPinDIN_PU,  // DIP3
    CGPIO::tPinDIN_PU,  // PB1
  }
);

CEXIO exio (
  (const CEXIO::TPinMode[16]) {
    CEXIO::tPinDIN_PU,  // 右側物体検出センサ
    CEXIO::tPinDIN_PU,  // 中央物体検出センサ
    CEXIO::tPinDIN_PU,  // 左側物体検出センサ
    CEXIO::tPinDIN_PU,  // 右側土俵検出センサ
    CEXIO::tPinDIN_PU,  // 左側土俵検出センサ
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU
  }
);

// GPIOとJPのビットアサイン
union bf_gpio {
  struct {
    uint16_t emg  : 1;
    uint16_t      : 1;
    uint16_t      : 1;
    uint16_t      : 1;
    uint16_t dip0 : 1;
    uint16_t dip1 : 1;
    uint16_t dip2 : 1;
    uint16_t dac  : 1;
    uint16_t dip3 : 1;
    uint16_t pb1  : 1;
  };
  uint16_t u16;
} mmi;

union bf_exio {
  struct {
    uint16_t TR  : 1;
    uint16_t TC  : 1;
    uint16_t TL  : 1;
    uint16_t RR  : 1;
    uint16_t RL  : 1;
  };
  uint16_t u16;
} sen;

// 最大PWM出力値(‰ ブートストラップ回路への充電時間を加味)
#define _MAX_MOTOR_VALUE  (1000-20)

CMotor motor (
  50000,  // PWM周波数
  0,      // モータの回転方向
  (uint16_t[2]) { _MAX_MOTOR_VALUE, _MAX_MOTOR_VALUE }
);

// モータへのランプ指令増分(0～1000‰/ms)
#define _MOTOR_RAMP (10)

// 出力を調整するためのゲイン(0～100%)
#define _POWER_GAIN (30)

//------------------------------------------------
// モータ動作用クラス
//------------------------------------------------
class cdrive {
  int gain;

  int32_t m1_target = 0, m2_target = 0;

  void motor (int32_t L, int32_t R) {
    m1_target = (R * gain) / 100;
    m2_target = (L * gain) / 100;
  }
 public:
  cdrive (uint32_t v) {
    if (v > 100) v = 100;
    gain = v;
  }

  // このクラスの出力結果はこの関数で取得
  int32_t get_m1 (void) {
    return m1_target;
  }
  int32_t get_m2 (void) {
    return m2_target;
  }

  void forward (int16_t pow) {
    motor (abs (pow), abs (pow));
  }
  void back (int16_t pow) {
    motor (-abs (pow), -abs (pow));
  }
  void stop (void) {
    motor (0, 0);
  }
  void right_turn (int16_t pow) {
    motor (abs (pow), -abs (pow));
  }
  void left_turn (int16_t pow) {
    motor (-abs (pow), abs (pow));
  }
  void turn (int16_t l_pow, int16_t r_pow) {
    motor (abs (l_pow), abs (r_pow));
  }
  void right_turn_time (uint32_t t) {
    right_turn (1000);
    UD5_WAIT (t);
    stop();
  }
  void left_turn_time (uint32_t t) {
    left_turn (1000);
    UD5_WAIT (t);
    stop();
  }
  void back_time (uint32_t t) {
    back (1000);
    UD5_WAIT (t);
    stop();
  }
} drive (_POWER_GAIN);

//---------------------------------------------------------------------------------
// 戦いタスク
//---------------------------------------------------------------------------------
// 時間定義
#define turn_back_wait  (500)   // 片側土俵検知時の後進時間[ms]
#define turn_turn_wait  (500)   // 片側土俵検知時の旋回時間[ms]
#define back_back_wait  (600)   // 両側土俵検知時の後進時間[ms]
#define back_turn_wait  (600)   // 両側土俵検知時の旋回時間[ms]

void TASK1 (void *pvParameters) {
  // ゲートON待ち (TASK3で処理)
  while (!motor.get_gate()) {
    UD5_WAIT (5);
    if (sen.u16 != 0) exio.set_LED_on (2);
    else exio.set_LED_off (2);
  }

  // 立ち合い
  switch (mmi.dip0 | (mmi.dip1 << 1) | (mmi.dip2 << 2) | (mmi.dip3 << 3)) {
    case 0:
      drive.forward (800);
      break;
    case 1:
      drive.back (800);
      break;
    case 2:
      drive.right_turn (800);
      break;
    case 3:
      drive.left_turn (800);
      break;
    case 4:
      drive.left_turn_time (500);
      drive.right_turn (1000);
      break;
  }

  for (;;) {
    if (!sen.RL && sen.RR) {
      drive.back_time (turn_back_wait);
      drive.left_turn_time (turn_turn_wait);
    } else if (sen.RL && !sen.RR) {
      drive.back_time (turn_back_wait);
      drive.right_turn_time (turn_turn_wait);
    } else if (sen.RL && sen.RR) {
      drive.back_time (back_back_wait);
      drive.left_turn_time (back_turn_wait);
    } else {
      if (!sen.TL && !sen.TC &&  sen.TR) drive.right_turn (1000);
      else if (!sen.TL &&  sen.TC && !sen.TR) drive.forward (1000);
      else if (!sen.TL &&  sen.TC &&  sen.TR) drive.turn (1000, 500);
      else if (sen.TL && !sen.TC && !sen.TR) drive.left_turn (1000);
      else if (sen.TL && !sen.TC &&  sen.TR) drive.forward (1000);
      else if (sen.TL &&  sen.TC && !sen.TR) drive.turn (500, 1000);
      else if (sen.TL &&  sen.TC &&  sen.TR) drive.forward (1000);
      else drive.forward (800);
    }
  }
}

//---------------------------------------------------------------------------------
// GPIOとEXIOの取り込みとモータへのランプ指令
//---------------------------------------------------------------------------------
void TASK2 (void *pvParameters) {
  static int32_t m1 = 0, m2 = 0;
  portTickType t = xTaskGetTickCount();

  for (;;) {
    mmi.u16 = ~gpio.get_gpio();
    sen.u16 = ~exio.get_gpio();

    if (m1 < drive.get_m1()) {
      m1 += _MOTOR_RAMP;
      if (m1 > drive.get_m1()) m1 = drive.get_m1();
    } else if (m1 > drive.get_m1()) {
      m1 -= _MOTOR_RAMP;
      if (m1 < drive.get_m1()) m1 = drive.get_m1();
    }
    if (m2 < drive.get_m2()) {
      m2 += _MOTOR_RAMP;
      if (m2 > drive.get_m2()) m2 = drive.get_m2();
    } else if (m2 > drive.get_m2()) {
      m2 -= _MOTOR_RAMP;
      if (m2 < drive.get_m2()) m2 = drive.get_m2();
    }
    motor.set_duty (m1, m2);
    vTaskDelayUntil (&t, 1);

    if (dx.rxbuff()) {
      switch (dx.getc()) {
        case '!':
          UD5_SOFTRESET();
          break;
      }
    }
  }
}

//---------------------------------------------------------------------------------
// スタート・ストップ
//---------------------------------------------------------------------------------
void TASK3 (void *pvParameters) {
  uint32_t t = 0;
  bool prev_emg, emg;

  UD5_WAIT (500); // 起動から少し時間をおいて処理開始
  prev_emg = emg = mmi.emg;

  while (1) {
    // 2つの入力のいずれかでゲートのON/OFFをトグル
    emg = mmi.emg;
    if (mmi.pb1 || (emg != prev_emg)) {
      motor.set_gate (!motor.get_gate());
      UD5_WAIT (50);
    }
    prev_emg = emg;

    // ゲートONの間は100ms、OFFの間は1000ms周期にLED1を点滅
    if (t == 0) {
      t = UD5_GET_ELAPSEDTIME() + (motor.get_gate() ? 100 : 1000);
    }
    if ((t != 0) && (UD5_GET_ELAPSEDTIME() > t)) {
      exio.set_LED_toggle (1);
      t = 0;
    }
    UD5_WAIT (10);
  }
}

//---------------------------------------------------------------------------------
// メイン
//---------------------------------------------------------------------------------
int main (void) {
  exio.set_LED_off (0xf);

  // 各タスクを作成
  xTaskCreate (TASK1, NULL, 100, NULL, 1, NULL);
  xTaskCreate (TASK2, NULL, 100, NULL, 1, NULL);
  xTaskCreate (TASK3, NULL, 100, NULL, 1, NULL);
  // カーネル起動
  vTaskStartScheduler();
}
