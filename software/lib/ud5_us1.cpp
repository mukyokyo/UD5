/*!
  @file    ud5_us1.cpp
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
// UART
//=======================================================================
/*!
 @brief UART wrapper class converted to half-duplex I/F.
 @note
   Call LPC845 DMA version UART1 library.
 */
CDXIF::CDXIF() {
  anchor = this;
  ptxbuff = (uint8_t *)malloc (10);
  prxbuff = (uint8_t *)malloc (10);

  LPC_SYSCON->UART1CLKSEL = SYSCON_FLEXCOMMCLKSELSRC_FRG0;
  PIO_Configure (pins, PIO_LISTSIZE (pins));
  usart1_dma_init (Chip_Clock_GetFRGClockRate (0), 115200, USIP_8N1, ptxbuff, 10, prxbuff, 10);
  usart1_dma_csw = _my_csw;
  init = (ptxbuff != NULL) && (prxbuff != NULL);
}

//! When you provide your own non-standard protocols and send/receive buffers.
CDXIF::CDXIF (uint32_t baud, uint8_t mode, int txb, int rxb) {
  anchor = this;
  ptxbuff = (uint8_t *)malloc (MIN (txb, 1024));
  prxbuff = (uint8_t *)malloc (MIN (rxb, 1024));

  LPC_SYSCON->UART1CLKSEL = SYSCON_FLEXCOMMCLKSELSRC_FRG0;
  PIO_Configure (pins, PIO_LISTSIZE (pins));
  usart1_dma_init (Chip_Clock_GetFRGClockRate (0), baud, mode, ptxbuff, txb, prxbuff, rxb);
  usart1_dma_csw = _my_csw;
  init = (ptxbuff != NULL) && (prxbuff != NULL);
}

CDXIF::~CDXIF() {
  Chip_UART_DeInit (LPC_USART1);
  Chip_DMA_DisableChannel (LPC_DMA, DMAREQ_USART1_TX);
  Chip_DMA_DisableChannel (LPC_DMA, DMAREQ_USART1_RX);
  usart1_dma_csw = NULL;
  free (ptxbuff);
  free (prxbuff);
  anchor = NULL;
}

//! Number of unsent data bytes
uint16_t CDXIF::txbuff (void) {
  if (init) return usart1_dma_tx_buff();
  return 0;
}
//! Number of data bytes received
uint16_t CDXIF::rxbuff (void) {
  if (init) return usart1_dma_rx_buff();
  return 0;
}
//! Clear transmit buffer.
void CDXIF::clear_txbuff (void) {
  if (init) usart1_dma_tx_purge();
}
//! Clear Received data.
void CDXIF::clear_rxbuff (void) {
  if (init) usart1_dma_rx_purge();
}
//! Send 1 byte of characters.
void CDXIF::putc (char c) {
  if (init) usart1_dma_putc (c);
}
//! Send string.
void CDXIF::puts (const char *s) {
  if (init) usart1_dma_puts (s);
}
//! Send specified number of bytes of data.
int CDXIF::putsb (const uint8_t *s, int n) {
  if (init) return usart1_dma_putsb (s, n);
  return 0;
}
//! Retrieve one character from the receive data buffer.
char CDXIF::getc (void) {
  if (init) return usart1_dma_getc();
  return -1;
}
//! Receive string.
uint16_t CDXIF::gets (char *s, int n) {
  if (init) return usart1_dma_gets (s, n);
  return 0;
}

#if 0
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

//! Get function pointer to printf
int (*CDXIF::pprintf()) (const char *, ...) {
  return usart1_dma_printf;
}

void CDXIF::no_csw (void) {
  usart1_dma_csw = NULL;
}

CDXIF *CDXIF::anchor = NULL;
