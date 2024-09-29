/*!
 @file  sample4_GPIO_INPUT1.cpp
 @brief GPIO/EXIOによる信号入力 (プルアップなし)
 @note
  GPIO/EXIOを入力端子として構成し、入力信号をコンソールでモニタする。
 */
#include <ud5.h>

CDXIF dx;
CGPIO gpio;
CEXIO exio;

//! main関数
int main (void) {
  // GPIOの初期化
  gpio.set_config (0, CGPIO::tPinDIN);
  gpio.set_config (1, CGPIO::tPinDIN);
  gpio.set_config (2, CGPIO::tPinDIN);
  gpio.set_config (3, CGPIO::tPinDIN);
  gpio.set_config (4, CGPIO::tPinDIN);
  gpio.set_config (5, CGPIO::tPinDIN);
  gpio.set_config (6, CGPIO::tPinDIN);
  gpio.set_config (7, CGPIO::tPinDIN);
  gpio.set_config (8, CGPIO::tPinDIN);
  gpio.set_config (9, CGPIO::tPinDIN);

  // EXIO0～15の初期化
  exio.set_config (0, CEXIO::tPinDIN);
  exio.set_config (1, CEXIO::tPinDIN);
  exio.set_config (2, CEXIO::tPinDIN);
  exio.set_config (3, CEXIO::tPinDIN);
  exio.set_config (4, CEXIO::tPinDIN);
  exio.set_config (5, CEXIO::tPinDIN);
  exio.set_config (6, CEXIO::tPinDIN);
  exio.set_config (7, CEXIO::tPinDIN);
  exio.set_config (8, CEXIO::tPinDIN);
  exio.set_config (9, CEXIO::tPinDIN);
  exio.set_config (10, CEXIO::tPinDIN);
  exio.set_config (11, CEXIO::tPinDIN);
  exio.set_config (12, CEXIO::tPinDIN);
  exio.set_config (13, CEXIO::tPinDIN);
  exio.set_config (14, CEXIO::tPinDIN);
  exio.set_config (15, CEXIO::tPinDIN);

  while (1) {
    dx.printf ("\rGPIO[0x%03X] EXIO[0x%04X]\33[K", gpio.get_gpio(), exio.get_gpio());
    UD5_WAIT (50);
    if (dx.rxbuff()) break;
  }
}
