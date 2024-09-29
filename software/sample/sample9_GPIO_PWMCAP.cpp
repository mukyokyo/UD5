/*!
 @file  sample9_GPIO_PWMCAP.cpp
 @brief GPIOによるパルス幅計測
 @note
  GPIOをパルス幅計測として構成し、計測値をコンソールでモニタする。
 */
#include <ud5.h>

CDXIF dx;
CGPIO gpio;

//! main関数
int main (void) {
  // GPIO0～7の初期化
  gpio.set_config (0, CGPIO::tPinINT0_MPW); // GPIO0をパルス幅測定0に割当
  gpio.set_config (1, CGPIO::tPinINT1_MPW); // GPIO1をパルス幅測定1に割当
  gpio.set_config (2, CGPIO::tPinINT2_MPW); // GPIO2をパルス幅測定2に割当
  gpio.set_config (3, CGPIO::tPinINT3_MPW); // GPIO3をパルス幅測定3に割当
  gpio.set_config (4, CGPIO::tPinINT4_MPW); // GPIO4をパルス幅測定4に割当
  gpio.set_config (5, CGPIO::tPinINT5_MPW); // GPIO5をパルス幅測定5に割当
  gpio.set_config (6, CGPIO::tPinINT6_MPW); // GPIO6をパルス幅測定6に割当
  gpio.set_config (7, CGPIO::tPinINT7_MPW); // GPIO7をパルス幅測定7に割当

  while (1) {
    dx.printf ("\rPWM CAP = [%5lu][%5lu][%5lu][%5lu][%5lu][%5lu][%5lu][%5lu]\33[K", gpio.get_pwd (0), gpio.get_pwd (1), gpio.get_pwd (2), gpio.get_pwd (3), gpio.get_pwd (4), gpio.get_pwd (5), gpio.get_pwd (6), gpio.get_pwd (7));
    UD5_WAIT (50);
    if (dx.rxbuff()) break;
  }
}
