/*!
  @file    ud5_motor.cpp
  @version 0.9981
  @brief   Collection of classes for UD5 control
  @date    2024/9/29
  @author  T.Uemitsu

  @copyright
    Copyright (c) BestTechnology CO.,LTD. 2024
    All rights reserved.

  @par
   The software is designed to use the minimum number of
   functions provided by UD5.
   Although it should be provided in the form of a library,
   it is provided in the form of a header file in order to
   lay aside the complexity of its introduction.
 */

#include "ud5.h"

//=======================================================================
// Motor Driver
//=======================================================================
/*!
 @brief PWM control for 2c DCM class.
 @note
  DC motor connected to a 2-channel H-bridge is operated with slow decay.
 */
inline bool CMotor::get_dirpin (int ch) {
  return Chip_GPIO_GetPinState (LPC_GPIO_PORT, 0, (ch == 0) ? _M0DIR : _M1DIR);
}

inline void CMotor::set_dirpin (int ch, bool on) {
  Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, (ch == 0) ? _M0DIR : _M1DIR, on);
}

void CMotor::set_pwm_frequency (uint16_t freq) {
  static uint16_t oldfreq = 0;

  if (freq == 0) {
    Chip_SCTPWM_Stop (LPC_SCT);
    Chip_SCT_DeInit (LPC_SCT);
  } else {
    if (oldfreq != freq) {
      Chip_SCT_Init (LPC_SCT);
      Chip_SCTPWM_SetRate (LPC_SCT, freq);
    }
  }
  oldfreq = freq;
}

void CMotor::set_pwm_duty (uint8_t ch, int16_t duty) {
  uint32_t d;
  uint16_t absduty;

  duty = MIN (MAX (duty, - (int16_t)_max_abs_duty[ch]), (int16_t)_max_abs_duty[ch]);
  absduty = abs (duty);

  Chip_SCTPWM_SetOutPin (LPC_SCT, ch + 1, ch);
  if (absduty == 0)         d = 0;
  else if (absduty == 1000) d = Chip_SCTPWM_GetTicksPerCycle (LPC_SCT) + 1;
  else                      d = ((uint64_t)absduty * (uint64_t)Chip_SCTPWM_GetTicksPerCycle (LPC_SCT)) / (uint64_t) (1000UL);
  Chip_SCTPWM_SetDutyCycle (LPC_SCT, ch + 1, d);
  Chip_SCTPWM_Start (LPC_SCT);
}

// freqHz  :0...50000 Hz
// dir     :0=default, 1=Invert m0  2=Invert m1, 3=Invert m0 & m1 (2bit:Swap m0 and m1)
// maxduty :0...1000 ‰
CMotor::CMotor (int freqHz, uint8_t swap_dir, const uint16_t maxduty[2]) {
  PIO_Configure (pins, PIO_LISTSIZE (pins));

  Chip_SCTPWM_Init (LPC_SCT);
  set_pwm_frequency (freqHz);
  _swap_dir = swap_dir & 0x07;

  for (int i = 0; i < 2; i++) _max_abs_duty[i] = MIN (MAX (maxduty[i], 0), 1000);
  Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, _MEN, false);
  set_pwm_duty (0, 0);
  set_pwm_duty (1, 0);
  _duty[0] = _duty[1] = 0;
}

CMotor::CMotor() {
  PIO_Configure (pins, PIO_LISTSIZE (pins));

  Chip_SCTPWM_Init (LPC_SCT);
  set_pwm_frequency (50000);
  _swap_dir = 0;
  _max_abs_duty[0] = _max_abs_duty[1] = 1000;
  Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, _MEN, false);
  set_pwm_duty (0, 0);
  set_pwm_duty (1, 0);
  _duty[0] = _duty[1] = 0;
}

//! Set default sign and channel swap.
// dir :0=default, 1=Invert m0  2=Invert m1, 3=Invert m0 & m1 (2bit:Swap m0 and m1)
void CMotor::set_swap_dir (uint8_t dir) {
  _swap_dir = dir & 0x7;
}
//! Get current sign and channel swap.
uint8_t CMotor::get_swap_dir (void) {
  return _swap_dir;
}

//! Set PWM carrier frequency.
// freqHz :0...50000 Hz
void CMotor::set_freq (uint32_t freqHz) {
  set_pwm_frequency (freqHz);
}

//! Set maximum PWM duty.
// maxduty:0...1000 ‰
void CMotor::set_max_duty (uint16_t m0, uint16_t m1) {
  _max_abs_duty[0] = MIN (MAX (m0, 0), 1000);
  _max_abs_duty[1] = MIN (MAX (m1, 0), 1000);
}

//! Get current gate state
bool CMotor::get_gate (void) {
  return Chip_GPIO_GetPinState (LPC_GPIO_PORT, 0, _MEN);
}
//! Turn gate on/off
void CMotor::set_gate (bool on) {
  bool prevon = get_gate();
  if (!on) {
    set_pwm_duty (0, 0);
    set_pwm_duty (1, 0);
    _duty[0] = _duty[1] = 0;
  }
  Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, _MEN, on);
  // Avoid point arc failure immediately after gate driver enable
  if (!prevon && on) {
    set_pwm_duty (0, 50);
    set_pwm_duty (1, 50);
    for (int i = 0; i < 10; i++) {
      Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, _M0DIR, 1);
      Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, _M1DIR, 1);
      _wait.us (10);
      Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, _M0DIR, 0);
      Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, _M1DIR, 0);
      _wait.us (10);
    }
    set_pwm_duty (0, 0);
    set_pwm_duty (1, 0);
  }
}

//! Set PWM duty to each channel.
// m0, m1: -1000...1000 ‰
void CMotor::set_duty (uint8_t ch, int16_t duty_permil) {
  ch = (_swap_dir & 4) ? ((ch == 0) ? 1 : 0) : ch;
  if (ch > 1) return;
  if (get_gate()) {
    _duty[ch] = duty_permil;
    bool prevdir = get_dirpin (ch);
    duty_permil = (_swap_dir & (1 << ch)) ? -duty_permil : duty_permil;
    if ((duty_permil >= 0 && !prevdir) || (duty_permil < 0 && prevdir)) {
      set_pwm_duty (ch, 0);
      _wait.us (20);
    }
    set_dirpin (ch, duty_permil >= 0 ? 1 : 0);
    set_pwm_duty (ch, duty_permil);
  } else
    _duty[ch] = 0;
}

//! Set PWM duty to biaxial.
// m0, m1: -1000...1000 ‰
void CMotor::set_biaxial_dual (int16_t m0, int16_t m1) {
  set_duty (0, m0);
  set_duty (1, m1);
}
//! Set PWM duty to biaxial.
// m0, m1: -1000...1000 ‰
void CMotor::set_biaxial_duty (int16_t d[2]) {
  set_duty (0, d[0]);
  set_duty (1, d[1]);
}

//! Get current PWM duty for each channel.
// m0, m1: -1000...1000 ‰
int16_t CMotor::get_duty (uint8_t ch) {
  ch = (_swap_dir & 4) ? ((ch == 0) ? 1 : 0) : ch;
  if (ch > 1) return 0;
  return _duty[ch];
}
// Get current PWM duty for biaxial.
void CMotor::get_biaxial_duty (int16_t d[2]) {
  if (_swap_dir & 4) {
    d[0] = _duty[1];
    d[1] = _duty[0];
  } else {
    d[0] = _duty[0];
    d[1] = _duty[1];
  }
}
