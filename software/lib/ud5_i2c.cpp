/*!
  @file    ud5_i2c.cpp
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
// I2C
//=======================================================================
/*!
 @brief Arduino-like I2C class.
 @note
   It was created to flow the numerous resources for Arduino.
 */
inline uint8_t CI2C::i2c_get_stat (void) {
  return (LPC_I2C0->STAT >> 1) & 0x07;
}

bool CI2C::i2c_wait_SI (LPC_I2C_T *I2Cx) {
  uint32_t  elap = UD5_GET_ELAPSEDTIME() + FLAG_TIMEOUT;
  while (! (I2Cx->STAT & I2C_MSTCTL_MSTCONTINUE)) {
    if (UD5_GET_ELAPSEDTIME() > elap) return false;
    if (csw != NULL) csw();
  }
  return true;
}

bool CI2C::i2c_wait_stat (LPC_I2C_T *I2Cx) {
  uint32_t  elap = UD5_GET_ELAPSEDTIME() + FLAG_TIMEOUT;
  while (elap > UD5_GET_ELAPSEDTIME()) {
    if (i2c_get_stat() != 0x03) return true;
    if (csw != NULL) csw();
  }
  return true;
}

bool CI2C::i2c_read (LPC_I2C_T *I2Cx, void *dat, bool fin) {
  uint8_t   t;

  if (!i2c_wait_SI (I2Cx)) return false;
  uint8_t stat = i2c_get_stat();

  if (stat != 0x01) { // RX RDY
    return false;
  }
  t = I2Cx->MSTDAT & 0xFF; // Store read byte
  if (!fin) I2Cx->MSTCTL = (1 << 0);// Send ACK and Continue to read
  * (uint8_t *)dat = t;
  return true;
}


int CI2C::i2c_write (LPC_I2C_T *I2Cx, int value, uint8_t addr) {
  // write the data
  I2Cx->MSTDAT = value;
  // Set continue for data. Should not be set for addr since that uses STA
  I2Cx->MSTCTL = I2C_MSTCTL_MSTCONTINUE;
  // wait and return status
  i2c_wait_SI (I2Cx);
  return i2c_get_stat();
}

inline void CI2C::i2c_genestart (LPC_I2C_T *I2Cx) {
  I2Cx->MSTCTL = I2C_MSTCTL_MSTSTART; // STA bit
}

CI2C::CI2C (TIICMode m) {
  PIO_Configure (pins, PIO_LISTSIZE (pins));
  Chip_IOCON_PinSetI2CMode (IOCON_PIO0_9, PIN_I2CMODE_FASTPLUS);
  Chip_IOCON_PinSetI2CMode (IOCON_PIO0_10, PIN_I2CMODE_FASTPLUS);

  LPC_SYSCON->I2C0CLKSEL = SYSCON_FLEXCOMMCLKSELSRC_MAINCLK;
  Chip_I2C_Init (LPC_I2C0);       // Enable I2C clock and reset I2C peripheral
  LPC_I2C0->CFG = (LPC_I2C0->CFG & I2C_CFG_MASK) | I2C_CFG_MSTEN; // Enable Master mode
  set_freq (m);
  csw = _my_csw;
  _mutex = xSemaphoreCreateMutex();
}

CI2C::~CI2C() {
  Chip_SYSCON_PeriphReset (RESET_I2C0);
  vSemaphoreDelete (_mutex);
}

//! Take semaphore.
void CI2C::lock_sem (void) {
  if ((xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) xSemaphoreTake (_mutex, portMAX_DELAY);
}
//! Give semaphore.
void CI2C::unlock_sem (void) {
  if ((xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) xSemaphoreGive (_mutex);
}

//! Set clock frequency
bool CI2C::set_freq (TIICMode m) {
  switch (m) {
    case IIC_Sm:  // Standard mode
      // Speed = 100kbps
      Chip_I2C_SetClockDiv (LPC_I2C0, SystemCoreClock / (100000 * 4)); // Setup clock rate for I2C
      Chip_I2CM_SetBusSpeed (LPC_I2C0, 100000);
      break;
    case IIC_Fm: // Fast Mode high
      // Speed = 400kbps
      Chip_I2C_SetClockDiv (LPC_I2C0, SystemCoreClock / (400000 * 4)); // Setup clock rate for I2C
      Chip_I2CM_SetBusSpeed (LPC_I2C0, 400000);
      break;
    case IIC_Fmp: // Fast Mode Plus
      // Speed = 1Mbps
      Chip_I2C_SetClockDiv (LPC_I2C0, SystemCoreClock / (1000000 * 4)); // Setup clock rate for I2C
      Chip_I2CM_SetBusSpeed (LPC_I2C0, 1000000);
      Chip_IOCON_PinSetI2CMode (/*LPC_IOCON, */IOCON_PIO0_10, PIN_I2CMODE_FASTPLUS);
      Chip_IOCON_PinSetI2CMode (/*LPC_IOCON, */IOCON_PIO0_11, PIN_I2CMODE_FASTPLUS);
      break;
  }
  Chip_I2CM_Enable (LPC_I2C0);              // Enable Master Mode
  return true;
}

//! Join the I2C bus.
bool CI2C::begin (uint8_t addr, bool read) {
  LPC_I2C0->MSTDAT = (addr << 1) | (read ? 1 : 0);

  i2c_genestart (LPC_I2C0);

  i2c_wait_SI (LPC_I2C0);
  return (i2c_get_stat() != 0x03); // NAK SlaveAddress
}

//! Release the I2C bus.
bool CI2C::end (void) {
  uint32_t timeout = UD5_GET_ELAPSEDTIME() + LONG_TIMEOUT;

  // STP bit and Continue bit. Sends NAK to complete previous RD
  LPC_I2C0->MSTCTL = I2C_MSTCTL_MSTSTOP | I2C_MSTCTL_MSTCONTINUE;

  //Spin until Ready (b0 == 1)and Status is Idle (b3..b1 == 000)
  while ((LPC_I2C0->STAT & (I2C_STAT_MSTSTATE | I2C_STAT_MSTPENDING)) != ((0 << 1) | I2C_STAT_MSTPENDING)) {
    if (UD5_GET_ELAPSEDTIME() > timeout) return false;
    if (csw != NULL) csw();
  }
  return true;
}

//! Transmission of 1 byte.
int CI2C::write (uint8_t val) {
  if (0x02 == i2c_write (LPC_I2C0, val, 0)) return 1;
  return 0;
}
int CI2C::write (const uint8_t *val, int l) {
  int i;
  for (i = 0; i < l; i++) {
    if (0x02 != i2c_write (LPC_I2C0, *val++, 0)) break;
  }
  return i;
}

//! Receive specified byte length
bool CI2C::read (void *prxd, int length) {
  bool result = false;
  if (length > 0) {
    if (i2c_wait_stat (LPC_I2C0)) {
      while (length--) {
        if ((result = i2c_read (LPC_I2C0, (void *)prxd, length == 0))) {
          prxd = (void *) ((uint8_t *)prxd + 1);
        } else break;
      }
    }
  }
  return result;
}

//! Check if the device are actually connected.
bool CI2C::ping (uint8_t addr) {
  bool b = false;
  if (begin (addr)) b = end();
  return b;
}
