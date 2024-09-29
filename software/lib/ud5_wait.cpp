/*!
  @file    ud5_wait.cpp
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

uint32_t CWait::get_cyccnt_diff (void) {
  wl_sem_lock (&sem);
  uint32_t tick_now, result;

  tick_now = 0x7fffffffUL - LPC_MRT_CH3->TIMER;
  if (tick_now >= oldreg) result = tick_now - oldreg;
  else                    result = (0x7fffffffUL - oldreg) + tick_now;
  oldreg = tick_now;
  mymstick += (uint64_t)result;
  wl_sem_unlock (&sem);
  return result;
}

CWait::CWait() {
  LPC_MRT_CH3->INTVAL = MRT_INTVAL_LOAD | 0x7fffffffUL;
  LPC_MRT_CH3->CTRL = MRT_MODE_REPEAT;

  if (oldreg == 0) oldreg = 0x7fffffffUL - LPC_MRT_CH3->TIMER;
  csw = _my_csw;
  sem = 0;
  mymstick = 0;
  oldreg = 0;
}

void CWait::no_csw (void) {
  csw = NULL;
}

//! Wait for microsecond.
void CWait::us (uint32_t us) {
  uint32_t topcnt = _USTICK * us;
  volatile uint32_t icnt = 0;

  do {
    icnt += get_cyccnt_diff();
    if (csw != NULL) csw();
  } while (topcnt > icnt);
}

//! Wait for milliseconds.
void CWait::ms (uint32_t ms) {
  while (ms--) us (1000);
}

//! Get elapsed time in milliseconds since immediately after startup.
uint32_t CWait::get_tick (void) {
  get_cyccnt_diff();
  return mymstick / _MSTICK;
}

CWait _wait;

//=======================================================================
// ETC
//=======================================================================
//! Wait for milliseconds.
void UD5_WAIT (uint32_t ms) {
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) vTaskDelay (ms);
  else _wait.ms (ms);
}

//! Get elapsed time in milliseconds since immediately after startup.
uint32_t UD5_GET_ELAPSEDTIME (void) {
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) return xTaskGetTickCount();
  else return _wait.get_tick();
}
