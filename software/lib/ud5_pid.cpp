/*!
  @file    ud5_pid.cpp
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
// PID
//=======================================================================
/*!
 @brief PID control class.
 @note
   General PID control only.
 */
  //! Reset PID controller.
  void CPID::reset (PPIDParam p) {
    p->err[0] = p->err[1] = p->integral = p->prevcalc = 0;
  }

  //! PID operation.
  float CPID::calc (PPIDParam p, float feedback, float target) {
    float p_term, i_term, d_term, f_term, integ, result;
    p->err[0] = p->err[1];
    p->err[1] = feedback - target;
    integ = p->integral + (p->err[1] + p->err[0]) * 0.5 * p->delta_t;
    p_term = p->Kp * p->err[1];
    i_term = p->Ki * p->integral;
    d_term = p->Kd * (p->err[1] - p->err[0]) / p->delta_t;
    f_term = p->Kf * target;
    result = p_term + i_term + d_term - f_term;

    // Reset wind-up measure
//    if ((p->max == 0) && (p->min == 0)) p->integral = integ;
//    else {
    if (result >= p->max) return (p->prevcalc = p->max);
    else if (result <= p->min) return (p->prevcalc = p->min);
    else p->integral = integ;
//    }
    return (p->prevcalc = result);
  }

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

//=======================================================================
// Initialization process called from startup routine
//=======================================================================
extern "C" void ResetExceptionDispatch_Top (void) {
  // frg0clk = frg0_src_clk / (1 + MULT / DIV)
  //         = 60,000,000 / (1 + FRG0MUL / (FRG0DIV + 1))
  //         = 60,000,000 / (1 + 35 / (39 + 1))
  //         = 60,000,000 / 1.875 = 32MHz
  LPC_SYSCON->FRG0MULT = 35;
  LPC_SYSCON->FRG0DIV = 39;
  LPC_SYSCON->FRG0CLKSEL = SYSCON_FRGCLKSRC_PLL;

  // Pre-activated here because MRT is heavily used.
  Chip_MRT_DeInit();
  Chip_MRT_Init();
}

extern "C" void ResetExceptionDispatch_Tail (void) {
  Chip_SYSCON_SetBODLevels (SYSCON_BODRSTLVL_1, SYSCON_BODINTVAL_LVL1);
  Chip_SYSCON_EnableBODReset();
}

//#ifdef __cplusplus
// Global class constructor/destructor treatment
//-----------------------------
extern "C" void (*__preinit_array_start)();
extern "C" void (*__preinit_array_end)();
extern "C" void (*__init_array_start)();
extern "C" void (*__init_array_end)();
extern "C" void (*__fini_array_end)();
extern "C" void (*__fini_array_start)();

// Iterate over all the init routines.
extern "C" void __libc_init_array (void) {
  for (void (**pf)() = &__preinit_array_start; pf < &__preinit_array_end; ++pf) (*pf)();
  for (void (**pf)() = &__init_array_start; pf < &__init_array_end; ++pf) (*pf)();
}

// Run all the cleanup routines.
extern "C" void __libc_fini_array (void) {
  for (void (**pf)() = &__fini_array_start; pf < &__fini_array_end; ++pf) (*pf)();
}
//#endif

// dummy function
extern "C" {
#if 0
  //! System Calls
  void _exit( int status ) { while (1); }
#endif
  //! System Calls
  unsigned int _getpid() { return 0; }
  //! System Calls
  int _kill (int id, int sig) { return -1; }

  //! System Calls
  caddr_t _sbrk (int incr) {
    extern int _end;
    static unsigned char *heap = NULL;
    unsigned char *prev_heap;

    if (heap == NULL) heap = (unsigned char *)&_end;
    prev_heap = heap;
    heap += incr;
    return (caddr_t)prev_heap;
  }

  //! System Calls
  int _fstat (int fd, void* buf) { return 0; }

  //! System Calls
  int _write (int fd, char *buf, int count) {
    if (CDXIF::anchor != NULL) {
      CDXIF::anchor->putsb((const uint8_t *)buf, count);
      return count;
    }
    return 0;
  }

  //! System Calls
  int _read( int fd, char *buf, int count ) { return 0; }
  //! System Calls
  int _lseek( int fd, int count ) { return 0; }
  //! System Calls
  int _close( int fd ) { return 0; }
  //! System Calls
  int _isatty( int fd ) { return 0; }
}

#if 0
extern "C" { void* __dso_handle __attribute__ ((__weak__)); }
// -fno-use-cxa-atexit
#endif
