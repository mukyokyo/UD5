/*!
 @file  sample17_MOTOR.cpp
 @brief モータ駆動
 @note
  2chのモータアンプを駆動。
 */
#include <ud5.h>

//! 最大デューティ比
#define _MAX_DUTY_VALUE   (1000-20)

CDXIF dx;

//! モータの動作モード設定
CMotor motor (
  50000,
  0,
  (uint16_t[2]) { _MAX_DUTY_VALUE, _MAX_DUTY_VALUE }
);

CEXIO exio;

//! main関数
int main (void) {
  char c;
  int8_t mode = 0, oldmode = 0;
  uint8_t sd = 0, oldsd = 0;
  int m1 = 0, m2 = 0, oldm1 = 0, oldm2 = 0;
  bool gate = false, oldgate = false;
  int f1 = 10, f2 = 40;

  while (1) {
    if (dx.rxbuff()) {
      switch ((c = dx.getc())) {
        case 'z': // 回転方向と入れ替えをデフォルトに
          motor.set_swap_dir (0);
          break;
        case 'x': // M1のみ回転方向反転
          motor.set_swap_dir (motor.get_swap_dir() ^ 1);
          break;
        case 'c': // M2のみ回転方向反転
          motor.set_swap_dir (motor.get_swap_dir() ^ 2);
          break;
        case 'v': // M1とM2を入れ替え
          motor.set_swap_dir (motor.get_swap_dir() ^ 4);
          break;

        case '0'...'9': // デューティーと方向を適宜変更
          mode = c - '0';
          break;
        case 'a': // 正弦波でデューティーを更新
          mode = 10;
          break;
        case '!': // 終了
          UD5_SOFTRESET();
          break;
        default:
          mode = -1;
          break;
      }
      sd = motor.get_swap_dir();
    }

    switch (mode) {
      case -1:  // モータフリー
      default:
        motor.set_gate (false);
        gate = false;
        break;
      case 0:   // ブレーキ
        motor.set_gate (true);
        motor.set_biaxial_dual (0, 0);
        gate = true;
        break;
      case 1 ... 4: // プラス方向
        motor.set_gate (true);
        m1 = m2 = mode * (_MAX_DUTY_VALUE / 4);
        motor.set_biaxial_dual (m1, m2);
        gate = true;
        break;
      case 5 ... 8: // マイナス方向
        motor.set_gate (true);
        m1 = m2 = - (mode - 4) * (_MAX_DUTY_VALUE / 4);
        motor.set_biaxial_dual (m1, m2);
        gate = true;
        break;
      case 10:  // 正弦波
        motor.set_gate (true);
        m1 += f1;
        if (m1 > _MAX_DUTY_VALUE) {
          m1 = _MAX_DUTY_VALUE;
          f1 *= -1;;
        }
        if (m1 < -_MAX_DUTY_VALUE) {
          m1 = 8 - _MAX_DUTY_VALUE;
          f1 *= -1;;
        }
        m2 += f2;
        if (m2 > _MAX_DUTY_VALUE) {
          m2 = _MAX_DUTY_VALUE;
          f2 *= -1;;
        }
        if (m2 < -_MAX_DUTY_VALUE) {
          m2 = -_MAX_DUTY_VALUE;
          f2 *= -1;;
        }
        motor.set_biaxial_dual (m1, m2);
        gate = true;
        break;
    }

    // ゲートON中にLED点滅
    if (motor.get_gate()) exio.set_LED_toggle (0xf);
    else exio.set_LED_off (0xf);

    // 表示の更新を必要最低限にするために以前の値と比較
    if ((mode != oldmode) || (m1 != oldm1) || (m2 != oldm2) || (gate != oldgate) || (sd != oldsd)) {
      // 現在の設定値を表示
      dx.printf (
        "\rM1:%5d->%5d   M2:%5d->%5d   GATE:%s DIR:0x%x",
        m1, motor.get_duty(0),
        m2, motor.get_duty(1),
        motor.get_gate() ? "ON " : "OFF",
        motor.get_swap_dir()
      );
    }

    oldmode = mode;
    oldm1 = m1;
    oldm2 = m2;
    oldgate = gate;
    oldsd = sd;

    UD5_WAIT (50);
  }
}
