/*!
 @file  sample10_EXIO_OUTPUT.cpp
 @brief EXIOによる信号出力 (プッシュプル)
 @note
  EXIOを出力端子として構成し、出力信号をコンソールでモニタする。
  tPinDOUTで初期化された場合、各端子は0の時に0V、1の時に3.3Vが出力される。
 */
#include <ud5.h>

CDXIF dx;
CEXIO exio;

//! main関数
int main (void) {
  exio.set_config (0, CEXIO::tPinDOUT);
  exio.set_config (1, CEXIO::tPinDOUT);
  exio.set_config (2, CEXIO::tPinDOUT);
  exio.set_config (3, CEXIO::tPinDOUT);
  exio.set_config (4, CEXIO::tPinDOUT);
  exio.set_config (5, CEXIO::tPinDOUT);
  exio.set_config (6, CEXIO::tPinDOUT);
  exio.set_config (7, CEXIO::tPinDOUT);
  exio.set_config (8, CEXIO::tPinDOUT);
  exio.set_config (9, CEXIO::tPinDOUT);
  exio.set_config (10, CEXIO::tPinDOUT);
  exio.set_config (11, CEXIO::tPinDOUT);
  exio.set_config (12, CEXIO::tPinDOUT);
  exio.set_config (13, CEXIO::tPinDOUT);
  exio.set_config (14, CEXIO::tPinDOUT);
  exio.set_config (15, CEXIO::tPinDOUT);

  int i = 0;
  while (1) {
    uint16_t v = i | i << 8;
    exio.set_gpio (v); // EXIOに出力
    dx.printf ("\rGPIO OUT:%04X IN:0x%04X \33[K", v, exio.get_gpio());
    if (++i > 0xff) i = 0;
    UD5_WAIT (50);
    if (dx.rxbuff()) break;
  }
}
