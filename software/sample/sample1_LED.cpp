/*!
 @file  sample1_LED.cpp
 @brief LED
 @note
  CEXIOに含まれるLEDを使って様々なパターンで明滅。
 */
#include <ud5.h>

//! LEDはCEXIOの機能
CEXIO exio;

//! main関数
int main (void) {
  exio.set_LED_off (0x3f);
  UD5_WAIT (1000);
  exio.set_LED_on (0x3f);
  UD5_WAIT (1000);
  for (int i = 0; i < 10; i++) {
    exio.set_LED_toggle (0x3f);
    UD5_WAIT (200);
  }
  for (int i = 0; i < 64; i++) {
    exio.set_LED (i);
    UD5_WAIT (200);
  }
  const uint8_t pattern[] = { 1, 2, 4, 8, 16, 32, 16, 8, 4, 2 };
  for (int i = 0; i < 50; i++) {
    exio.set_LED (pattern[i % 10]);
    UD5_WAIT (100);
  }
  exio.set_LED_off (0xf);
}
