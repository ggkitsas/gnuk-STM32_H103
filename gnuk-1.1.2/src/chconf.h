/* ChibiOS/RT configuration file */

#ifndef _CHCONF_H_
#define _CHCONF_H_

#include <config.h>
#ifdef DFU_SUPPORT
#define CORTEX_VTOR_INIT (0x00003000+0x00001000)
#else
#define CORTEX_VTOR_INIT 0x00001000
#endif

#define CH_FREQUENCY                    1000
#define CH_TIME_QUANTUM                 20
#define CH_USE_NESTED_LOCKS             FALSE
#define CH_MEMCORE_SIZE                 0 /* Whole RAM */
#define CH_NO_IDLE_THREAD               FALSE
#define CH_OPTIMIZE_SPEED               TRUE
#define CH_USE_REGISTRY                 TRUE
#define CH_USE_WAITEXIT                 TRUE
#define CH_USE_SEMAPHORES               TRUE
#define CH_USE_SEMAPHORES_PRIORITY      FALSE
#define CH_USE_SEMSW                    FALSE
#define CH_USE_MUTEXES                  TRUE
#define CH_USE_CONDVARS                 TRUE
#define CH_USE_CONDVARS_TIMEOUT         TRUE
#define CH_USE_EVENTS                   TRUE /* We use this! */
#define CH_USE_EVENTS_TIMEOUT           TRUE /* We use this! */
#define CH_USE_MESSAGES                 FALSE
#define CH_USE_MESSAGES_PRIORITY        FALSE
#define CH_USE_MAILBOXES                FALSE
#define CH_USE_QUEUES                   FALSE
#define CH_USE_MEMCORE                  TRUE
#define CH_USE_HEAP                     TRUE
#define CH_USE_MALLOC_HEAP              FALSE
#define CH_USE_MEMPOOLS                 FALSE
#define CH_USE_DYNAMIC                  TRUE

/* Debug options */
#define CH_DBG_ENABLE_CHECKS            FALSE
#define CH_DBG_ENABLE_ASSERTS           FALSE
#define CH_DBG_ENABLE_TRACE             FALSE
#define CH_DBG_ENABLE_STACK_CHECK       TRUE
#define CH_DBG_FILL_THREADS             FALSE
#define CH_DBG_THREADS_PROFILING        FALSE

#define THREAD_EXT_FIELDS                                               \
  /* Add threads custom fields here.*/                                  \

#define THREAD_EXT_INIT(tp) {                                           \
  /* Add threads initialization code here.*/                            \
}

#define THREAD_CONTEXT_SWITCH_HOOK(ntp, otp) {                              \
  /* System halt code here.*/                                               \
}

#define THREAD_EXT_EXIT(tp) {                                           \
  /* Add threads finalization code here.*/                              \
}

#define IDLE_LOOP_HOOK() {                                              \
  /* Idle loop code here.*/                                             \
}

#define SYSTEM_TICK_EVENT_HOOK() {                                          \
  /* System tick event code here.*/                                         \
}

#define SYSTEM_HALT_HOOK() {                                                \
  /* System halt code here.*/                                               \
}

#endif  /* _CHCONF_H_ */
