/*!
 @file  sample18_TASK1.cpp
 @brief タスク
 @note
  メモリの許す限りではあるが複数のタスクが使用できる。

  各タスクの実行は1ms周期のタスクスイッチにて自動的に切り替わる。
  実行時間やタスクの優先度、負荷のバランスを考慮して、各タスク内のループにウェ
  イトを入れて置くことを推奨する。
  カーネルを起動すると登録したタスクが終了するまでmainはその時点で停止する。
 */
#include <ud5.h>

CDXIF dx;

CEXIO exio (
  (const CEXIO::TPinMode[16]) {
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
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU,
    CEXIO::tPinDIN_PU
  }
);

#define JP1 ((exio.get_gpio()&1)==0)
#define JP2 ((exio.get_gpio()&2)==0)
#define JP3 ((exio.get_gpio()&4)==0)

//! タスク1
//! コンソールに0.5秒周期で「.」を送信するタスク
void TASK1 (void *pvParameters) {
  while (1) {
    dx.putc ('.'); // 「.」送信
    UD5_WAIT (500);   // 0.5秒待ち
  }
}

//! タスク2
//! JP1をLOWに落とすとLEDを点灯させるタスク
void TASK2 (void *pvParameters) {
  while (1) {
    if (JP1) exio.set_LED_on (0xf);
    UD5_WAIT (10);    // 10ミリ秒待ち
  }
}

//! タスク3
//! JP2をLOWに落とすとLEDを消灯させるタスク
void TASK3 (void *pvParameters) {
  while (1) {
    if (JP2) exio.set_LED_off (0xf);
    UD5_WAIT (10);    // 10ミリ秒待ち
  }
}

//! タスク4
//! 2秒置きにLEDを点滅させるタスク
void TASK4 (void *pvParameters) {
  while (1) {
    exio.set_LED_toggle (0xf); // LEDの状態を反転
    UD5_WAIT (2000);   // 2秒待ち
  }
}

//! タスク5
//! DXL I/Fから入力された文字をエコーバックするタスク
void TASK5 (void *pvParameters) {
  while (1) {
    while (dx.rxbuff()) {
      char c = dx.getc();
      dx.putc (c);
      if (c == '!') UD5_SOFTRESET();
    }
    UD5_WAIT (10);
  }
}

//! main関数
int main (void) {
  // 各タスクを作成
  xTaskCreate (TASK1, NULL, 100, NULL, 1, NULL);
  xTaskCreate (TASK2, NULL, 100, NULL, 1, NULL);
  xTaskCreate (TASK3, NULL, 100, NULL, 1, NULL);
  xTaskCreate (TASK4, NULL, 100, NULL, 1, NULL);
  xTaskCreate (TASK5, NULL, 100, NULL, 1, NULL);
  // カーネル起動
  vTaskStartScheduler();
}
