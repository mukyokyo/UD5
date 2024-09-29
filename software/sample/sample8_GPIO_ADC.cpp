/*!
 @file  sample8_GPIO_ADC.cpp
 @brief GPIOによるアナログ計測
 @note
  GPIOをアナログ入力端子として構成し、計測値をコンソールでモニタする。
  アナログ入力をサポートするのはGPIO0～GPIO7の8チャネルのみであり、0～3.3Vの範囲を
  12bitの分解能で測定する。
 */
#include <ud5.h>

//! こういう書き方でも初期ができる
CGPIO gpio (
  (const CGPIO::TPinMode[10]) {
    CGPIO::tPinAIN,
    CGPIO::tPinAIN,
    CGPIO::tPinAIN,
    CGPIO::tPinAIN,
    CGPIO::tPinAIN,
    CGPIO::tPinAIN,
    CGPIO::tPinAIN,
    CGPIO::tPinAIN,
    CGPIO::tPinAIN,
    CGPIO::tPinAIN,
  }
);

CDXIF dx;

//! main関数
int main (void) {
  while (1) {
    dx.puts ("\rADC = ");
    for (int i = 0; i < 9; i++) dx.printf ("%d:%4d ", i, gpio.get_adc (i));
    dx.puts ("\33[K");
    UD5_WAIT (50);

    if (dx.rxbuff()) {
      if (dx.getc() == '!') UD5_SOFTRESET();
    }
  }
}
