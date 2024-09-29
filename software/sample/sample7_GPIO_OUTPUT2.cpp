/*!
 @file  sample7_GPIO_OUTPUT2.cpp
 @brief GPIOによる信号出力 (プッシュプル)
 @note
  GPIOを出力端子として構成し、出力信号をコンソールでモニタする。
  tPinDOUTで初期化された場合、各端子は0の時に0V、1の時に3.3Vが出力される。
 */
#include <ud5.h>

CDXIF dx;
CGPIO gpio;

//! main関数
int main (void) {
  gpio.set_config (0, CGPIO::tPinDOUT);
  gpio.set_config (1, CGPIO::tPinDOUT);
  gpio.set_config (2, CGPIO::tPinDOUT);
  gpio.set_config (3, CGPIO::tPinDOUT);
  gpio.set_config (4, CGPIO::tPinDOUT);
  gpio.set_config (5, CGPIO::tPinDOUT);
  gpio.set_config (6, CGPIO::tPinDOUT);
  gpio.set_config (7, CGPIO::tPinDOUT);
  gpio.set_config (8, CGPIO::tPinDOUT);
  gpio.set_config (9, CGPIO::tPinDOUT);

  int i = 0;
  while (1) {
    gpio.set_gpio (i); // GPIOに出力
    dx.printf ("\rGPIO OUT:0x%03X IN:0x%03X \33[K", i, gpio.get_gpio());
    if (++i > 0x3FF) i = 0;
    UD5_WAIT (50);
    if (dx.rxbuff()) break;
  }
}
