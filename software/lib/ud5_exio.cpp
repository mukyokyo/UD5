/*!
  @file    ud5_exio.cpp
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
// EXIO
//=======================================================================
/*!
 @brief SPI I/O expander control class.
 @note
   It covers simple input/output of external signals and LEDs on the board.
 */

void CEXIO::init_SPI1 (void) {
  Chip_Clock_EnablePeriphClock (SYSCON_CLOCK_SPI1);
  LPC_SYSCON->SPI1CLKSEL = SYSCON_FLEXCOMMCLKSELSRC_MAINCLK;
  SPI_Init (&_spi1_conf);
  SPI_csw = _my_csw;

  // DMA reinitialization suppression
  if (! (LPC_DMA->CTRL & DMA_CTRL_ENABLE)) {
    Chip_DMA_DeInit (LPC_DMA);
    Chip_DMA_Init (LPC_DMA);
    Chip_DMA_Enable (LPC_DMA);
  }

  // DMA transmit send configuration (use 2 descripotr just to have SSEL sent out automatically)
  Chip_DMA_DisableChannel (LPC_DMA, DMAREQ_SPI1_TX);
  Chip_DMA_EnableChannel (LPC_DMA, DMAREQ_SPI1_TX);
  Chip_DMA_DisableIntChannel (LPC_DMA, DMAREQ_SPI1_TX);
  Chip_DMA_SetupChannelConfig (LPC_DMA, DMAREQ_SPI1_TX, (DMA_CFG_PERIPHREQEN | DMA_CFG_TRIGBURST_SNGL | DMA_CFG_CHPRIORITY (3)));
  // Data except the last byte is sent using TXDAT
  Chip_DMA_Table[_DMAREQ_TX0].dest = DMA_ADDR (&_spi1_conf.SPIx->TXDAT);
  Chip_DMA_Table[_DMAREQ_TX0].next = DMA_ADDR (&Chip_DMA_Table[_DMAREQ_TX1]); // 次のdescriptorへのリンク
  // Last byte sent with flags using TXDATCTL
  Chip_DMA_Table[_DMAREQ_TX1].source = DMA_ADDR (&_txbuflast);
  Chip_DMA_Table[_DMAREQ_TX1].dest = DMA_ADDR (&_spi1_conf.SPIx->TXDATCTL);
  Chip_DMA_Table[_DMAREQ_TX1].next = DMA_ADDR (0);

  // DMA receive configuration
  Chip_DMA_DisableChannel (LPC_DMA, DMAREQ_SPI1_RX);
  Chip_DMA_EnableChannel (LPC_DMA, DMAREQ_SPI1_RX);
  Chip_DMA_DisableIntChannel (LPC_DMA, DMAREQ_SPI1_RX);
  Chip_DMA_SetupChannelConfig (LPC_DMA, DMAREQ_SPI1_RX, (DMA_CFG_PERIPHREQEN | DMA_CFG_TRIGBURST_SNGL | DMA_CFG_CHPRIORITY (3)));
  Chip_DMA_Table[DMAREQ_SPI1_RX].next = DMA_ADDR (0);
}

// SPI1 send/receive with DMA
// 8 bits wide regardless of spiconf.dataw
//-----------------------------
bool CEXIO::readwrite_SPI1 (const TSPIConf *SPIConf, const uint8_t *txd, uint8_t *rxd, uint16_t datalen) {
  if (datalen > sizeof (_txb)) return false;

  // Copy transmit data to buffer
  __aeabi_memcpy4 (_txb, txd, datalen);

  // SPI initial condition setting
  Chip_SPI_ClearStatus (SPIConf->SPIx, SPI_STAT_CLR_RXOV | SPI_STAT_CLR_TXUR | SPI_STAT_CLR_SSA | SPI_STAT_CLR_SSD);
  SPIConf->SPIx->TXCTRL =
    SPI_TXCTL_ASSERT_SSEL |   // Assert SSEL
    SPI_TXCTL_FLEN (8 - 1);   // Bit width of transmitted data -1

  // Receive settings
  Chip_DMA_Table[DMAREQ_SPI1_RX].xfercfg =
    DMA_XFERCFG_CFGVALID |
    DMA_XFERCFG_WIDTH_8 |
    DMA_XFERCFG_SRCINC_0 |
    DMA_XFERCFG_DSTINC_1 |
    DMA_XFERCFG_RELOAD |
    DMA_XFERCFG_XFERCOUNT (datalen);
  Chip_DMA_Table[DMAREQ_SPI1_RX].source = DMA_ADDR (&SPIConf->SPIx->RXDAT);
  Chip_DMA_Table[DMAREQ_SPI1_RX].dest = DMA_ADDR (_rxb) + datalen - 1;

  Chip_DMA_SetupTranChannel (LPC_DMA, DMAREQ_SPI1_RX, (DMA_CHDESC_T *)&Chip_DMA_Table[DMAREQ_SPI1_RX]);
  Chip_DMA_SetupChannelTransfer (LPC_DMA, DMAREQ_SPI1_RX, Chip_DMA_Table[DMAREQ_SPI1_RX].xfercfg);
  Chip_DMA_SetValidChannel (LPC_DMA, DMAREQ_SPI1_RX);
  Chip_DMA_SWTriggerChannel (LPC_DMA, DMAREQ_SPI1_RX);

  // Transmission settings excluding the last byte
  Chip_DMA_Table[_DMAREQ_TX0].xfercfg =
    DMA_XFERCFG_CFGVALID |
    DMA_XFERCFG_SWTRIG |
    DMA_XFERCFG_WIDTH_8 |
    DMA_XFERCFG_SRCINC_1 |
    DMA_XFERCFG_DSTINC_0 |
    DMA_XFERCFG_RELOAD |
    DMA_XFERCFG_XFERCOUNT (datalen - 1);
  Chip_DMA_Table[_DMAREQ_TX0].source = DMA_ADDR (_txb) + datalen - 1 - 1;

  // Transmission setting for last byte only
  _txbuflast =
    SPI_TXDATCTL_EOF |                      // End of frame
    SPI_TXDATCTL_EOT |                      // End of transmit, SSEL deasserts at end of transfer
    SPI_TXDATCTL_LEN (8 - 1) |              // Bit width of transmitted data -1
    SPI_TXDATCTL_DATA (_txb[datalen - 1]);  // Transmission data
  Chip_DMA_Table[_DMAREQ_TX1].xfercfg =
    DMA_XFERCFG_CFGVALID |
    DMA_XFERCFG_SWTRIG |
    DMA_XFERCFG_WIDTH_32 |
    DMA_XFERCFG_SRCINC_0 |
    DMA_XFERCFG_DSTINC_0 |
    DMA_XFERCFG_XFERCOUNT (1);

  Chip_DMA_SetupTranChannel (LPC_DMA, DMAREQ_SPI1_TX, (DMA_CHDESC_T *)&Chip_DMA_Table[_DMAREQ_TX0]);
  Chip_DMA_SetValidChannel (LPC_DMA, DMAREQ_SPI1_TX);

  // Excitation of transmission
  Chip_DMA_SetupChannelTransfer (LPC_DMA, DMAREQ_SPI1_TX, Chip_DMA_Table[_DMAREQ_TX0].xfercfg);

  // Waiting for transmission completion
  while ((Chip_DMA_GetActiveChannels (LPC_DMA) & (1 << DMAREQ_SPI1_TX)) != 0) ;
  while (! (SPIConf->SPIx->STAT & SPI_STAT_LE));

  // Copying received data
  __aeabi_memcpy4 (rxd, _rxb, datalen);

  return true;
}

// PCAL9722 register write
uint16_t CEXIO::PCAL_reg_write (uint8_t adr, const uint8_t *dat, uint16_t size) {
  uint8_t w[size + 2];
  uint8_t r[size + 2];
  w[0] = (_PCAL9722_addr << 1);
  w[1] = adr;
  __aeabi_memcpy4 (w + 2, dat, size);
  readwrite_SPI1 (&_spi1_conf, w, r, size + 2);

  return size;
}

// PCAL9722 register read
uint16_t CEXIO::PCAL_reg_read (uint8_t adr, uint8_t *dat, uint16_t size) {
  uint8_t w[size + 2];
  uint8_t r[size + 2];
  w[0] = (_PCAL9722_addr << 1) | 0x1;
  w[1] = adr;
  readwrite_SPI1 (&_spi1_conf, w, r, size + 2);
  __aeabi_memcpy4 (dat, r + 2, size);

  return size;
}

// For LED GPIOs only
bool CEXIO::PCAL_LED_Write (uint8_t mode, uint8_t b) {
  if ((xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) xSemaphoreTake (_mutex, portMAX_DELAY);
  switch (mode) {
    case 0: // off
      _LED_stat &= ~b;
      break;
    case 1: // on
      _LED_stat |= b;
      break;
    case 2: // toggle
      _LED_stat ^= b;
      break;
    case 4: // direct
      _LED_stat = b;
      break;
  }
  _LED_stat &= 0x3f;
  bool result = PCAL_reg_write (0x06, (const uint8_t *)&_LED_stat, 1) == 1;
  if ((xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) xSemaphoreGive (_mutex);
  return result;
}

CEXIO::CEXIO() {
  PIO_Configure (pins, PIO_LISTSIZE (pins));

  _LED_stat = 0;
  _mutex = xSemaphoreCreateMutex();
  init_SPI1();
  PCAL_reg_write (0x06, (const uint8_t[1]) { 0 }, 1);
  PCAL_reg_write (0x0c, (const uint8_t[3]) { 0xff, 0xff, 0xc0 }, 3);
  anchor = this;
}

//! To initialize all terminals at once at instance
CEXIO::CEXIO (const TPinMode cfg[16]) {
  PIO_Configure (pins, PIO_LISTSIZE (pins));

  _LED_stat = 0;
  _mutex = xSemaphoreCreateMutex();
  init_SPI1();
  PCAL_reg_write (0x06, (const uint8_t[1]) { 0 }, 1);
  PCAL_reg_write (0x0c, (const uint8_t[3]) { 0xff, 0xff, 0xc0 }, 3);
  for (int i = 0; i < 16; i++) set_config (i, cfg[i]);
  anchor = this;
}

CEXIO::~CEXIO() {
  PCAL_reg_write (0x4c, (const uint8_t[3]) { 0, 0, 0 }, 2);
  PCAL_reg_write (0x50, (const uint8_t[3]) { 0xff, 0xff, 0xff }, 3);
  PCAL_reg_write (0x0c, (const uint8_t[3]) { 0xff, 0xff, 0xff }, 3);
  PCAL_reg_write (0x04, (const uint8_t[3]) { 0xff, 0xff, 0xff }, 3);

  Chip_DMA_DisableChannel (LPC_DMA, DMAREQ_SPI1_TX);
  Chip_DMA_DisableChannel (LPC_DMA, DMAREQ_SPI1_RX);
  Chip_SPI_DeInit (_spi1_conf.SPIx);
  Chip_Clock_DisablePeriphClock (SYSCON_CLOCK_SPI1);

  Chip_SPI_DeInit (_spi1_conf.SPIx);
  Chip_Clock_DisablePeriphClock (SYSCON_CLOCK_SPI1);
  vSemaphoreDelete (_mutex);
  anchor = NULL;
}

void CEXIO::no_csw (void) {
  SPI_csw = NULL;
}

//! Flicker for all LEDs.
void CEXIO::set_LED (uint8_t b) {
  PCAL_LED_Write (4, b);
}

//! Turn on the LED of the masked bit.
void CEXIO::set_LED_off (uint8_t b) {
  PCAL_LED_Write (0, b);
}

//! Turn off LEDs of masked bits.
void CEXIO::set_LED_on (uint8_t b) {
  PCAL_LED_Write (1, b);
}

//! Toggle on/off LEDs of masked bits.
void CEXIO::set_LED_toggle (uint8_t b) {
  PCAL_LED_Write (2, b);
}

//! GPIO function setting.
bool CEXIO::set_config (uint8_t ch, TPinMode pm) {
  if ((xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) xSemaphoreTake (_mutex, portMAX_DELAY);
  bool result = false;
  if (ch <= 15) {
    uint16_t r_c, r_pe, r_pud, msk = 1 << ch;
    PCAL_reg_read (0x0c, (uint8_t *)&r_c, 2);
    PCAL_reg_read (0x4c, (uint8_t *)&r_pe, 2);
    PCAL_reg_read (0x50, (uint8_t *)&r_pud, 2);

    switch (pm) {
      case tPinDIN:
        r_c |= msk;
        r_pe &= ~msk;
        r_pud |= msk;
        break;
      case tPinDIN_PU:
        r_c |= msk;
        r_pe |= msk;
        r_pud |= msk;
        break;
      case tPinDIN_PD:
        r_c |= msk;
        r_pe |= msk;
        r_pud &= ~msk;
        break;
      case tPinDOUT:
        r_c &= ~msk;
        r_pe &= ~msk;
        r_pud |= msk;
        break;
      default:
        break;
    }
    PCAL_reg_write (0x0c, (const uint8_t *)&r_c, 2);
    PCAL_reg_write (0x4c, (const uint8_t *)&r_pe, 2);
    PCAL_reg_write (0x50, (const uint8_t *)&r_pud, 2);
  }
  if ((xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) xSemaphoreGive (_mutex);
  return result;
}

//! Digital input.
uint16_t CEXIO::get_gpio (void) {
  if ((xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) xSemaphoreTake (_mutex, portMAX_DELAY);
  uint16_t r;
  PCAL_reg_read (0x00, (uint8_t *)&r, 2);
  if ((xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) xSemaphoreGive (_mutex);
  return r;
}

//! Digital output.
bool CEXIO::set_gpio (uint16_t out) {
  if ((xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) xSemaphoreTake (_mutex, portMAX_DELAY);
  bool result = PCAL_reg_write (0x04, (const uint8_t *)&out, 2) == 2;
  if ((xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) xSemaphoreGive (_mutex);
  return result;
}

CEXIO *CEXIO::anchor = NULL;
