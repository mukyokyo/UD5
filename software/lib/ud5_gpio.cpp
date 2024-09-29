/*!
  @file    ud5_gpio.cpp
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
// GPIO
//=======================================================================
/*!
 @brief LPC845 GPIO class.
 @note
   The pins directly connected to the MCU can be used as GPIO/ADC/DAC.
   It also supports pulse width measurement and 2-phase encoder acquisition.
 */
inline uint8_t CGPIO::enc_phase (uint8_t ch) {
  return Chip_GPIO_GetPinState (LPC_GPIO_PORT, 0, PinInfo[enc_phase_pin[ch][0]].No) | (Chip_GPIO_GetPinState (LPC_GPIO_PORT, 0, PinInfo[enc_phase_pin[ch][1]].No) << 1);
}

CGPIO::CGPIO() {
  anchor = this;
  for (int i = 0; i < 8; i++) previous_mpw_gpiono[i] = 0;
  pulse_update_cnt = 0;
  // PININT
  Chip_Clock_EnablePeriphClock (SYSCON_CLOCK_GPIOINT);
  ADC_Init (10000, ADC_SEQ_CTRL_CHANSEL (2) | ADC_SEQ_CTRL_CHANSEL (3) | ADC_SEQ_CTRL_CHANSEL (4) | ADC_SEQ_CTRL_CHANSEL (5) | ADC_SEQ_CTRL_CHANSEL (6) | ADC_SEQ_CTRL_CHANSEL (7) | ADC_SEQ_CTRL_CHANSEL (8) | ADC_SEQ_CTRL_CHANSEL (9) | ADC_SEQ_CTRL_CHANSEL (10) | ADC_SEQ_CTRL_CHANSEL (11));
  PIO_Configure (pins, PIO_LISTSIZE (pins));
};

//! To initialize all terminals at once at instance
CGPIO::CGPIO (const TPinMode cfg[10]) {
  anchor = this;
  for (int i = 0; i < 8; i++) previous_mpw_gpiono[i] = 0;
  pulse_update_cnt = 0;
  // PININT
  Chip_Clock_EnablePeriphClock (SYSCON_CLOCK_GPIOINT);
  ADC_Init (10000, ADC_SEQ_CTRL_CHANSEL (2) | ADC_SEQ_CTRL_CHANSEL (3) | ADC_SEQ_CTRL_CHANSEL (4) | ADC_SEQ_CTRL_CHANSEL (5) | ADC_SEQ_CTRL_CHANSEL (6) | ADC_SEQ_CTRL_CHANSEL (7) | ADC_SEQ_CTRL_CHANSEL (8) | ADC_SEQ_CTRL_CHANSEL (9) | ADC_SEQ_CTRL_CHANSEL (10) | ADC_SEQ_CTRL_CHANSEL (11));

  for (int i = 0; i < 10; i++) set_config (i, cfg[i]);
};

CGPIO::~CGPIO() {
  for (int i = 0; i < 10; i++) NVIC_EnableIRQ ((IRQn_Type) (PININT0_IRQn + i));
  PIO_Configure (pins, PIO_LISTSIZE (pins));
}

//! PININT interrupt callback
void CGPIO::PIN_INT_cb (uint8_t ch) {
  if (Chip_PININT_GetIntStatus (LPC_PININT) & (1 << ch)) {
    Chip_PININT_ClearIntStatus (LPC_PININT, (1 << ch));
    int8_t n;
    switch (pinint_interruptor[ch] & 0xf0) {
      case 0: {   // Pulse width measurement
          uint32_t t = LPC_MRT_CH3->TIMER;  // Obtain 31-bit countdown value by 32 MHz (assuming MRT has already started)
          pulse_update_cnt++;
          // Up Edge
          if (Chip_GPIO_GetPinState (LPC_GPIO_PORT, 0, previous_mpw_gpiono[ch])) pwdup[ch] = t;
          // Down Edge (Ignore counter value overflow)
          else if (t < pwdup[ch]) pwd[ch] = pwdup[ch] - t;
          break;
        }
      case 0x10:  // Encoder ch 0
        enc_ind[0] = (enc_ind[0] << 2) | enc_phase (0);
        if ((n = enc_dirtable[enc_ind[0] & 15])) enc_polrev[0] ? (enc_counter[0] -= n) : (enc_counter[0] += n);
        break;
      case 0x20:  // Encoder ch 1
        enc_ind[1] = (enc_ind[1] << 2) | enc_phase (1);
        if ((n = enc_dirtable[enc_ind[1] & 15])) enc_polrev[1] ? (enc_counter[1] -= n) : (enc_counter[1] += n);
        break;
    }
  }
}

//! GPIO function setting
void CGPIO::set_config (uint8_t adch, TPinMode pm) {
  if (adch < 10) {
    TPin pin = { PinInfo[adch].No, PIO_TYPE_FIXED, 0, PIO_MODE_DEFAULT };
    int n;

    pin.gpio_no = PinInfo[adch].No;
    switch (pm) {
      // Digital input
      case tPinDIN:
        PIO_Configure (&pin, 1);
        pin.pin_type = PIO_TYPE_INPUT;
        pin.pin_mode = PIO_MODE_DEFAULT;    // high impedance
        break;
      case tPinDIN_PU:
        pin.pin_type = PIO_TYPE_INPUT;
        pin.pin_mode = PIO_MODE_PULLUP;     // pull-up
        break;
      case tPinDIN_PD:
        pin.pin_type = PIO_TYPE_INPUT;
        pin.pin_mode = PIO_MODE_PULLDOWN;   // pull-down
        break;
      // Digital output
      case tPinDOUT:
        pin.pin_type = PIO_TYPE_OUTPUT;
        pin.pin_mode = PIO_MODE_DEFAULT;    // push-pull
        break;
      case tPinDOUT_OD:
        pin.pin_type = PIO_TYPE_OUTPUT;
        pin.pin_mode = PIO_MODE_OPENDRAIN;  // open drain
        break;

      // Analog input
      case tPinAIN:
        pin.pin_type = PIO_TYPE_FIXED;
        pin.pin_movable_fixed = PinInfo[adch].AD;
        pin.pin_mode = PIO_MODE_DEFAULT;    // high impedance
        break;

      // Pulse Width Measurement
      case tPinINT0_MPW ... tPinINT7_MPW:
        n = pm - tPinINT0_MPW;
        pin.pin_type = PIO_TYPE_INPUT;
        pin.pin_mode = PIO_MODE_PULLUP;     // pull-up
        Chip_SYSCON_SetPinInterrupt (n, pin.gpio_no);
        previous_mpw_gpiono[n] = pin.gpio_no;
        pwd[n] = 0;

        Chip_PININT_ClearIntStatus (LPC_PININT, 1 << n); // clear INT status
        Chip_PININT_EnableRisingEdgeOrLevel (LPC_PININT, (1 << n) | LPC_PININT->SIENR);  // Rising edge
        Chip_PININT_EnableFallingEdgeOrLevel (LPC_PININT, (1 << n) | LPC_PININT->SIENF); // Falling edge
        NVIC_EnableIRQ ((IRQn_Type) (PININT0_IRQn + n));
        pinint_interruptor[n] = n;
        break;

      // Encoder Ch 0 phase A
      case tPinINT0_ENC0A ... tPinINT7_ENC0A:
        n = pm - tPinINT0_ENC0A;
        pin.pin_type = PIO_TYPE_INPUT;
        pin.pin_mode = PIO_MODE_PULLUP;     // pull-up
        Chip_SYSCON_SetPinInterrupt (n, pin.gpio_no);
        enc_phase_pin[0][0] = adch;

        Chip_PININT_ClearIntStatus (LPC_PININT, 1 << n); // clear INT status
        Chip_PININT_EnableRisingEdgeOrLevel (LPC_PININT, (1 << n) | LPC_PININT->SIENR);  // Rising edge
        Chip_PININT_EnableFallingEdgeOrLevel (LPC_PININT, (1 << n) | LPC_PININT->SIENF); // Falling edge
        NVIC_EnableIRQ ((IRQn_Type) (PININT0_IRQn + n));
        pinint_interruptor[n] = 0x10 | n;
        break;
      // Encoder Ch 0 phase B
      case tPinINT0_ENC0B ... tPinINT7_ENC0B:
        n = pm - tPinINT0_ENC0B;
        pin.pin_type = PIO_TYPE_INPUT;
        pin.pin_mode = PIO_MODE_PULLUP;     // pull-up
        Chip_SYSCON_SetPinInterrupt (n, pin.gpio_no);
        enc_phase_pin[0][1] = adch;

        Chip_PININT_ClearIntStatus (LPC_PININT, 1 << n); // clear INT status
        Chip_PININT_EnableRisingEdgeOrLevel (LPC_PININT, (1 << n) | LPC_PININT->SIENR);  // Rising edge
        Chip_PININT_EnableFallingEdgeOrLevel (LPC_PININT, (1 << n) | LPC_PININT->SIENF); // Falling edge
        NVIC_EnableIRQ ((IRQn_Type) (PININT0_IRQn + n));
        pinint_interruptor[n] = 0x10 | n;
        break;

      // Encoder Ch 1 phase A
      case tPinINT0_ENC1A ... tPinINT7_ENC1A:
        n = pm - tPinINT0_ENC1A;
        pin.pin_type = PIO_TYPE_INPUT;
        pin.pin_mode = PIO_MODE_PULLUP;     // pull-up
        Chip_SYSCON_SetPinInterrupt (n, pin.gpio_no);
        enc_phase_pin[1][0] = adch;

        Chip_PININT_ClearIntStatus (LPC_PININT, 1 << n); // clear INT status
        Chip_PININT_EnableRisingEdgeOrLevel (LPC_PININT, (1 << n) | LPC_PININT->SIENR);  // Rising edge
        Chip_PININT_EnableFallingEdgeOrLevel (LPC_PININT, (1 << n) | LPC_PININT->SIENF); // Falling edge
        NVIC_EnableIRQ ((IRQn_Type) (PININT0_IRQn + n));
        pinint_interruptor[n] = 0x20 | n;
        break;
      // Encoder Ch 1 phase B
      case tPinINT0_ENC1B ... tPinINT7_ENC1B:
        n = pm - tPinINT0_ENC1B;
        pin.pin_type = PIO_TYPE_INPUT;
        pin.pin_mode = PIO_MODE_PULLUP;     // pull-up
        Chip_SYSCON_SetPinInterrupt (n, pin.gpio_no);
        enc_phase_pin[1][1] = adch;

        Chip_PININT_ClearIntStatus (LPC_PININT, 1 << n); // clear INT status
        Chip_PININT_EnableRisingEdgeOrLevel (LPC_PININT, (1 << n) | LPC_PININT->SIENR);  // Rising edge
        Chip_PININT_EnableFallingEdgeOrLevel (LPC_PININT, (1 << n) | LPC_PININT->SIENF); // Falling edge
        NVIC_EnableIRQ ((IRQn_Type) (PININT0_IRQn + n));
        pinint_interruptor[n] = 0x20 | n;
        break;

      // SPI
      case tPinSPISEL:
        pin.pin_type = PIO_TYPE_MOVABLE;
        pin.pin_movable_fixed = SWM_SPI0_SSEL0_IO;
        pin.pin_mode = PIO_MODE_DEFAULT;
        break;
      case tPinSPISCK:
        pin.pin_type = PIO_TYPE_MOVABLE;
        pin.pin_movable_fixed = SWM_SPI0_SCK_IO;
        pin.pin_mode = PIO_MODE_DEFAULT;
        break;
      case tPinSPIMISO:
        pin.pin_type = PIO_TYPE_MOVABLE;
        pin.pin_movable_fixed = SWM_SPI0_MISO_IO;
        ///      pin.pin_mode = PIN_MODE_PULLUP;
        break;
      case tPinSPIMOSI:
        pin.pin_type = PIO_TYPE_MOVABLE;
        pin.pin_movable_fixed = SWM_SPI0_MOSI_IO;
        pin.pin_mode = PIO_MODE_DEFAULT;
        break;

      // Others
      case tPinSPFUNC:
        break;
    }
    if (pm != tPinSPFUNC) PIO_Configure (&pin, 1);
  }
}

//! Get igital input (valid range 10bit)
uint16_t CGPIO::get_gpio (void) {
  uint16_t w = 0;
  for (int i = 0; i <= 9; i++) w |= (Chip_GPIO_GetPinState (LPC_GPIO_PORT, 0, PinInfo[i].No) ? (1 << i) : 0);
  return w;
}

//! Set digital output (valid range 10bit)
void CGPIO::set_gpio (uint16_t out) {
  for (int i = 0; i <= 9; i++) Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, PinInfo[i].No, out & (1 << i) ? true : false);
}

//! Get ADCn result (ch:0...9)
uint16_t CGPIO::get_adc (uint8_t ch) {
  if (ch <= 9) return ADC_Get (ch + 2);
  return 0;
}

//! Get pulse width measurement (ch: 0...7)
uint32_t CGPIO::get_pwd (uint8_t ch) {
  if (ch <= 7) return pwd[ch];
  return 0;
}

uint32_t CGPIO::get_pulse_update_cnt (void) {
  return pulse_update_cnt;
}

int32_t CGPIO::get_ENC_rawcounter (uint8_t ch) {
  if (ch > 1) return 0;
  return enc_counter[ch];
}

int32_t CGPIO::get_encoder_count (uint8_t ch) {
  if (ch > 1) return 0;
  return enc_counter[ch] - enc_offset[ch];
}

void CGPIO::reset_encoder_count (uint8_t ch) {
  if (ch > 1) return;
  enc_offset[ch] = enc_counter[ch];
}

CGPIO *CGPIO::anchor = NULL;

// PININT interrupt routine for pulse width measurement
//-----------------------------------
//! Interrupt handler for PININT0 @note Call from CGPIO.
extern "C" void PIN_INT0_IRQHandler (void) {
  CGPIO::anchor->PIN_INT_cb (0);
}

//! Interrupt handler for PININT1 @note Call from CGPIO.
extern "C" void PIN_INT1_IRQHandler (void) {
  CGPIO::anchor->PIN_INT_cb (1);
}

//! Interrupt handler for PININT2 @note Call from CGPIO.
extern "C" void PIN_INT2_IRQHandler (void) {
  CGPIO::anchor->PIN_INT_cb (2);
}

//! Interrupt handler for PININT3 @note Call from CGPIO.
extern "C" void PIN_INT3_IRQHandler (void) {
  CGPIO::anchor->PIN_INT_cb (3);
}

//! Interrupt handler for PININT4 @note Call from CGPIO.
extern "C" void PIN_INT4_IRQHandler (void) {
  CGPIO::anchor->PIN_INT_cb (4);
}

//! Interrupt handler for PININT5 @note Call from CGPIO.
extern "C" void PIN_INT5_IRQHandler (void) {
  CGPIO::anchor->PIN_INT_cb (5);
}

//! Interrupt handler for PININT6 @note Call from CGPIO.
extern "C" void PIN_INT6_IRQHandler (void) {
  CGPIO::anchor->PIN_INT_cb (6);
}

//! Interrupt handler for PININT7 @note Call from CGPIO.
extern "C" void PIN_INT7_IRQHandler (void) {
  CGPIO::anchor->PIN_INT_cb (7);
}
