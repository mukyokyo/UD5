/*!
  @file    ud5.h
  @version 0.998
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

#pragma once

// FreeRTOS
#include  <FreeRTOS.h>
#include  <task.h>
#include  <semphr.h>

// LPC845 LPCOpen library
#include  <chip.h>
#include  <us.h>
#include  <piocfg.h>

// LPC845 Extras library for GDL
#include  <wl_sem.h>
#include  <wl_spi.h>
#include  <wl_adc.h>
#include  <wl_flash.h>

#include  <sys/types.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <limits.h>
#include  <math.h>

#include  <vector>

//=======================================================================
// Macro definitions and other functions
//=======================================================================

//! Embedded macro for the specified file in the current
//! Data body by symbol name, byte size of data prefixed with “_size_” to symbol name, each variable generated together
#define INCBIN(file, sym) asm (\
   ".section .rodata\n"                  \
   ".balign 4\n"                         \
   ".global " #sym "\n"                  \
   #sym ":\n"                            \
   ".incbin \"" file "\"\n"              \
   ".global _size_" #sym "\n"            \
   ".set _size_" #sym ", . - " #sym "\n" \
   ".balign 4\n"                         \
   ".section \".text\"\n");              \
   extern const void *sym; \
   extern const size_t *_size_##sym;

// Memory copying
extern "C" void __aeabi_memcpy8 (void *dest, const void *src, size_t n);
extern "C" void __aeabi_memcpy4 (void *dest, const void *src, size_t n);
extern "C" void __aeabi_memcpy (void *dest, const void *src, size_t n);

// Memory clearing and setting
extern "C" void __aeabi_memset8 (void *dest, size_t n, int c);
extern "C" void __aeabi_memset4 (void *dest, size_t n, int c);
extern "C" void __aeabi_memset (void *dest, size_t n, int c);

extern "C" void __aeabi_memclr8 (void *dest, size_t n);
extern "C" void __aeabi_memclr4 (void *dest, size_t n);
extern "C" void __aeabi_memclr (void *dest, size_t n);

//! Soft reset (bootloader command mode is activated after reset)
inline void sys_reset (void) {
  NVIC_SystemReset();
}

//! Excite dispatch
void _my_csw (void);

//=======================================================================
// WAIT
//=======================================================================
/*!
 @brief Time waiting and elapsed time measurement class
 @note
   Newly established without wl_delay because we want to use MRT for other purposes.
   Also instantiated as _wait early
 @note
   Can be used with or without FreeRTOS.
 */
class CWait {
  const uint32_t _USTICK = (32000000UL / 1000000UL);
  const uint32_t _MSTICK = (32000000UL / 1000UL);

  void (*csw) (void);

  volatile uint64_t mymstick;
  volatile uint32_t oldreg;
  uint32_t sem;

  uint32_t get_cyccnt_diff (void);

 public:

  CWait();

  void no_csw (void);

  //! Wait for microsecond.
  void us (uint32_t us);

  //! Wait for milliseconds.
  void ms (uint32_t ms);

  //! Get elapsed time in milliseconds since immediately after startup.
  uint32_t get_tick (void);
};

extern CWait _wait;

//=======================================================================
// ETC
//=======================================================================
//! Wait for milliseconds.
extern void UD5_WAIT (uint32_t ms);

//! Get elapsed time in milliseconds since immediately after startup.
extern uint32_t UD5_GET_ELAPSEDTIME (void);

//! Return to bootloader.
inline void UD5_SOFTRESET (void) {
  sys_reset();
}

//! Transition to low power consumption mode (return by interrupt).
inline void UD5_LOWPOWERMODE (void) {
  __WFI();
}

//=======================================================================
// UART
//=======================================================================
/*!
 @brief UART wrapper class for communication with Raspberry Pi
 @note
   Call LPC845 DMA version UART0 library.
 */
class CRPiIF {
  const uint8_t _RPi_RX = 12;
  const uint8_t _RPi_TX = 28;

  const TPin pins[2] = {
    { _RPi_TX,PIO_TYPE_MOVABLE, SWM_U0_TXD_O, PIO_MODE_DEFAULT },
    { _RPi_RX,PIO_TYPE_MOVABLE, SWM_U0_RXD_I, PIO_MODE_PULLUP  },
  };
  bool init = false;
  uint8_t *ptxbuff, *prxbuff;

 public:

  static CRPiIF *anchor;

  //! When you provide your own non-standard protocols and send/receive buffers.
  CRPiIF (uint32_t baud, uint8_t mode, uint16_t txb, uint16_t rxb);
  ~CRPiIF();

  //! Number of unsent data bytes
  uint16_t txbuff (void);
  //! Number of data bytes received
  uint16_t rxbuff (void);
  //! Clear transmit buffer.
  void clear_txbuff (void);
  //! Clear Received data.
  void clear_rxbuff (void);
  //! Send 1 byte of characters.
  void putc (char c);
  //! Send string.
  void puts (const char *s);
  //! Send specified number of bytes of data.
  int putsb (const uint8_t *s, int n);
  //! Retrieve one character from the receive data buffer.
  char getc (void);
  //! Receive string.
  uint16_t gets (char *s, int n);

#if 1
  //! Send format conversion strings
  template <typename ... Args>
  int printf (const char *format, Args const &... args) {
    if (init) return usart0_dma_printf (format, args ...);
    return 0;
  }
  //! Receive format conversion strings
  template <typename ... Args>
  int scanf (const char *format, Args const &... args) {
    if (init) return usart0_dma_scanf (format, args ...);
    return 0;
  }
#endif
  int (*pprintf()) (const char *, ...);

  void no_csw (void);
};

/*!
 @brief UART wrapper class converted to half-duplex I/F.
 @note
   Call LPC845 DMA version UART1 library.
 */
class CDXIF {
  const uint8_t _TTL_TX  = 27;
  const uint8_t _TTL_RX  = 25;
  const uint8_t _TTL_RTS = 26;

  const TPin pins[3] = {
    { _TTL_TX,  PIO_TYPE_MOVABLE, SWM_U1_TXD_O, PIO_MODE_DEFAULT },
    { _TTL_RX,  PIO_TYPE_MOVABLE, SWM_U1_RXD_I, PIO_MODE_PULLUP  },
    { _TTL_RTS, PIO_TYPE_MOVABLE, SWM_U1_RTS_O, PIO_MODE_DEFAULT },
  };

  bool init = false;
  uint8_t *ptxbuff, *prxbuff;

 public:

  static CDXIF *anchor;

  CDXIF();

  //! When you provide your own non-standard protocols and send/receive buffers.
  CDXIF (uint32_t baud, uint8_t mode, int txb, int rxb);

  ~CDXIF();

  //! Number of unsent data bytes
  uint16_t txbuff (void);
  //! Number of data bytes received
  uint16_t rxbuff (void);
  //! Clear transmit buffer.
  void clear_txbuff (void);
  //! Clear Received data.
  void clear_rxbuff (void);
  //! Send 1 byte of characters.
  void putc (char c);
  //! Send string.
  void puts (const char *s);
  //! Send specified number of bytes of data.
  int putsb (const uint8_t *s, int n);
  //! Retrieve one character from the receive data buffer.
  char getc (void);
  //! Receive string.
  uint16_t gets (char *s, int n);

#if 1
  //! Send format conversion strings
  template <typename ... Args>
  int printf (const char *format, Args const &... args) {
    if (init) return usart1_dma_printf (format, args ...);
    return 0;
  }
  template <typename ... Args>
  int scanf (const char *format, Args const &... args) {
    if (init) return usart1_dma_scanf (format, args ...);
    return 0;
  }
#endif
  //! Receive format conversion strings
  int (*pprintf()) (const char *, ...);

  void no_csw (void);
};

//=======================================================================
// GPIO
//=======================================================================
/*!
 @brief LPC845 GPIO class.
 @note
   The pins directly connected to the MCU can be used as GPIO/ADC/DAC.
   It also supports pulse width measurement and 2-phase encoder acquisition.
 */
class CGPIO {
  const uint8_t _GPIO9 = 4;
  const uint8_t _GPIO8 = 13;
  const uint8_t _GPIO7 = 17;
  const uint8_t _GPIO6 = 18;
  const uint8_t _GPIO5 = 19;
  const uint8_t _GPIO4 = 20;
  const uint8_t _GPIO3 = 21;
  const uint8_t _GPIO2 = 22;
  const uint8_t _GPIO1 = 23;
  const uint8_t _GPIO0 = 14;

  const TPin pins[10] = {
    { _GPIO9, PIO_TYPE_INPUT, 0, PIO_MODE_DEFAULT },
    { _GPIO8, PIO_TYPE_INPUT, 0, PIO_MODE_DEFAULT },
    { _GPIO7, PIO_TYPE_INPUT, 0, PIO_MODE_DEFAULT },
    { _GPIO6, PIO_TYPE_INPUT, 0, PIO_MODE_DEFAULT },
    { _GPIO5, PIO_TYPE_INPUT, 0, PIO_MODE_DEFAULT },
    { _GPIO4, PIO_TYPE_INPUT, 0, PIO_MODE_DEFAULT },
    { _GPIO3, PIO_TYPE_INPUT, 0, PIO_MODE_DEFAULT },
    { _GPIO2, PIO_TYPE_INPUT, 0, PIO_MODE_DEFAULT },
    { _GPIO1, PIO_TYPE_INPUT, 0, PIO_MODE_DEFAULT },
    { _GPIO0, PIO_TYPE_INPUT, 0, PIO_MODE_DEFAULT },
  };

  // Pin information
  typedef struct {
    uint8_t               No;
    CHIP_PINx_T           CPin;
    CHIP_SWM_PIN_FIXED_T  AD;
  } __attribute__ ((__packed__)) TPinInfo;

  const TPinInfo PinInfo[10] = {
    { 14, IOCON_PIO0_14, SWM_FIXED_ADC_2  },
    { 23, IOCON_PIO0_23, SWM_FIXED_ADC_3  },
    { 22, IOCON_PIO0_22, SWM_FIXED_ADC_4  },
    { 21, IOCON_PIO0_21, SWM_FIXED_ADC_5  },
    { 20, IOCON_PIO0_20, SWM_FIXED_ADC_6  },
    { 19, IOCON_PIO0_19, SWM_FIXED_ADC_7  },
    { 18, IOCON_PIO0_18, SWM_FIXED_ADC_8  },
    { 17, IOCON_PIO0_17, SWM_FIXED_ADC_9  },
    { 13, IOCON_PIO0_13, SWM_FIXED_ADC_10 },
    {  4, IOCON_PIO0_4,  SWM_FIXED_ADC_11 },
  };

  // Pulse Width Measurement Result
  uint32_t pwd[8], pwdup[8];
  uint32_t pulse_update_cnt;

  // GPIO No. for pulse width measurement
  uint8_t previous_mpw_gpiono[8];

  uint8_t enc_phase_pin[2][2]; // [ch][phase]
  int32_t enc_counter[2];
  int32_t enc_offset[2];
  bool enc_polrev[2];
  int8_t enc_ind[2];
  const int8_t enc_dirtable[16] = { 0, 1, -1, 0, -1, 0, 0, 1, 1, 0, 0, -1, 0, -1, 1, 0 };

  uint8_t pinint_interruptor[8];

  inline uint8_t enc_phase (uint8_t ch);

 public:

  static CGPIO *anchor;

  //! GPIO Pin Mode
  typedef enum {
    tPinDIN,      ///< Input (Hi-Z)
    tPinDIN_PU,   ///< Input (Pull Up)
    tPinDIN_PD,   ///< Input (Pull Down)
    tPinDOUT,     ///< Output (Push Pull)
    tPinDOUT_OD,  ///< Output (Open Drain)
    tPinAIN,      ///< Analog input
    //! PININT is used for pulse system signals. Therefore, the upper limit is 8 channels.
    //! @attention Also, All must specify different channels.
    tPinINT0_MPW, ///< PININT0 input (Pulse Width Measurement)
    tPinINT1_MPW, ///< PININT1 input (Pulse Width Measurement)
    tPinINT2_MPW, ///< PININT2 input (Pulse Width Measurement)
    tPinINT3_MPW, ///< PININT3 input (Pulse Width Measurement)
    tPinINT4_MPW, ///< PININT4 input (Pulse Width Measurement)
    tPinINT5_MPW, ///< PININT5 input (Pulse Width Measurement)
    tPinINT6_MPW, ///< PININT6 input (Pulse Width Measurement)
    tPinINT7_MPW, ///< PININT7 input (Pulse Width Measurement)
    tPinINT0_ENC0A, ///< PININT0 input (2 Phase Encoder 0 A)
    tPinINT1_ENC0A, ///< PININT1 input (2 Phase Encoder 0 A)
    tPinINT2_ENC0A, ///< PININT2 input (2 Phase Encoder 0 A)
    tPinINT3_ENC0A, ///< PININT3 input (2 Phase Encoder 0 A)
    tPinINT4_ENC0A, ///< PININT4 input (2 Phase Encoder 0 A)
    tPinINT5_ENC0A, ///< PININT5 input (2 Phase Encoder 0 A)
    tPinINT6_ENC0A, ///< PININT6 input (2 Phase Encoder 0 A)
    tPinINT7_ENC0A, ///< PININT7 input (2 Phase Encoder 0 A)
    tPinINT0_ENC0B, ///< PININT0 input (2 Phase Encoder 0 B)
    tPinINT1_ENC0B, ///< PININT1 input (2 Phase Encoder 0 B)
    tPinINT2_ENC0B, ///< PININT2 input (2 Phase Encoder 0 B)
    tPinINT3_ENC0B, ///< PININT3 input (2 Phase Encoder 0 B)
    tPinINT4_ENC0B, ///< PININT4 input (2 Phase Encoder 0 B)
    tPinINT5_ENC0B, ///< PININT5 input (2 Phase Encoder 0 B)
    tPinINT6_ENC0B, ///< PININT6 input (2 Phase Encoder 0 B)
    tPinINT7_ENC0B, ///< PININT7 input (2 Phase Encoder 0 B)
    tPinINT0_ENC1A, ///< PININT0 input (2 Phase Encoder 1 A)
    tPinINT1_ENC1A, ///< PININT1 input (2 Phase Encoder 1 A)
    tPinINT2_ENC1A, ///< PININT2 input (2 Phase Encoder 1 A)
    tPinINT3_ENC1A, ///< PININT3 input (2 Phase Encoder 1 A)
    tPinINT4_ENC1A, ///< PININT4 input (2 Phase Encoder 1 A)
    tPinINT5_ENC1A, ///< PININT5 input (2 Phase Encoder 1 A)
    tPinINT6_ENC1A, ///< PININT6 input (2 Phase Encoder 1 A)
    tPinINT7_ENC1A, ///< PININT7 input (2 Phase Encoder 1 A)
    tPinINT0_ENC1B, ///< PININT0 input (2 Phase Encoder 1 B)
    tPinINT1_ENC1B, ///< PININT1 input (2 Phase Encoder 1 B)
    tPinINT2_ENC1B, ///< PININT2 input (2 Phase Encoder 1 B)
    tPinINT3_ENC1B, ///< PININT3 input (2 Phase Encoder 1 B)
    tPinINT4_ENC1B, ///< PININT4 input (2 Phase Encoder 1 B)
    tPinINT5_ENC1B, ///< PININT5 input (2 Phase Encoder 1 B)
    tPinINT6_ENC1B, ///< PININT6 input (2 Phase Encoder 1 B)
    tPinINT7_ENC1B, ///< PININT7 input (2 Phase Encoder 1 B)

    tPinSPISEL, tPinSPISCK, tPinSPIMISO, tPinSPIMOSI,
    tPinSPFUNC
  } TPinMode;

  CGPIO();

  //! To initialize all terminals at once at instance
  CGPIO (const TPinMode cfg[10]);

  ~CGPIO();

  //! PININT interrupt callback
  void PIN_INT_cb (uint8_t ch);

  //! GPIO function setting
  void set_config (uint8_t adch, TPinMode pm);
  //! Get igital input (valid range 10bit)
  uint16_t get_gpio (void);

  //! Set digital output (valid range 10bit)
  void set_gpio (uint16_t out);

  //! Get ADCn result (ch:0...9)
  uint16_t get_adc (uint8_t ch);

  //! Get pulse width measurement (ch: 0...7)
  uint32_t get_pwd (uint8_t ch);

  uint32_t get_pulse_update_cnt (void);

  int32_t get_ENC_rawcounter (uint8_t ch);

  int32_t get_encoder_count (uint8_t ch);

  void reset_encoder_count (uint8_t ch);
};


//=======================================================================
// EXIO
//=======================================================================
/*!
 @brief SPI I/O expander control class.
 @note
   It covers simple input/output of external signals and LEDs on the board.
 */
class CEXIO {
  const uint8_t _SPI_MOSI = 16;
  const uint8_t _SPI_SCLK = 24;
  const uint8_t _SPI_MISO = 15;
  const uint8_t _SPI_CE   = 1;

  const TPin pins[4] = {
    { _SPI_MISO, PIO_TYPE_MOVABLE, SWM_SPI1_MISO_IO,  PIO_MODE_DEFAULT },
    { _SPI_MOSI, PIO_TYPE_MOVABLE, SWM_SPI1_MOSI_IO,  PIO_MODE_DEFAULT },
    { _SPI_SCLK, PIO_TYPE_MOVABLE, SWM_SPI1_SCK_IO,   PIO_MODE_DEFAULT },
    { _SPI_CE,   PIO_TYPE_MOVABLE, SWM_SPI1_SSEL0_IO, PIO_MODE_DEFAULT },
  };

  const uint8_t _PCAL9722_addr = (0x40 >> 1);
  uint8_t _txb[20], _rxb[20];
  uint32_t _txbuflast;

  uint8_t _LED_stat;

  const TSPIConf _spi1_conf = { LPC_SPI1, SPIMode0, SPIBitMSBF, 5000000, 8, SPICSLow };
  SemaphoreHandle_t _mutex;

  // Two DMA descriptors for SPI1 transmission are required here.
  // Therefore, forcing the use of channels provided for other peripherals.
  const int _DMAREQ_TX0 = DMA_CH14;
  const int _DMAREQ_TX1 = DMA_CH15;

  void init_SPI1 (void);

  // SPI1 send/receive with DMA
  // 8 bits wide regardless of spiconf.dataw
  //-----------------------------
  bool readwrite_SPI1 (const TSPIConf *SPIConf, const uint8_t *txd, uint8_t *rxd, uint16_t datalen);
  // PCAL9722 register write
  uint16_t PCAL_reg_write (uint8_t adr, const uint8_t *dat, uint16_t size);

  // PCAL9722 register read
  uint16_t PCAL_reg_read (uint8_t adr, uint8_t *dat, uint16_t size);

  // For LED GPIOs only
  bool PCAL_LED_Write (uint8_t mode, uint8_t b);

 public:

  static CEXIO *anchor;

  typedef enum {
    tPinDIN,      ///< Input (Hi-Z)
    tPinDIN_PU,   ///< Input (Pull Up)
    tPinDIN_PD,   ///< Input (Pull Down)
    tPinDOUT      ///< Output (Push Pull)
  } TPinMode;

  CEXIO();

  //! To initialize all terminals at once at instance
  CEXIO (const TPinMode cfg[16]);

  ~CEXIO();
  void no_csw (void);

  //! Flicker for all LEDs.
  void set_LED (uint8_t b);

  //! Turn on the LED of the masked bit.
  void set_LED_off (uint8_t b);

  //! Turn off LEDs of masked bits.
  void set_LED_on (uint8_t b);

  //! Toggle on/off LEDs of masked bits.
  void set_LED_toggle (uint8_t b);

  //! GPIO function setting.
  bool set_config (uint8_t ch, TPinMode pm);
  //! Digital input.
  uint16_t get_gpio (void);

  //! Digital output.
  bool set_gpio (uint16_t out);
};

//=======================================================================
// Motor Driver
//=======================================================================
/*!
 @brief PWM control for 2c DCM class.
 @note
  DC motor connected to a 2-channel H-bridge is operated with slow decay.
 */
class CMotor {
  uint8_t _swap_dir;
  uint16_t _max_abs_duty[2];
  int16_t _duty[2];

  const uint8_t _MEN   = 9;
  const uint8_t _M0DIR = 7;
  const uint8_t _M0PWM = 0;
  const uint8_t _M1DIR = 8;
  const uint8_t _M1PWM = 6;

  const TPin pins[5] = {
    { _MEN,   PIO_TYPE_OUTPUT_0, 0,              PIO_MODE_DEFAULT },
    { _M0DIR, PIO_TYPE_OUTPUT_1, 0,              PIO_MODE_DEFAULT },
    { _M0PWM, PIO_TYPE_MOVABLE,  SWM_SCTOUT_0_O, PIO_MODE_DEFAULT },
    { _M1DIR, PIO_TYPE_OUTPUT_1, 0,              PIO_MODE_DEFAULT },
    { _M1PWM, PIO_TYPE_MOVABLE,  SWM_SCTOUT_1_O, PIO_MODE_DEFAULT },
  };

  inline bool get_dirpin (int ch);

  inline void set_dirpin (int ch, bool on);

  void set_pwm_frequency (uint16_t freq);

  void set_pwm_duty (uint8_t ch, int16_t duty);

 public:

  // freqHz  :0...50000 Hz
  // dir     :0=default, 1=Invert m0  2=Invert m1, 3=Invert m0 & m1 (2bit:Swap m0 and m1)
  // maxduty :0...1000 ‰
  CMotor (int freqHz, uint8_t swap_dir, const uint16_t maxduty[2]);

  CMotor();

  //! Set default sign and channel swap.
  // dir :0=default, 1=Invert m0  2=Invert m1, 3=Invert m0 & m1 (2bit:Swap m0 and m1)
  void set_swap_dir (uint8_t dir);
  //! Get current sign and channel swap.
  uint8_t get_swap_dir (void);

  //! Set PWM carrier frequency.
  // freqHz :0...50000 Hz
  void set_freq (uint32_t freqHz);

  //! Set maximum PWM duty.
  // maxduty:0...1000 ‰
  void set_max_duty (uint16_t m0, uint16_t m1);

  //! Get current gate state
  bool get_gate (void);
  //! Turn gate on/off
  void set_gate (bool on);

  //! Set PWM duty to each channel.
  // m0, m1: -1000...1000 ‰
  void set_duty (uint8_t ch, int16_t duty_permil);

  //! Set PWM duty to biaxial.
  // m0, m1: -1000...1000 ‰
  void set_biaxial_dual (int16_t m0, int16_t m1);
  //! Set PWM duty to biaxial.
  // m0, m1: -1000...1000 ‰
  void set_biaxial_duty (int16_t d[2]);

  //! Get current PWM duty for each channel.
  // m0, m1: -1000...1000 ‰
  int16_t get_duty (uint8_t ch);
  // Get current PWM duty for biaxial.
  void get_biaxial_duty (int16_t d[2]);
};

//=======================================================================
// I2C
//=======================================================================
/*!
 @brief Arduino-like I2C class.
 @note
   It was created to flow the numerous resources for Arduino.
 */
class CI2C {
  const uint8_t _SCL = 10;
  const uint8_t _SDA = 11;

  const TPin pins[2] = {
    { _SCL, PIO_TYPE_FIXED, SWM_FIXED_I2C0_SCL, PIO_MODE_DEFAULT },
    { _SDA, PIO_TYPE_FIXED, SWM_FIXED_I2C0_SDA, PIO_MODE_DEFAULT },
  };

  const uint32_t FLAG_TIMEOUT = 100;
  const uint32_t LONG_TIMEOUT = 1000;

  SemaphoreHandle_t _mutex;

  void (*csw) (void);

  inline uint8_t i2c_get_stat (void);

  bool i2c_wait_SI (LPC_I2C_T *I2Cx);

  bool i2c_wait_stat (LPC_I2C_T *I2Cx);

  bool i2c_read (LPC_I2C_T *I2Cx, void *dat, bool fin);


  int i2c_write (LPC_I2C_T *I2Cx, int value, uint8_t addr);

  inline void i2c_genestart (LPC_I2C_T *I2Cx);

 public:

  typedef enum {
    IIC_Sm,   //!< Standard-mode 100k
    IIC_Fm,   //!< Fast-mode 400k
    IIC_Fmp,  //!< Fast-mode Plus 1M
  } TIICMode;

  CI2C (TIICMode m);

  ~CI2C();

  //! Take semaphore.
  void lock_sem (void);
  //! Give semaphore.
  void unlock_sem (void);

  //! Set clock frequency
  bool set_freq (TIICMode m);

  //! Join the I2C bus.
  bool begin (uint8_t addr, bool read = false);

  //! Release the I2C bus.
  bool end (void);

  //! Transmission of 1 byte.
  int write (uint8_t val);
  int write (const uint8_t *val, int l);

  //! Receive specified byte length
  bool read (void *prxd, int length);

  //! Check if the device are actually connected.
  bool ping (uint8_t addr);
};

//=======================================================================
// PCM Audio player
//=======================================================================
/*!
 @brief PCM Playback class.
 @note
   PCM data is played back via the LPC845 DAC.
 */
struct CPCM {
  static CPCM *anchor;

  //! PCM data management information.
  typedef struct {
    int8_t  *raw; //!< PCM data pointer
    int32_t size; //!< PCM data byte size
    int32_t ind;  //!< -1:stop, 0:start
    int16_t freq; //!< 256:no conversion
    uint8_t vol;  //!< 127:max volume
  } TPCMInfo, *PPCMInfo;

 private:

  const uint8_t _GPIO7 = 17;

  std::vector<TPCMInfo> *ppcminfo;

  const int _UPDATE_FREQ = 11025;
  uint8_t volume;

  int32_t (*pcuston_callback) (void);

  int (*dbg) (const char *, ...);

 public:

  // pinfo  :TPCMInfo
  // vol    :0..127 default volume
  CPCM (std::vector<TPCMInfo> *pinfo, uint8_t vol);

  ~CPCM();

  //! Starts playback process in the background.
  void begin (void);

  //! Ends the playback process in the background.
  void end (void);

  //! Excitation of PCM reproduction
  // playback is performed by MRT and DAC
  void play (uint8_t ind, uint8_t tone, uint8_t vol);

  //! Stops the specified PCM during playback.
  void stop (uint8_t ind);

  //! Set master volume.
  uint8_t set_volume (int v);

  //! Get current master volume.
  uint8_t get_volume (void);

  //! Set your own function to be called on interrupt
  void set_custom_cb(int32_t (*cb)(void));

  //! MRT interrupt callback
  void mrt_cb (void);
};

//=======================================================================
// Radio Controlled receiver
//=======================================================================
/*!
 @brief Pulse processing class for radio-controlled radio receivers
 @note
   Use CGPIO class pulse width measurement.
 */
class CRCStick {
  // NVM address
  const uint32_t _EEPROM_ADDR = 0xfc00;

  CGPIO *cgpio;

 public:

  // Pulse data for 1ch
  typedef struct {
    int32_t min;      // minimum
    int32_t neutral;  // neutrality
    int32_t max;      // maximum
  } TTcaldata;
  typedef TTcaldata Tcaldata[8];

 private:

  Tcaldata caldata;

  // Interpolation
  int32_t _INTERP (int32_t xi, int32_t xj, int32_t yi, int32_t yj, int32_t x);

  int interp1dim (const int32_t x, const int32_t *ar, const int32_t w);

  // moving mean
  void movingmean (int32_t *ave, int32_t dat, int32_t num);

#if 0
  template <typename ... Args>
  int _printf (const char *format, Args const &... args);
#endif

  void _led (uint8_t n);

  void _play (uint8_t t, uint32_t w = 0);

 public:

  CRCStick (CGPIO *g);

  //! Calibration
  //  f_nextstep:Trigger to advance the operation under calibration to the next step
  void calibration (bool (*f_nextstep) (void), void (*f_mon) (const char *s, uint32_t *) = NULL);

  //! Get calibration value.
  void get_calibration (Tcaldata *pcal);

  //! Normalize pulses to -1000...1000 (per_new: neutral, per_ul: % of deadband to measured full scale at both ends)
  int32_t get_normal (uint8_t ch, int32_t per_neu, int32_t per_ul);
};

//=======================================================================
// PID
//=======================================================================
/*!
 @brief PID control class.
 @note
   General PID control only.
 */
struct CPID {
  //! PID control information
  //! @attention Do not declare const.
  typedef struct {
    float Kp;       //!< proportional gain
    float Ki;       //!< integral  gain
    float Kd;       //!< derivative gain
    float Kf;       //!< feedforward gain
    float delta_t;  //!< discrete time[sec]
    float min;      //!< saturation minimum
    float max;      //!< saturation maximum
    float err[2];   //!< deviation internal buffer
    float integral; //!< integral value
    float prevcalc; //!< previous operation result
  } TPIDParam, *PPIDParam;

  //! Reset PID controller.
  void reset (PPIDParam p);

  //! PID operation.
  float calc (PPIDParam p, float feedback, float target);
};

//=======================================================================
// Speed Control
//=======================================================================
/*!
 @brief PID speed control class.
 @note
   General PID control only.
   The default control cycle is 5 ms. In addition, the speed is calculated
   using 80 ms of history to improve accuracy when encoder pulses are low.
 @attention
   FreeRTOS scheduler must be running to use this class.
 */
class CSpeedControl {
#define _SPD_DIV 16   //!< 2 or more and multiple of 2, Speed calculation with time difference of (_SPD_DIV - 1) * ControlPeriod [ms].
#define _CTRL_DIV 1   //!< Control in _CTRL_DIV * 5[ms] cycles

  uint8_t spddetect_div, spdctrl_div;
  CGPIO *pEnc;
  CMotor *pMot;

  struct {
    int32_t prev_enc;
    int32_t current_counter;
    int32_t counter_history[_SPD_DIV];
    int32_t counter[3];
    int32_t current_speed;
    int32_t target_speed;   // count/(_SPD_DIV * 5)ms
    float ramp;
    float ramped_target_speed;
    CPID pid;
    CPID::PPIDParam gain;
  } spparam[2];

  xTaskHandle interval_timer_task_handle;
  bool kill_interval_timer_task;

  bool control_on = false;

  float calcramp (float *prev, int32_t target, int32_t dt, float ramp);

  // 5ms cycle interval
  void interval_timer_task();

 public:

  CSpeedControl (CMotor *m, CGPIO *e);

  //! Reset all control operation results.
  void reset_gain (void);

  //! Start speed control task.
  void begin (void);
  //! Stop speed control task.
  void end (void);

  //! Start speed control.
  void start (void);
  //! Stop speed control.
  void stop (void);

  //! Set control gain for each axis.
  void set_gain (uint8_t ch, CPID::PPIDParam g);
  //! Set the control biaxial gain.
  void set_biaxial_gain (CPID::PPIDParam g0, CPID::PPIDParam g1);

  //! Set target speed for each axis.
  void set_taget_speed (uint8_t ch, int32_t s);
  //! Set biaxial target speed.
  void set_biaxial_taget_speed  (int32_t d0, int32_t d1);
  //! Get target speed of each axis.
  int32_t get_target_speed (uint8_t ch);

  //! Get current speed of each axis.
  int32_t get_current_speed (uint8_t ch);
  //! Set ramp for each axis.
  void set_ramp (uint8_t ch, float r);
  //! Set biaxial ramp.
  void set_biaxial_ramp (float r0, float r1);

  //! Reseet current pulse count for each axis
  void reset_counter (uint8_t ch, int n);
  //! Get current pulse count for each axis
  int32_t get_counter (uint8_t ch, int n);
};



//=======================================================================
// Synthesizer & MML player
//=======================================================================
#define _MAX_MUSICSQ          (6)
#define _DEFAULT_MUSICICSIZE  (700)

/*!
 @brief Simple MML & Player class
 @note
   Functions are minimized to save memory resources.
   So the scalability is poor.
   Playback task is created at the same time as the object is created.
   FreeRTOS scheduler must be running to use this class.
 @details
  <ul><li>[MML]<ul>
  <li>Scale             : Cx,Dx,Ex,Fx,Gx,Ax,Bx (x:+/-/#/1..64 note lengths, Dots correspond only to 2,4,8,16)
  <li>Rest              : Rx (x:[1;64] note lengths, Dots correspond only to 2,4,8,16)
  <li>Length            : Lx (x:[1;64] note lengths default=4)
  <li>Slur              : &
  <li>Tempo             : Tx (x:[0;9] default=5)
  <li>Octave            : Ox (x:[0;9] default=4)
  <li>Octave UP         : <
  <li>Octave DOWN       : >
  <li>Repetition Head   : [x (x:Number of repetitions 2..9, Infinite if not specified, Cannot branch, Nest up to 16 levels)
  <li>Repetition Tail   : ]
  <li>Wavetable/PCM No. : \@x (x:[0;9] default=0)
  <li>Detune            : Kx (x:[1;15] default=8)
  <li>Velocity/Volume   : Vx (x:[0;127] default=63)
  <li>Envelope          : $a,d,s,r (each value:[0;127] default=0,32,32,0)
  </ul></ul>
  <ul><li>[Conversion Data]<ul>
  <li>HI 4bit
    0     :fin (low=0), slur (low=1), repetition head (low=2..10), repetition tail (low=11), velocity (low=12), wave change (low=13), detune (low=14), envelope (low=15)
    1     :rest (low=length)
    2～13 :tone(c,c+,d,d+,e,f,f+,g,g+,a,a+,b) (low=length)
    14    :tempo
    15    :octave
  <li>LOW 4bit
    0～15 :length value
    0～9  :tempo value
    0～9  :octave value
  </ul></ul>
 */
class CTinyMusicSq {
  enum TStat { stPlaying, stStopExec, stStopping } stat;
  static int ChCount; // Channel number increment
  int ChannelNo;      // Own channel number
  int size;           // Data size after conversion
  int mi_ind;         // Playing index of playing data
  TaskHandle_t TaskHandle;

  bool enable_pcm;

  // Converted data
  union TIntermediateCode {
    uint8_t byte;
    struct {
      uint8_t L :4;
      uint8_t H :4;
    } bit;
  } *IC;

  // Store conversion data
  bool store_MI (int cmd, int param);
  bool store_MI_b (uint8_t d);

  // Convert note length
  int len2ind (int ll);

  // scale analysis
  bool analyze_scale (const char **MML, int oct, int tnum, int len);

  void noteOff (uint8_t ch, uint8_t n);

  void noteOn (uint8_t ch, uint8_t n, uint8_t v, bool slur);

  void noteReset (uint8_t ch);

  // play task
  void playtask (void);

public:
  //! convert MML to intermediate code
  bool Convert (const char *MML);

  //! play music
  void StartMusic (void);

  //! stop music
  void StopMusic (void);

  //! Set this channel to PCM playback only
  void enable_PCM (void);
  //! Set this channel to Wavetable only
  void disable_PCM (void);

  //! Tied to PCM (required)
  void begin (CPCM *pcm);
  void end (void);

  CTinyMusicSq (int sz = _DEFAULT_MUSICICSIZE);
  ~CTinyMusicSq ();
};

//! Set ADSR envelope
void SetEnvelope (uint8_t ch, const int8_t *v);
//! Set wave form
void SetWaveForm (uint8_t no, const int8_t *v, uint8_t size);

//=======================================================================
// Stack over flow hook
//=======================================================================
void vApplicationStackOverflowHook (TaskHandle_t pxTask, char *pcTaskName) {
  taskDISABLE_INTERRUPTS();
  _wait.no_csw();
  CDXIF *pdx = CDXIF::anchor;
  if (pdx == NULL) pdx = new CDXIF;
  CEXIO *pex = CEXIO::anchor;
  if (pex == NULL) pex = new CEXIO;
  pdx->no_csw();
  pdx->puts ("\r\n!! Stack Overflow !!\r\n");
  pdx->puts ((const char *)pcTaskName);
  pdx->puts ("\r\n");
  while (1) {
    for (int i = 0; i < 1000; i++) {
      pex->set_LED (0xf);
      _wait.us (100);
      pex->set_LED (0);
      _wait.us (i);
      pex->set_LED_toggle (0xf);
    }
    _wait.ms (50);
    Chip_WWDT_Feed (LPC_WWDT);
  }
}
