/*!
 @file  sample2_DXLIF.cpp
 @brief コンソール入出力
 @note
  DXL I/Fにてシリアル通信を行う。
  ついでにLEDも明滅。
 */
#include <ud5.h>

CDXIF dx;

CEXIO exio;

int main (void) {
  int i = 0;
  char c;
  while (1) {
    exio.set_LED_toggle (0x1);  // LED1の状態を反転
    if (dx.rxbuff() > 0) {      // 受信バッファにあるデータ数を確認
      i++;
      c = dx.getc();            // 受信バッファから1バイト取り出す
      switch (c) {
        case '!': // ソフトリセット
          UD5_SOFTRESET();
          break;
        case '1': // 文字列の送信
          dx.puts ("[1 key pushed]");
          break;
        case '2': // 書式化文字列の送信
          dx.printf ("[i=%d]", i);
          break;
        case '3': // 書式化文字列の入力
          dx.printf ("\nNOW i=%d\nINPUT NUMBER=", i);
          dx.scanf ("%d", &i);
          dx.printf ("\nNEW i=%d\n", i);
          break;
        case '4': // LED2点灯
          dx.puts ("\33[>5lon");
          exio.set_LED_on (0x2);
          break;
        case '5': // LED2消灯
          dx.puts ("\33[>5hoff");
          exio.set_LED_off (0x2);;
          break;
        default:  // エコーバック
          dx.putc (c);
          break;
      }
    }
    UD5_WAIT (50);
  }
}
