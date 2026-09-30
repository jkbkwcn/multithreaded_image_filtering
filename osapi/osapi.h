#ifndef OSAPI_H
#define OSAPI_H

// check if any operating system was selected
#if (!defined _WIN32) && (!defined __linux__) && (!defined OSAPI_USE_FREERTOS) && (!defined OSAPI_USE_RTX) && (!defined OSAPI_USE_THREADX) 
#error "Unable to select operating system for OSAPI. Provide a compiler definition: OSAPI_USE_FREERTOS, OSAPI_USE_RTX or OSAPI_USE_THREADX"
#endif

#include <csignal>
#include <cstring>

#ifdef _WIN32
#include "Windows.h"
#endif

#ifdef __linux__
//#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
# ifndef PTHREAD_STACK_MIN
#  define PTHREAD_STACK_MIN 16384
# endif
#endif

#ifdef OSAPI_USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif

#ifdef OSAPI_USE_RTX
#include "cmsis_os2.h"
#endif

#ifdef OSAPI_USE_THREADX
// TODO
#endif


#include "osapi_mutex_interface.h"
#include "osapi_thread_interface.h"

#ifdef _WIN32
// include windows implementation
#include "windows/osapi_mutex_windows.h"
#include "windows/osapi_recursive_mutex_windows.h"
#include "windows/osapi_thread_windows.h"
#endif

#ifdef __linux__
// include linux implementation
#include "linux/osapi_mutex_linux.h"
#include "linux/osapi_recursive_mutex_linux.h"
#include "linux/osapi_thread_linux.h"
#endif


#ifdef OSAPI_USE_FREERTOS
// include FreeRTOS implementation
#include "freertos/osapi_mutex_freertos.h"
#include "freertos/osapi_recursive_mutex_freertos.h"
#include "freertos/osapi_thread_freertos.h"
#endif

#ifdef OSAPI_USE_RTX
// include RTX implementation
#include "rtx/osapi_mutex_rtx.h"
#include "rtx/osapi_recursive_mutex_rtx.h"
#include "rtx/osapi_thread_rtx.h"
#endif

#ifdef OSAPI_USE_THREADX
// include threadx implementation
#include "threadx/osapi_mutex_threadx.h"
#include "threadx/osapi_recursive_mutex_threadx.h"
#include "threadx/osapi_thread_threadx.h"
#endif

#include "osapi_mortal_thread.h"

namespace osapi {

/**
 * This system-related function returns the number of system ticks
 * elapsed since the system was started.
 *
 * @return current value of the system tick counter
 */
unsigned int getSystemTime();

} // namespace osapi

#endif // OSAPI_H
