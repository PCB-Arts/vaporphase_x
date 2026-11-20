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

#include <cmsis_os2.h>
#include <sd.hpp>
#include <stdbool.h>
#include <string.h>
#include <tinyprintf.h>

#include "logging.h"
#include "memory_card.h"
#include "reflow_log.hpp"

#include "log.h"

#include "ProfileParser.hpp"

osThreadId_t SD_TaskHandle;
const osThreadAttr_t SD_Task_attributes = []
{
  osThreadAttr_t attr;
  attr.name = "SDTask";
  attr.stack_size = 2048 * 4;
  attr.priority = (osPriority_t)osPriorityLow;
  return attr;
}();

int DataLoadCounter = -1;
std::vector<std::string> AvailableCSVProfiles;

static void SD_Task(void *argument);

volatile SD_ErrorTypeDef SD_error_state;

static volatile bool SD_mounted = false;
static volatile bool SD_should_unmount = false;
static volatile bool SD_should_mount = false;

#define SD_PRESENCE_CHECK_INTERVAL_MS 1000
// A single failed check can be a transfer error or a card that is still busy,
// so only this many failed checks in a row count as removed
#define SD_PRESENCE_CHECK_MAX_FAILURES 3
static uint32_t SD_last_presence_check_ms = 0;
static int SD_presence_check_failures = 0;

void SD_Init()
{
  // BSP_SD_Detect_MspInit(NULL, NULL);

  SD_TaskHandle = osThreadNew(SD_Task, NULL, &SD_Task_attributes);

  SD_Log_Init();
}

static void SD_Mount()
{

  bool mount_success = false;

  if (!SD_mounted)
  {
    mount_success = sd_mount() == 0;
  }

  vpx_log("SD mount status: %s", mount_success ? "true" : "false");

  SD_error_state.mount_failed = !mount_success;
  SD_error_state.removed_without_unmount = false;
  SD_mounted = mount_success;

  if (!mount_success)
  {
    sd_unmount();
    SD_should_mount = false;
    return;
  }

  // sd_unmount();

  AvailableCSVProfiles.clear();
  auto files = FindCSVFiles();
  vpx_log("Found valid CSV files");
  for (auto file : files)
  {
    if (IsSolderprofileCSV(file))
    {
      vpx_log("	%s", file.c_str());

      AvailableCSVProfiles.push_back(file);
    }
  }

  SD_should_mount = false;
  DataLoadCounter++;
}

// The reason goes into the last line of the log file. It is written directly:
// lines logged now only reach the card with the next mount.
static void SD_Unmount(const char *reason)
{
  ReflowLog_OnUnmount();
  SD_Log_OnUnmount();

  vpx_log("SD unmounted: %s", reason);

  char line[100];
  tfp_snprintf(line, sizeof(line), "x - DISP: SD unmounted: %s.", reason);
  sd_log_write_line(sizeof(line), line);

  DataLoadCounter++;
  AvailableCSVProfiles.clear();

  sd_unmount();

  SD_mounted = false;
  SD_should_unmount = false;
  SD_presence_check_failures = 0;
}

// A card pulled without eject is unmounted automatically (after
// SD_PRESENCE_CHECK_MAX_FAILURES failed checks), so nothing keeps writing to
// it and the UI offers "mount" again
static void SD_CheckPresence()
{
  if (HAL_GetTick() - SD_last_presence_check_ms < SD_PRESENCE_CHECK_INTERVAL_MS)
  {
    return;
  }
  SD_last_presence_check_ms = HAL_GetTick();

  const int fatfs_result = sd_card_check();
  if (fatfs_result == FR_OK)
  {
    SD_presence_check_failures = 0;
    return;
  }

  SD_presence_check_failures++;
  vpx_log("SD card check failed (FatFs result %d), %d of %d", fatfs_result,
          SD_presence_check_failures, SD_PRESENCE_CHECK_MAX_FAILURES);
  if (SD_presence_check_failures < SD_PRESENCE_CHECK_MAX_FAILURES)
  {
    return;
  }

  char reason[48];
  tfp_snprintf(reason, sizeof(reason), "removal detected (FatFs result %d)",
               fatfs_result);
  SD_Unmount(reason);
  SD_error_state.removed_without_unmount = true;
}

static void SD_Mount_Worker()
{
  if (SD_mounted)
  {
    if (SD_should_unmount)
    {
      SD_Unmount("ejected");
    }
    else
    {
      SD_CheckPresence();
    }
  }
  else
  {
    if (SD_should_mount)
    {
      SD_Mount();
    }
  }
}

static void SD_Task(void *argument)
{
  // if (BSP_SD_IsDetected()) {
  SD_Mount();

  if (SD_mounted)
  {
    vpx_log("SD mounted on boot.");
  }

  while (true)
  {
    SD_Mount_Worker();
    // Not while the card fails the presence check: a failed write drops the
    // lines taken from the log buffer. They stay buffered until the card
    // answers again or is mounted again.
    if (SD_mounted && SD_presence_check_failures == 0)
    {
      SD_Log_Worker();
    }
    ReflowLog_Worker();

    osDelay(10);
  }
}

bool SD_Mounted() { return SD_mounted; }

void SD_SheduleUnmount()
{
  if (SD_mounted)
  {
    SD_should_unmount = true;
  }
}

void SD_SheduleMount()
{
  if (!SD_mounted)
  {
    SD_should_mount = true;
  }
}

int GetFileLoadCounter() { return DataLoadCounter; }

std::vector<std::string> GetAvailableCSVFiles()
{
  std::vector<std::string> copy(AvailableCSVProfiles);
  return copy;
}
