/*!
  @file    ud5_rc.cpp
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
// Radio Controlled receiver
//=======================================================================
/*!
 @brief Pulse processing class for radio-controlled radio receivers
 @note
   Use CGPIO class pulse width measurement.
 */
// Interpolation
int32_t CRCStick::_INTERP (int32_t xi, int32_t xj, int32_t yi, int32_t yj, int32_t x) {
  return yi + (((yj - yi) * (x - xi)) / (xj - xi));
}

int CRCStick::interp1dim (const int32_t x, const int32_t *ar, const int32_t w) {
  int i;
  if (x <= ar[0]) return ar[w];
  else if (x >= ar[w - 1]) return ar[w * 2 - 1];
  for (i = 1; i < w; i++) if (ar[i] >= x) break;
  return _INTERP (ar[i - 1], ar[i], ar[i + w - 1], ar[i + w], x);
}

// moving mean
void CRCStick::movingmean (int32_t *ave, int32_t dat, int32_t num) {
  *ave = (*ave * (num - 1) + dat) / num;
}

#if 0
template <typename ... Args>
int CRCStick::_printf (const char *format, Args const &... args) {
  if (CDXIF::anchor != NULL) return CDXIF::anchor->printf (format, args ...);
  return 0;
}
#endif

void CRCStick::_led (uint8_t n) {
  switch (n) {
    case 0:
      if (CEXIO::anchor != NULL) CEXIO::anchor->set_LED_off (0xf);
      break;
    case 1:
      if (CEXIO::anchor != NULL) CEXIO::anchor->set_LED_on (0xf);
      break;
    case 2:
      if (CEXIO::anchor != NULL) CEXIO::anchor->set_LED_toggle (0xf);
      break;
  }
}

void CRCStick::_play (uint8_t t, uint32_t w) {
  if (CPCM::anchor != NULL) {
    CPCM::anchor->play (0, t, 127);
    if (w > 0) UD5_WAIT (w);
  }
}

CRCStick::CRCStick (CGPIO *g) {
  cgpio = g;
  // Reads calibrated data from NVM
  __aeabi_memcpy4 (&caldata, (void *)_EEPROM_ADDR, sizeof (Tcaldata));
};

//! Calibration
//  f_nextstep:Trigger to advance the operation under calibration to the next step
void CRCStick::calibration (bool (*f_nextstep) (void), void (*f_mon) (const char *s, uint32_t *)) {
  int32_t d;
  uint32_t pls[8] = {0};

  if (f_mon != 0) f_mon ("Are you ready?     ", pls);

  UD5_WAIT (100);
  _led (0);
  _play (48, 80);
  _play (52, 80);
  _play (55, 80);

  // step 1. waiting for start condition
  while (!f_nextstep()) {
    _led (2);
    UD5_WAIT (200);
  }
  while (f_nextstep()) {
    _led (2);
    UD5_WAIT (50);
  }
  _led (0);

  for (int i = 0; i < 8; i++) {
    caldata[i].max = INT_MIN;
    pls[i] = caldata[i].neutral = cgpio->get_pwd (i);
    caldata[i].min = INT_MAX;
    if (f_mon != 0) f_mon ("Neutral pulse      ", pls);
  }

  // step 2. Measure neutral position & wait for end condition
  do {
    _led (2);
    UD5_WAIT (50);
    for (int i = 0; i < 8; i++) {
      movingmean (&caldata[i].neutral, cgpio->get_pwd (i), 20);
      pls[i] = caldata[i].neutral;
    }
    if (f_mon != 0) f_mon ("Neutral pulse      ", pls);
  } while (!f_nextstep());

  _led (0);

  while (f_nextstep()) {
    _led (2);
    UD5_WAIT (50);
  }
  _led (1);

  // step 3. Measure both end positions & wait for end condition
  do {
    uint32_t t[8], maxdif = 0;
    _led (2);
    UD5_WAIT (50);

    for (int i = 0; i < 8; i++) {
      pls[i] = d = cgpio->get_pwd (i);
      caldata[i].max = (d > caldata[i].max) ? d : caldata[i].max;
      caldata[i].min = (d < caldata[i].min) ? d : caldata[i].min;

      // Flattening of deviation from center to 48 steps
      if (d >= caldata[i].neutral)
        t[i] = (d - caldata[i].neutral) * 24 / (caldata[i].max - caldata[i].neutral);
      else if (d < caldata[i].neutral)
        t[i] = (d - caldata[i].neutral) * 24 / (caldata[i].neutral - caldata[i].min);

      // Detect the largest difference
      if ((caldata[i].neutral != 0) && (labs (t[i]) > labs (maxdif))) maxdif = (int)t[i];
    }
    if (f_mon != 0) f_mon ("Full scale pulse   ", pls);
    _play (34 + maxdif);

  } while (!f_nextstep());
  _led (0);

  while (f_nextstep()) {
    _led (2);
    UD5_WAIT (50);
  }
  _led (0);

  // step 4. Write measurement results to NVM
  vTaskSuspendAll();
  taskDISABLE_INTERRUPTS();
  taskENTER_CRITICAL();
  FLASH_Write (_EEPROM_ADDR, (uint8_t *)&caldata, sizeof (Tcaldata));
  taskEXIT_CRITICAL();
  taskENABLE_INTERRUPTS();
  xTaskResumeAll();
  _play (55, 80);
  _play (52, 80);
  _play (48, 80);
}

//! Get calibration value.
void CRCStick::get_calibration (Tcaldata *pcal) {
  __aeabi_memcpy4 (pcal, caldata, sizeof (Tcaldata));
}

//! Normalize pulses to -1000...1000 (per_new: neutral, per_ul: % of deadband to measured full scale at both ends)
int32_t CRCStick::get_normal (uint8_t ch, int32_t per_neu, int32_t per_ul) {
  static int32_t rc_1d[10] = {0, 0, 0, 0, 0, -1000, 0, 0, 0, 1000};
  per_ul = MIN (MAX (per_ul, 0), 50);
  per_neu = MIN (MAX (per_neu, 0), 50);
  rc_1d[0] = caldata[ch].neutral - ((caldata[ch].neutral - caldata[ch].min) * (100 - per_ul)) / 100;
  rc_1d[1] = caldata[ch].neutral - ((caldata[ch].neutral - caldata[ch].min) * per_neu) / 100;
  rc_1d[2] = caldata[ch].neutral;
  rc_1d[3] = caldata[ch].neutral + ((caldata[ch].max - caldata[ch].neutral) * per_neu) / 100;
  rc_1d[4] = caldata[ch].neutral + ((caldata[ch].max - caldata[ch].neutral) * (100 - per_ul)) / 100;
  return interp1dim (cgpio->get_pwd (ch), rc_1d, 5);
};
