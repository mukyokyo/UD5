/*!
 @file  sample11_RC_STICK.cpp
 @brief ラジコン受信機取り込みサンプル
 @note
  GPIOをパルス幅計測端子として構成しラジコン受信機のパルスを測定
 */
#include  <ud5.h>

CGPIO gpio (
  (const CGPIO::TPinMode[10]) {
    CGPIO::tPinINT0_MPW,
    CGPIO::tPinINT1_MPW,
    CGPIO::tPinINT2_MPW,
    CGPIO::tPinINT3_MPW,
    CGPIO::tPinINT4_MPW,
    CGPIO::tPinINT5_MPW,
    CGPIO::tPinINT6_MPW,
    CGPIO::tPinINT7_MPW,
    CGPIO::tPinDIN_PU,
    CGPIO::tPinDIN_PU,
  }
);

CEXIO exio;
//! プロポの処理クラス
CRCStick rcstick (&gpio);
CDXIF dx;

//! キャリブレーション処理遷移条件
bool next_step (void) {
  if (dx.rxbuff()) {
    dx.clear_rxbuff();
    return true;
  }
  return false;
}

//! LED明滅パターン処理
void led (uint8_t n) {
  switch (n) {
    case 0:
      exio.set_LED_off (0xf);
      break;
    case 1:
      exio.set_LED_on (0xf);
      break;
    case 2:
      exio.set_LED_toggle (0xf);
      break;
  }
}

//! main関数
int main (void) {
  int32_t cap, conv;

  while (1) {
    dx.puts ("\rMPW = ");
    int  i;
    // 8ch分のパルス幅測定結果を表示
    for (i = 0; i < 8; i++) {
      cap = gpio.get_pwd (i);
      conv = rcstick.get_normal (i, 0, 0); // 測定結果を-1000～1000に変換
      dx.printf ("%d[%5ld:%5ld] ", i, cap, conv);
    }
    dx.printf ("\33[K");

    if (dx.rxbuff()) {
      switch (dx.getc()) {
        case '!': // プログラム終了
          UD5_SOFTRESET();
          break;
        case 'c': // キャリブレーション
          rcstick.calibration (next_step);
          break;
      }
    }
    UD5_WAIT (50);
  }
}
