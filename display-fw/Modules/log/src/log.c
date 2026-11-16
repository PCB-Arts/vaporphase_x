/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Copyright (c) 2021 - PCB Arts GmbH
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <sd.hpp>
#include <semphr.h>
#include <stdbool.h>
#include <tinyprintf.h>

#include "log.h"

// Several tasks log (GUI, SD, core communication), and they share the two
// buffers. A line that doesn't get the lock in time is dropped.
#define LOG_LOCK_TIMEOUT_TICKS 30

static SemaphoreHandle_t log_mutex = NULL;
static StaticSemaphore_t log_mutex_buffer;

static char log_buffer_1[128];
static char log_buffer_2[128];

void vpx_log_init(void)
{
  log_mutex = xSemaphoreCreateMutexStatic(&log_mutex_buffer);
}

// Before vpx_log_init() only main() runs, so there is nothing to lock yet
static bool log_lock(void)
{
  return log_mutex == NULL ||
         xSemaphoreTake(log_mutex, LOG_LOCK_TIMEOUT_TICKS) == pdTRUE;
}

static void log_unlock(void)
{
  if (log_mutex != NULL)
  {
    xSemaphoreGive(log_mutex);
  }
}

static void component_log_locked(const char *message, const char *component)
{
  tfp_snprintf(log_buffer_2, sizeof(log_buffer_2), "%d:%s: %s",
               (int)osKernelGetTickCount(), component, message);

  SD_Log(log_buffer_2);
}

void vpx_component_log(const char *message, const char *component)
{
  if (!log_lock())
  {
    return;
  }
  component_log_locked(message, component);
  log_unlock();
}

void vpx_log(const char *format, ...)
{
  if (!log_lock())
  {
    return;
  }

  va_list format_arguments;

  va_start(format_arguments, format);

  // keep single space at the beginning
  tfp_vsnprintf(log_buffer_1, sizeof(log_buffer_1), format, format_arguments);

  va_end(format_arguments);

  component_log_locked(log_buffer_1, "DISP");
  log_unlock();
}
