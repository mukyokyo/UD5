/*!
 @file  sample16_IIC_ENC.cpp
 @brief I2C通信 ENCODER
 @note
  STEMMA QT/Qwiicコネクタを使用してロータリーエンコーダボードと通信
  adafruit社のRotary Encoder Breakout with NeoPixelを想定
 */
#include <ud5.h>
#include <stdio.h>

CDXIF dx;

// I2Cポートを初期化
CI2C Wire(CI2C::IIC_Fmp);

//! エンコーダボードのデフォルトアドレス
#define ADDR_ENC (0x36)

//! 初期化
void init (void) {
/*
  encoder = rotaryio.IncrementalEncoder(seesaw)
//  seesaw.pin_mode(24, seesaw.INPUT_PULLUP)
  self._pin_mode_bulk_x(8, 4, 1<<24, INPUT_PULLUP)

#define _GPIO_BASE 0x01
#define _GPIO_DIRCLR_BULK 0x03
#define _GPIO_BULK_SET 0x05
#define _GPIO_PULLENSET 0x0B
      self.write(_GPIO_BASE, _GPIO_DIRCLR_BULK, cmd)
      self.write(_GPIO_BASE, _GPIO_PULLENSET, cmd)
      self.write(_GPIO_BASE, _GPIO_BULK_SET, cmd)

  switch = digitalio.DigitalIO(seesaw, 24)

  pixel = neopixel.NeoPixel(seesaw, 6, 1)

  pixel.brightness = 0.5

  last_position = -1
  color = 0  # start at red
*/
}

//! エンコーダ取り込み
uint8_t get_enc_data (void){
/*
  encoder.position
*/
  return 0;
}

//! RGB LED点灯
void set_led (uint8_t r, uint8_t g, uint8_t b) {
}

//! main関数
int main (void){
  uint8_t x, y;
  dx.puts("\n\r    0 1 2 3 4 5 6 7 8 9 A B C D E F\n\r");
  for (y=0;y<8;y++) {
    x = 0;
    dx.printf("%02X:",y*16+x);
    for (x=0;x<16;x++) {
      dx.printf(" %c", Wire.ping(y*16+x) ? 'X' : '-');
    }
    dx.puts("\n\r");
  }

  init ();
  if (Wire.ping(ADDR_ENC)){
    while (1) {
      dx.printf("enc :%5d", get_enc_data());

      if (dx.rxbuff()) { if(dx.getc() == '!') UD5_SOFTRESET(); }
      UD5_WAIT(20);
    }
  }
}
