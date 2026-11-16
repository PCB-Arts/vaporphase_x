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

#include "reflow_log.hpp"

#include <FreeRTOS.h>
#include <ctype.h>
#include <sd.hpp>
#include <stdlib.h>
#include <string.h>
#include <tinyprintf.h>

#include "core-communication.hpp"
#include "ff.h"
#include "log.h"
#include "main.h"
#include "version.h"

#define REFLOW_LOG_DIR "reflows"
#define REFLOW_LOG_PROFILE_NAME_LENGTH 32
#define REFLOW_LOG_SYNC_INTERVAL_LINES 10

// Set by the GUI task while no session is active, read by the SD task when a
// session starts. Profile selection is disabled during a session.
static char profile_name[REFLOW_LOG_PROFILE_NAME_LENGTH + 1] = "profile";
static const float *profile_target = NULL;
static int profile_duration_s = 0;

// Set by the GUI task, read by the SD task when a session starts
static volatile bool recording_enabled = false;

// The FIL object is 4 KB (FF_MAX_SS) and static RAM is full, so it is only
// allocated from the FreeRTOS heap while a session is recorded
static FIL *log_file = NULL;
static bool last_reflow_active = false;
static uint32_t session_start_ms;
static int32_t next_line_s;
static int lines_since_sync;

static const char *phase_name(CurrentState state)
{
  switch (state)
  {
  case CS_Preheat:
    return "Preheat";
  case CS_Soldering:
    return "Soldering";
  case CS_AntiCondensation:
    return "Quick cool";
  case CS_Calibration:
    return "Calibrating";
  default:
    return "Idle";
  }
}

// Format a value in tenths as "<int>.<digit>", tinyprintf has no float support
static void format_tenths(char *buffer, size_t size, int32_t tenths)
{
  const char *sign = tenths < 0 ? "-" : "";
  const int32_t magnitude = abs(tenths);
  tfp_snprintf(buffer, size, "%s%d.%d", sign, (int)(magnitude / 10),
               (int)(magnitude % 10));
}

static int32_t milli_to_tenths(uint64_t milli)
{
  // the core reports signed values in an unsigned property
  const int64_t value = (int64_t)milli;
  return (int32_t)((value >= 0 ? value + 50 : value - 50) / 100);
}

static bool write_text(const char *text)
{
  UINT written;
  const UINT length = strlen(text);
  return f_write(log_file, text, length, &written) == FR_OK &&
         written == length;
}

static void close_log(bool write_trailer)
{
  if (log_file == NULL)
  {
    return;
  }

  if (write_trailer)
  {
    char line[48];
    tfp_snprintf(line, sizeof(line), "### End;\nDuration;%d\n",
                 (int)((HAL_GetTick() - session_start_ms) / 1000));
    if (!write_text(line))
    {
      vpx_log("Reflow log: writing the trailer failed");
    }
  }

  if (f_close(log_file) != FR_OK)
  {
    vpx_log("Reflow log: closing the file failed");
  }

  vPortFree(log_file);
  log_file = NULL;
}

// Highest leading number of the files in REFLOW_LOG_DIR + 1
static int next_counter()
{
  DIR dir;
  FILINFO info;
  int highest = 0;

  if (f_findfirst(&dir, &info, REFLOW_LOG_DIR, "*.csv") == FR_OK)
  {
    while (info.fname[0] != '\0')
    {
      const int counter = atoi(info.fname);
      if (counter > highest)
      {
        highest = counter;
      }
      if (f_findnext(&dir, &info) != FR_OK)
      {
        break;
      }
    }
    f_closedir(&dir);
  }

  return highest + 1;
}

static void start_log()
{
  const FRESULT mkdir_result = f_mkdir(REFLOW_LOG_DIR);
  if (mkdir_result != FR_OK && mkdir_result != FR_EXIST)
  {
    vpx_log("Reflow log: creating %s failed (%d)", REFLOW_LOG_DIR,
            mkdir_result);
    return;
  }

  log_file = (FIL *)pvPortMalloc(sizeof(FIL));
  if (log_file == NULL)
  {
    vpx_log("Reflow log: out of memory, session not recorded");
    return;
  }

  char path[64];
  tfp_snprintf(path, sizeof(path), "%s/%04d_%s.csv", REFLOW_LOG_DIR,
               next_counter(), profile_name);

  const FRESULT open_result =
      f_open(log_file, path, FA_WRITE | FA_CREATE_ALWAYS);
  if (open_result != FR_OK)
  {
    vpx_log("Reflow log: creating %s failed (%d)", path, open_result);
    vPortFree(log_file);
    log_file = NULL;
    return;
  }

  char header[96];
  tfp_snprintf(header, sizeof(header),
               "### VPX Reflow Log;\nProfile;%s\nFirmware;%d.%d\n",
               profile_name, FW_VER_MAJOR, FW_VER_MINOR);

  if (!write_text(header) ||
      !write_text("Time [s];Phase;Core time [s];Target [C];PCB [C];"
                  "Galden [C];Coolant [C];Heater 1 [C];Heater 2 [C];"
                  "Heater on;Lid fan;Pump;Quick cool fan;Radiator fan;"
                  "Lid lock;Heater OTP;Coolant OTP;Lid pos;Lift pos\n"))
  {
    vpx_log("Reflow log: writing the header failed");
    close_log(false);
    return;
  }

  session_start_ms = HAL_GetTick();
  next_line_s = 0;
  lines_since_sync = 0;
  vpx_log("Reflow log: %s", path);
}

static bool write_line(int32_t time_s)
{
  const Vpx_CoreStateTypeDef state = CoreCom_CoreState();
  const Vpx_CoreMonitorStateComplete monitor = CoreCom_GetMonitorState();
  const int32_t core_time_ms = CoreCom_CoreOperationTime();

  // core time and target only exist from the start of the profile run on
  char core_time[12] = "";
  char target[12] = "";
  if (core_time_ms >= 0)
  {
    int32_t index = core_time_ms / 1000;
    tfp_snprintf(core_time, sizeof(core_time), "%d", (int)index);

    if (profile_target != NULL)
    {
      if (index > profile_duration_s)
      {
        index = profile_duration_s;
      }
      const float value = profile_target[index] * 10.0f;
      format_tenths(target, sizeof(target),
                    (int32_t)(value >= 0 ? value + 0.5f : value - 0.5f));
    }
  }

  char pcb[12];
  char galden[12];
  char coolant[12];
  format_tenths(pcb, sizeof(pcb), milli_to_tenths(CoreCom_PcbTemperature()));
  format_tenths(galden, sizeof(galden),
                milli_to_tenths(CoreCom_LiquidTemperature()));
  format_tenths(coolant, sizeof(coolant),
                milli_to_tenths(CoreCom_CoolantTemperature()));

  char line[192];
  tfp_snprintf(line, sizeof(line),
               "%d;%s;%s;%s;%s;%s;%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n",
               (int)time_s, phase_name(state.current_state), core_time, target,
               pcb, galden, coolant, (int)monitor.heater1_temp,
               (int)monitor.heater2_temp, (int)monitor.heater_on,
               (int)monitor.lid_cooling_fan_on, (int)monitor.coolant_pump_on,
               (int)monitor.quick_cool_fan_on, (int)monitor.radiator_fan_speed,
               (int)monitor.lid_lock_active, (int)monitor.heater_otp_active,
               (int)monitor.coolant_otp_active, (int)monitor.lid_position,
               (int)monitor.lift_position);

  if (!write_text(line))
  {
    return false;
  }

  if (++lines_since_sync >= REFLOW_LOG_SYNC_INTERVAL_LINES)
  {
    lines_since_sync = 0;
    return f_sync(log_file) == FR_OK;
  }
  return true;
}

void ReflowLog_Worker()
{
  const bool reflow_active = CoreCom_CoreState().reflow_active;
  const bool session_started = reflow_active && !last_reflow_active;
  const bool session_ended = !reflow_active && last_reflow_active;
  last_reflow_active = reflow_active;

  // Only record from the start of a session, not when the card is mounted or
  // recording is switched on during one
  if (session_started && recording_enabled && SD_Mounted())
  {
    start_log();
  }

  if (log_file == NULL)
  {
    return;
  }

  if (session_ended)
  {
    close_log(true);
    return;
  }

  // One line per second; if the SD card was slow, continue with the current
  // second instead of writing the missed ones
  const int32_t elapsed_s = (HAL_GetTick() - session_start_ms) / 1000;
  if (elapsed_s < next_line_s)
  {
    return;
  }
  next_line_s = elapsed_s + 1;

  if (!write_line(elapsed_s))
  {
    vpx_log("Reflow log: writing failed, recording stopped");
    close_log(false);
  }
}

void ReflowLog_OnUnmount()
{
  if (log_file != NULL)
  {
    vpx_log("Reflow log: SD card unmounted, recording stopped");
    close_log(false);
  }
}

void ReflowLog_SetEnabled(bool enabled) { recording_enabled = enabled; }

void ReflowLog_SetProfile(const char *profile_path, const float *target,
                          int duration_s)
{
  // Use the file name without folder and extension, reduced to characters
  // that are safe in a file name
  const char *name = strrchr(profile_path, '/');
  name = name != NULL ? name + 1 : profile_path;
  const char *extension = strrchr(name, '.');
  const size_t name_length =
      extension != NULL ? (size_t)(extension - name) : strlen(name);

  size_t length = 0;
  for (size_t i = 0; i < name_length && length < REFLOW_LOG_PROFILE_NAME_LENGTH;
       i++)
  {
    const char c = name[i];
    if (isalnum((unsigned char)c) || c == '_' || c == '-')
    {
      profile_name[length++] = c;
    }
  }
  profile_name[length] = '\0';

  if (length == 0)
  {
    strcpy(profile_name, "profile");
  }

  profile_target = target;
  profile_duration_s = duration_s;
}
