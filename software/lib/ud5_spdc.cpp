/*!
  @file    ud5_spdc.cpp
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
#define _SPD_DIV 16   //!< 2 or more and multiple of 2, Speed calculation with time difference of (_SPD_DIV - 1) * ControlPeriod [ms].
#define _CTRL_DIV 1   //!< Control in _CTRL_DIV * 5[ms] cycles

float CSpeedControl::calcramp (float *prev, int32_t target, int32_t dt, float ramp) {
  float r;
  if (ramp > 0.0) {
    if (target > *prev) {
      r = *prev + (ramp * dt * 0.001);
      if (r > target) r = target;
    } else if (target < *prev) {
      r = *prev - (ramp * dt * 0.001);
      if (r < target) r = target;
    } else r = target;
    return (*prev = r);
  } else return (*prev = target);
}

// 5ms cycle interval
void CSpeedControl::interval_timer_task() {
  portTickType t = xTaskGetTickCount();
  uint32_t prev_t = xTaskGetTickCount();
  uint32_t tt = prev_t;
  while (!kill_interval_timer_task) {
    if (spdctrl_div++ % _CTRL_DIV == 0) {
      tt = xTaskGetTickCount();
      spddetect_div = (spddetect_div + 1) % _SPD_DIV;
      uint8_t oldestind = (spddetect_div + 1) % _SPD_DIV;

      for (uint8_t axis = 0; axis < 2; axis++) {
        spparam[axis].current_counter = spparam[axis].counter_history[spddetect_div] = pEnc->get_encoder_count (axis);
        spparam[axis].current_speed = spparam[axis].current_counter - spparam[axis].counter_history[oldestind];
        for (int i = 0; i < 3; i++) {
          spparam[axis].counter[i] += spparam[axis].current_counter - spparam[axis].prev_enc;
        }
        spparam[axis].prev_enc = spparam[axis].current_counter;
        if (control_on) {
          int32_t duty;
          calcramp (&spparam[axis].ramped_target_speed, spparam[axis].target_speed, spparam[axis].ramp, tt - prev_t);
          duty = spparam[axis].pid.calc (spparam[axis].gain, spparam[axis].current_speed, spparam[axis].ramped_target_speed);
          // Update PWM Duty
          pMot->set_duty (axis, duty);
        }
      }
      prev_t = tt;
    }
    vTaskDelayUntil (&t, 5);
  }
  interval_timer_task_handle = NULL;
  vTaskDelete (NULL);
}

CSpeedControl::CSpeedControl (CMotor *m, CGPIO *e) {
  pEnc = e;
  pMot = m;
  spddetect_div = spdctrl_div = 0;
  kill_interval_timer_task = false;

  for (uint8_t axis = 0; axis < 2; axis++) {
    for (int i = 0; i < _SPD_DIV; i++) spparam[axis].counter_history[i] = 0;
    spparam[axis].prev_enc = 0;
  }
}

//! Reset all control operation results.
void CSpeedControl::reset_gain (void) {
  for (uint8_t axis = 0; axis < 2; axis++) {
    spparam[axis].pid.reset (spparam[axis].gain);
    pMot->set_duty (axis, 0);
    spparam[axis].target_speed = spparam[axis].ramped_target_speed = spparam[axis].current_speed = 0;
  }
}

//! Start speed control task.
void CSpeedControl::begin (void) {
  if (interval_timer_task_handle == NULL) {
    kill_interval_timer_task = false;
    reset_gain();
    // Interval timer task create
    xTaskCreate ([] (void *arg) { static_cast<CSpeedControl *> (arg)->interval_timer_task(); }, "SPDC", 100, this, 1, &interval_timer_task_handle);
  }
}
//! Stop speed control task.
void CSpeedControl::end (void) {
  if (interval_timer_task_handle != NULL) {
    kill_interval_timer_task = true;
    vTaskDelete (interval_timer_task_handle);
    reset_gain();
  }
}

//! Start speed control.
void CSpeedControl::start (void) {
  control_on = false;
  reset_gain();
  control_on = true;
}
//! Stop speed control.
void CSpeedControl::stop (void) {
  control_on = false;
  reset_gain();
}

//! Set control gain for each axis.
void CSpeedControl::set_gain (uint8_t ch, CPID::PPIDParam g) {
  if (ch < 2) spparam[ch].gain = g;
}
//! Set the control biaxial gain.
void CSpeedControl::set_biaxial_gain (CPID::PPIDParam g0, CPID::PPIDParam g1) {
  spparam[0].gain = g0;
  spparam[1].gain = g1;
}

//! Set target speed for each axis.
void CSpeedControl::set_taget_speed (uint8_t ch, int32_t s) {
  if (ch < 2) spparam[ch].target_speed = s;
}
//! Set biaxial target speed.
void CSpeedControl::set_biaxial_taget_speed  (int32_t d0, int32_t d1) {
  spparam[0].target_speed = d0;
  spparam[1].target_speed = d1;
}
//! Get target speed of each axis.
int32_t CSpeedControl::get_target_speed (uint8_t ch) {
  if (ch > 1) return 0;
  return spparam[ch].target_speed;
}

//! Get current speed of each axis.
int32_t CSpeedControl::get_current_speed (uint8_t ch) {
  if (ch > 1) return 0;
  return spparam[ch].current_speed;
}
//! Set ramp for each axis.
void CSpeedControl::set_ramp (uint8_t ch, float r) {
  if (ch < 2) spparam[ch].ramp = r;
}
//! Set biaxial ramp.
void CSpeedControl::set_biaxial_ramp (float r0, float r1) {
  spparam[0].ramp = r0;
  spparam[1].ramp = r1;
}

//! Reseet current pulse count for each axis
void CSpeedControl::reset_counter (uint8_t ch, int n) {
  if (ch < 2) spparam[ch].counter[n] = 0;
}
//! Get current pulse count for each axis
int32_t CSpeedControl::get_counter (uint8_t ch, int n) {
  if (ch > 1) return 0;
  return spparam[ch].counter[n];
}
