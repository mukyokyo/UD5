/*!
 @file  sample3_RASPIIF.cpp
 @brief コンソール入出力
 @note
  RasPi I/Fにてシリアル通信を行う。
  ボーレート:1Mbps、送信バッファ100byte、受信バッファ300byteで初期化。
  単純にエコーバックするのみ。
  プログラムを終了させるためにのみDXIFを使用。
 */
#include <ud5.h>

CRPiIF rpi(1000000, USIP_8N1, 100, 300);
CDXIF dxi;

int main (void) {
  while (1) {
    while (rpi.rxbuff() > 0) {
      rpi.putc(rpi.getc());
    }

    if (dxi.rxbuff() > 0) {
      if (dxi.getc() == '!') UD5_SOFTRESET();
    }
  }
}
