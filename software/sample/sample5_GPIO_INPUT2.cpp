/*!
 @file  sample5_GPIO_INPUT2.cpp
 @brief GPIO/EXIOによる信号入力 (プルアップあり)
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
  gpio.set_config (0, CGPIO::tPinDIN_PU);
  gpio.set_config (1, CGPIO::tPinDIN_PU);
  gpio.set_config (2, CGPIO::tPinDIN_PU);
  gpio.set_config (3, CGPIO::tPinDIN_PU);
  gpio.set_config (4, CGPIO::tPinDIN_PU);
  gpio.set_config (5, CGPIO::tPinDIN_PU);
  gpio.set_config (6, CGPIO::tPinDIN_PU);
  gpio.set_config (7, CGPIO::tPinDIN_PU);
  gpio.set_config (8, CGPIO::tPinDIN_PU);
  gpio.set_config (9, CGPIO::tPinDIN_PU);

  // EXIO0～15の初期化
  exio.set_config (0, CEXIO::tPinDIN_PU);
  exio.set_config (1, CEXIO::tPinDIN_PU);
  exio.set_config (2, CEXIO::tPinDIN_PU);
  exio.set_config (3, CEXIO::tPinDIN_PU);
  exio.set_config (4, CEXIO::tPinDIN_PU);
  exio.set_config (5, CEXIO::tPinDIN_PU);
  exio.set_config (6, CEXIO::tPinDIN_PU);
  exio.set_config (7, CEXIO::tPinDIN_PU);
  exio.set_config (8, CEXIO::tPinDIN_PU);
  exio.set_config (9, CEXIO::tPinDIN_PU);
  exio.set_config (10, CEXIO::tPinDIN_PU);
  exio.set_config (11, CEXIO::tPinDIN_PU);
  exio.set_config (12, CEXIO::tPinDIN_PU);
  exio.set_config (13, CEXIO::tPinDIN_PU);
  exio.set_config (14, CEXIO::tPinDIN_PU);
  exio.set_config (15, CEXIO::tPinDIN_PU);

  while (1) {
    dx.printf ("\rGPIO[0x%03X] EXIO[0x%04X]\33[K", gpio.get_gpio(), exio.get_gpio());
    UD5_WAIT (50);
    if (dx.rxbuff()) break;
  }
}
