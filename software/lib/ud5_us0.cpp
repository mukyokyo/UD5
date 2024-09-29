/*!
  @file    ud5_us0.cpp
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
 @brief UART wrapper class for communication with Raspberry Pi
 @note
   Call LPC845 DMA version UART0 library.
 */
//! When you provide your own non-standard protocols and send/receive buffers.
CRPiIF::CRPiIF (uint32_t baud, uint8_t mode, uint16_t txb, uint16_t rxb) {
  anchor = this;
  ptxbuff = (uint8_t *)malloc (MIN (txb, 1024));
  prxbuff = (uint8_t *)malloc (MIN (rxb, 1024));

  LPC_SYSCON->UART0CLKSEL = SYSCON_FLEXCOMMCLKSELSRC_FRG0;
  PIO_Configure (pins, PIO_LISTSIZE (pins));
  usart0_dma_init (Chip_Clock_GetFRGClockRate (0), baud, mode, ptxbuff, txb, prxbuff, rxb);
  usart0_dma_csw = _my_csw;
  init = (ptxbuff != NULL) && (prxbuff != NULL);
}
CRPiIF::~CRPiIF() {
  Chip_UART_DeInit (LPC_USART0);
  Chip_DMA_DisableChannel (LPC_DMA, DMAREQ_USART0_TX);
  Chip_DMA_DisableChannel (LPC_DMA, DMAREQ_USART0_RX);
  usart0_dma_csw = _my_csw;
  free (ptxbuff);
  free (prxbuff);
  anchor = NULL;
}

//! Number of unsent data bytes
uint16_t CRPiIF::txbuff (void) {
  if (init) return usart0_dma_tx_buff();
  return 0;
}
//! Number of data bytes received
uint16_t CRPiIF::rxbuff (void) {
  if (init) return usart0_dma_rx_buff();
  return 0;
}
//! Clear transmit buffer.
void CRPiIF::clear_txbuff (void) {
  if (init) usart0_dma_tx_purge();
}
//! Clear Received data.
void CRPiIF::clear_rxbuff (void) {
  if (init) usart0_dma_rx_purge();
}
//! Send 1 byte of characters.
void CRPiIF::putc (char c) {
  if (init) usart0_dma_putc (c);
}
//! Send string.
void CRPiIF::puts (const char *s) {
  if (init) usart0_dma_puts (s);
}
//! Send specified number of bytes of data.
int CRPiIF::putsb (const uint8_t *s, int n) {
  if (init) return usart0_dma_putsb (s, n);
  return 0;
}
//! Retrieve one character from the receive data buffer.
char CRPiIF::getc (void) {
  if (init) return usart0_dma_getc();
  return -1;
}
//! Receive string.
uint16_t CRPiIF::gets (char *s, int n) {
  if (init) return usart0_dma_gets (s, n);
  return 0;
}

#if 0
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

//! Get function pointer to printf
int (*CRPiIF::pprintf()) (const char *, ...) {
  return usart0_dma_printf;
}

void CRPiIF::no_csw (void) {
  usart0_dma_csw = NULL;
}

CRPiIF *CRPiIF::anchor = NULL;
