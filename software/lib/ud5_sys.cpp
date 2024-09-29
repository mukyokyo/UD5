/*!
  @file    ud5_sys.cpp
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

//! Excite dispatch
void _my_csw (void) {
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) taskYIELD();
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
