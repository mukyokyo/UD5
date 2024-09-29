/*!
  @file    ud5_pcm.cpp
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
// PCM Audio player
//=======================================================================
/*!
  Dynamic range compression table (Consumes 4 kbytes of ROM)
  @note For preventing crackling during PCM playback.
 */
struct CcompressTab {
  uint8_t ary[4096];
  constexpr CcompressTab() : ary() {
    for (int i = 0; i < 4096; i++) ary[i] = __builtin_atan ((i - 2048) * 0.006) / M_PI_2 * 134 + 128;
  }
};

static constexpr CcompressTab compressTab;

/*!
 @brief PCM Playback class.
 @note
   PCM data is played back via the LPC845 DAC.
 */
// pinfo  :TPCMInfo
// vol    :0..127 default volume
CPCM::CPCM (std::vector<TPCMInfo> *pinfo, uint8_t vol) {
  anchor = this;

  ppcminfo = pinfo;
  volume = vol;

  pcuston_callback = NULL;

  // init dac pin
  const TPin pin = { _GPIO7, PIO_TYPE_FIXED, SWM_FIXED_DAC_0, PIO_MODE_DEFAULT }; // DAC0
  PIO_Configure (&pin, 1);
  LPC_IOCON->PIO0_17 |= IOCON_PIO_DACMODE_M;
  Chip_DAC0_Init();
  Chip_DAC_ConfigDMAConverterControl (LPC_DAC0, 0);
  Chip_DAC_SetDMATimeOut (LPC_DAC0, 0);

  // init mrt
  Chip_MRT_SetInterval (LPC_MRT_CH0, (Chip_Clock_GetSystemClockRate() / _UPDATE_FREQ) | MRT_INTVAL_LOAD);
  Chip_MRT_SetEnabled (LPC_MRT_CH0);
  Chip_MRT_SetMode (LPC_MRT_CH0, MRT_MODE_REPEAT);
  NVIC_EnableIRQ (MRT_IRQn);
}

CPCM::~CPCM() {
  pcuston_callback = NULL;
  NVIC_DisableIRQ (MRT_IRQn);
  LPC_MRT_CH0->CTRL = 0;
  Chip_DAC0_DeInit();
  LPC_IOCON->PIO0_17 &= ~IOCON_PIO_DACMODE_M;
}

//! Starts playback process in the background.
void CPCM::begin (void) {
  NVIC_EnableIRQ (MRT_IRQn);
}

//! Ends the playback process in the background.
void CPCM::end (void) {
  for (size_t i = 0; i < ppcminfo->size(); i++) stop (i);
  NVIC_DisableIRQ (MRT_IRQn);
}

//! Excitation of PCM reproduction
// playback is performed by MRT and DAC
void CPCM::play (uint8_t ind, uint8_t tone, uint8_t vol) {
  const uint16_t pcm_tone_table[12] = { 8192, 8679, 9195, 9742, 10321, 10935, 11585, 12274, 13004, 13777, 14596, 15464 }; // oct=9
  NVIC_EnableIRQ (MRT_IRQn);
  if ((ind < ppcminfo->size()) && (tone <= 127)) {
    div_t o_t = div (tone, 12);
    ppcminfo->data()[ind].ind = -1;
    ppcminfo->data()[ind].freq = pcm_tone_table[o_t.rem] >> (9 - o_t.quot);
    ppcminfo->data()[ind].vol = vol;
    ppcminfo->data()[ind].ind = 0;
  }
}

//! Stops the specified PCM during playback.
void CPCM::stop (uint8_t ind) {
  if (ind < ppcminfo->size()) ppcminfo->data()[ind].ind = -1;
}

//! Set master volume.
uint8_t CPCM::set_volume (int v) {
  return (volume = MAX (MIN (v, 127), 0));
}

//! Get current master volume.
uint8_t CPCM::get_volume (void) {
  return volume;
}

//! Set your own function to be called on interrupt
void CPCM::set_custom_cb(int32_t (*cb)(void)) {
  pcuston_callback = cb;
}

//! MRT interrupt callback
void CPCM::mrt_cb (void) {
  static int32_t v = 0;
  // Prevention of sound cracking & Resolution change
  Chip_DAC_UpdateValue (LPC_DAC0, ((((compressTab.ary[v + 2047] << 2) - 511) * CPCM::anchor->volume) >> 7) + 511);

  if (pcuston_callback != NULL) v = pcuston_callback();
  else v =0 ;

  for (size_t i = 0; i < ppcminfo->size(); i++) {
    if (ppcminfo->data()[i].ind >= 0) {
      ppcminfo->data()[i].ind += ppcminfo->data()[i].freq;
      if ((ppcminfo->data()[i].ind >> 8) >= ppcminfo->data()[i].size) ppcminfo->data()[i].ind = -1;
      else v += (((ppcminfo->data()[i].raw[ppcminfo->data()[i].ind >> 8]) << 2) * ppcminfo->data()[i].vol) >> 7;
    }
  }
  v = MIN (MAX (v, SHRT_MIN), SHRT_MAX);
}

CPCM *CPCM::anchor = NULL;

// MRTINT interrupt routine
//-----------------------------------
//! Interrupt handler for MRT
//! @note Call from CPCM.
extern "C" void MRT_IRQHandler (void) {
  Chip_MRT_IntClear (LPC_MRT_CH0);
  CPCM::anchor->mrt_cb();
}
