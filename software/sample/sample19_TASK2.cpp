/*!
 @file  sample19_TASK2.cpp
 @brief セマフォ
 @note
  排他的に使用しなくてはならない事象があればセマフォを使って排他区間を設置する。
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
#define DIP ((~exio.get_gpio()&0xc)>>2)

//! セマフォ
SemaphoreHandle_t Sem1 = NULL;

//! タスクハンドラ
xTaskHandle xHandle;

//! タスク1
//! LEDの点滅タスク
void TASK1 (void *pvParameters) {
  while (1) {
    // セマフォ獲得のタイムアウトなし
    if (DIP == 0) {

      if (Sem1 != NULL) {
        // セマフォの獲得待ち
        if (xSemaphoreTake (Sem1, portMAX_DELAY) == pdTRUE) {

          exio.set_LED_toggle (0x1);     // LED1反転

          // セマフォの返却
          xSemaphoreGive (Sem1);
        }
      }

      // セマフォ獲得のタイムアウトあり
    } else {
      if (Sem1 != NULL) {
        // 1秒間のセマフォの獲得待ち
        if (xSemaphoreTake (Sem1, 1000) == pdTRUE) {

          exio.set_LED_toggle (0x1);     // LED1反転

          // セマフォの返却
          xSemaphoreGive (Sem1);
        }
      }
    }
    dx.putc ('.');   // '.'送信

    // ソフトリセット
    if (dx.rxbuff() > 0) {
      if (dx.getc() == '!') UD5_SOFTRESET();
    }

    UD5_WAIT (200);       // 0.2秒待ち
  }
}

//! タスク2
//! JP1によるLED点灯タスク
void TASK2 (void *pvParameters) {
  while (1) {
    if (JP1) {

      if (Sem1 != NULL) {
        // セマフォの獲得待ち
        if (xSemaphoreTake (Sem1, portMAX_DELAY) == pdTRUE) {

          exio.set_LED_on (0x01);      // LED1点灯
          while (JP1) UD5_WAIT (5);

          // セマフォの返却
          xSemaphoreGive (Sem1);
        }
      }
    }

    UD5_WAIT (5); // 5m秒待ち
  }
}

//! タスク3
//! JP2によるLED消灯タスク
void TASK3 (void *pvParameters) {
  while (1) {
    if (JP2) {

      if (Sem1 != NULL) {
        // セマフォの獲得待ち
        if (xSemaphoreTake (Sem1, portMAX_DELAY) == pdTRUE) {

          exio.set_LED_off (0x1);      // LED1消灯
          while (JP2) UD5_WAIT (5);

          // セマフォの返却
          xSemaphoreGive (Sem1);
        }
      }
    }

    UD5_WAIT (5); // 5m秒待ち
  }
}

//! main関数
int main (void) {
  // セマフォ作成
  Sem1 = xSemaphoreCreateMutex();

  // 各タスクを作成
  xTaskCreate (TASK1, NULL, 100, NULL, 1, NULL);
  xTaskCreate (TASK2, NULL, 100, NULL, 1, NULL);
  xTaskCreate (TASK3, NULL, 100, NULL, 1, NULL);
  // カーネル起動
  vTaskStartScheduler();
}
