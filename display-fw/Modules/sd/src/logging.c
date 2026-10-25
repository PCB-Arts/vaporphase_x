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
#include <stream_buffer.h>
#include <string.h>
#include <task.h>

#include "ff.h"
#include "logging.h"
#include "memory_card.h"

#define SD_LOG_STREAM_BUFFER_SIZE_BYTES 4096
const size_t SD_Log_TriggerLevel = 1;
#define SD_LOG_WRITE_BUFFER_SIZE_BYTES 1024
const size_t SD_Log_MaxLineLength = 120;

static uint8_t SD_Log_StreamBufferStorage[SD_LOG_STREAM_BUFFER_SIZE_BYTES];

StaticStreamBuffer_t SD_Log_StreamBufferStruct;
StreamBufferHandle_t SD_Log_StreamBufferHandle;

SemaphoreHandle_t SD_Log_Semaphore = NULL;
StaticSemaphore_t SD_Log_MutexBuffer;

void SD_Log_Init(void)
{
  SD_Log_StreamBufferHandle = xStreamBufferCreateStatic(
      sizeof(SD_Log_StreamBufferStorage), SD_Log_TriggerLevel,
      SD_Log_StreamBufferStorage, &SD_Log_StreamBufferStruct);

  SD_Log_Semaphore = xSemaphoreCreateMutexStatic(&SD_Log_MutexBuffer);

  if (SD_Log_Semaphore == NULL)
  {
    SD_error_state.logging_module_error = true;
  }
}

void SD_Log(const char *s)
{
  size_t length = strnlen(s, SD_Log_MaxLineLength);

  //  the access to writing to the stream has to be synchronized
  if (SD_Log_Semaphore == NULL)
  {
    return;
  }

  if (xSemaphoreTake(SD_Log_Semaphore, (TickType_t)30) != pdTRUE)
  {
    // did not receive mutex after 30 ticks
    SD_error_state.logging_module_error = true;
    return;
  }

  if (xStreamBufferSpacesAvailable(SD_Log_StreamBufferHandle) < length + 1)
  {
    xSemaphoreGive(SD_Log_Semaphore);
    return;
  }

  size_t bytes_sent =
      xStreamBufferSend(SD_Log_StreamBufferHandle, s, length, 0);

  if (bytes_sent != length)
  {
    SD_error_state.logging_module_error = true;
    xSemaphoreGive(SD_Log_Semaphore);
    return;
  }

  bytes_sent = xStreamBufferSend(SD_Log_StreamBufferHandle, "\n", 1, 0);

  if (bytes_sent != 1)
  {
    SD_error_state.logging_module_error = true;
  }

  xSemaphoreGive(SD_Log_Semaphore);
}

static bool selftestActive = false;
static bool selftestFileOpened = false;

// time hysterese for setting selftest active to workaround stale state
static uint32_t selftest_started;
void SD_SetSelftestActive(bool active)
{
  taskENTER_CRITICAL();

  if (active && !selftestActive)
  {
    selftest_started = HAL_GetTick();
  }

  // hysterese of 3s
  if (active || HAL_GetTick() - selftest_started >= 3000)
  {
    selftestActive = active;
  }

  taskEXIT_CRITICAL();
}

char SD_Log_WriteBuffer[SD_LOG_WRITE_BUFFER_SIZE_BYTES];
void SD_Log_Worker(void)
{
  if (xStreamBufferIsEmpty(SD_Log_StreamBufferHandle))
  {
    return;
  }

  size_t bytes_to_write =
      xStreamBufferReceive(SD_Log_StreamBufferHandle, SD_Log_WriteBuffer,
                           sizeof(SD_Log_WriteBuffer), 0);

  size_t bytes_written;

  if (f_write(&g_logfile, SD_Log_WriteBuffer, bytes_to_write, &bytes_written) !=
          FR_OK ||
      bytes_written != bytes_to_write)
  {
    SD_error_state.logging_module_error = true;
    return;
  }

  if (f_sync(&g_logfile) != FR_OK)
  {
    SD_error_state.logging_module_error = true;
  }

  if (selftestActive && !selftestFileOpened)
  {
    // optimistic open TODO: add error handling
    sd_open_selftest_logfile();
    selftestFileOpened = true;
  }

  if (selftestFileOpened)
  {
    if (f_write(&g_selftestLogfile, SD_Log_WriteBuffer, bytes_to_write,
                &bytes_written) != FR_OK ||
        bytes_written != bytes_to_write)
    {
      SD_error_state.logging_module_error = true;
      return;
    }

    if (f_sync(&g_selftestLogfile) != FR_OK)
    {
      SD_error_state.logging_module_error = true;
    }
  }

  if (!selftestActive && selftestFileOpened)
  {
    sd_close_selftest_logfile();
    selftestFileOpened = false;
  }
}
