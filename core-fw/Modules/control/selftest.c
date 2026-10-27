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

#include "selftest.h"

#include "log.h"
#include "main.h"
#include "modules/fans.h"
#include "modules/heater.h"
#include "modules/lid.h"
#include "modules/lift.h"
#include "modules/monitor_pins.h"
#include "modules/pump.h"
#include "modules/temp_sensors.h"
#include "version.h"

typedef enum __SelftestStateWorkerReturnCode
{
  SELFTEST_STATE_WORKER_RETURN_CODE_CONTINUE,
  SELFTEST_STATE_WORKER_RETURN_CODE_COMPLETED,
} SelftestStateWorkerReturnCode;

static void selftest_state_init();
static void selftest_state_cleanup();
static void selftest_state_worker();
static void selftest_state_next();

static void selftest_lid_init();
static SelftestStateWorkerReturnCode selftest_lid_worker();
static void selftest_lid_cleanup();
static void selftest_lift_init();
static SelftestStateWorkerReturnCode selftest_lift_worker();
static void selftest_lift_cleanup();
static void selftest_pump_init();
static SelftestStateWorkerReturnCode selftest_pump_worker();
static void selftest_pump_cleanup();
static void selftest_radiator_fans_init();
static SelftestStateWorkerReturnCode selftest_radiator_fans_worker();
static void selftest_radiator_fans_cleanup();
static void selftest_quickcool_fans_init();
static SelftestStateWorkerReturnCode selftest_quickcool_fans_worker();
static void selftest_quickcool_fans_cleanup();
static void selftest_heater_init();
static SelftestStateWorkerReturnCode selftest_heater_worker();
static void selftest_heater_cleanup();

typedef enum __SelftestState
{
  SELFTEST_STATE_STOPPED,
  SELFTEST_STATE_LID,
  SELFETST_STATE_LIFT,
  SELFTEST_STATE_PUMP,
  SELFTEST_STATE_RADIATOR_FANS,
  SELFTEST_STATE_QUICKCOOL_FANS,
  SELFTEST_STATE_HEATER,
} SelftestState;

static SelftestState selftest_state;

void selftest_init() { selftest_state = SELFTEST_STATE_STOPPED; }

void selftest_start()
{
  if (selftest_active())
  {
    return;
  }

  vpx_log("FW VER v%d.%d", FW_VER_MAJOR, FW_VER_MINOR);

  const int permittedCode = 0xCA;
  const int restrictedCode = 0xFA;

  vpx_log("Third party pin state 0x%x",
          HAL_GPIO_ReadPin(THIRD_PARTY_GPIO_Port, THIRD_PARTY_Pin) ==
                  GPIO_PIN_RESET
              ? permittedCode
              : restrictedCode);

  selftest_state_next();
}

bool selftest_active() { return selftest_state != SELFTEST_STATE_STOPPED; }

void selftest_worker() { selftest_state_worker(); }

static void selftest_state_init()
{
  switch (selftest_state)
  {
  case SELFTEST_STATE_STOPPED:
    break;
  case SELFTEST_STATE_LID:
    selftest_lid_init();
    break;
  case SELFETST_STATE_LIFT:
    selftest_lift_init();
    break;
  case SELFTEST_STATE_PUMP:
    selftest_pump_init();
    break;
  case SELFTEST_STATE_RADIATOR_FANS:
    selftest_radiator_fans_init();
    break;
  case SELFTEST_STATE_QUICKCOOL_FANS:
    selftest_quickcool_fans_init();
    break;
  case SELFTEST_STATE_HEATER:
    selftest_heater_init();
    break;
  default:
    vpx_log("Error unkown selftest state reached");
  }
}

static void selftest_state_cleanup()
{
  switch (selftest_state)
  {
  case SELFTEST_STATE_STOPPED:
    break;
  case SELFTEST_STATE_LID:
    selftest_lid_cleanup();
    break;
  case SELFETST_STATE_LIFT:
    selftest_lift_cleanup();
    break;
  case SELFTEST_STATE_PUMP:
    selftest_pump_cleanup();
    break;
  case SELFTEST_STATE_RADIATOR_FANS:
    selftest_radiator_fans_cleanup();
    break;
  case SELFTEST_STATE_QUICKCOOL_FANS:
    selftest_quickcool_fans_cleanup();
    break;
  case SELFTEST_STATE_HEATER:
    selftest_heater_cleanup();
    vpx_log("Selftest completed.");
    break;
  }
}

static void selftest_state_worker()
{
  SelftestStateWorkerReturnCode returnCode =
      SELFTEST_STATE_WORKER_RETURN_CODE_COMPLETED;

  switch (selftest_state)
  {
  case SELFTEST_STATE_STOPPED:
    returnCode = SELFTEST_STATE_WORKER_RETURN_CODE_CONTINUE;
    break;
  case SELFTEST_STATE_LID:
    returnCode = selftest_lid_worker();
    break;
  case SELFETST_STATE_LIFT:
    returnCode = selftest_lift_worker();
    break;
  case SELFTEST_STATE_PUMP:
    returnCode = selftest_pump_worker();
    break;
  case SELFTEST_STATE_RADIATOR_FANS:
    returnCode = selftest_radiator_fans_worker();
    break;
  case SELFTEST_STATE_QUICKCOOL_FANS:
    returnCode = selftest_quickcool_fans_worker();
    break;
  case SELFTEST_STATE_HEATER:
    returnCode = selftest_heater_worker();
    break;
  }

  if (returnCode == SELFTEST_STATE_WORKER_RETURN_CODE_COMPLETED)
  {
    selftest_state_next();
  }
}

static void selftest_state_next()
{
  // cleanup old state
  selftest_state_cleanup();

  switch (selftest_state)
  {
  case SELFTEST_STATE_STOPPED:
    selftest_state = SELFTEST_STATE_LID;
    break;
  case SELFTEST_STATE_LID:
    selftest_state = SELFETST_STATE_LIFT;
    break;
  case SELFETST_STATE_LIFT:
    selftest_state = SELFTEST_STATE_PUMP;
    break;
  case SELFTEST_STATE_PUMP:
    selftest_state = SELFTEST_STATE_RADIATOR_FANS;
    break;
  case SELFTEST_STATE_RADIATOR_FANS:
    selftest_state = SELFTEST_STATE_QUICKCOOL_FANS;
    break;
  case SELFTEST_STATE_QUICKCOOL_FANS:
    selftest_state = SELFTEST_STATE_HEATER;
    break;
  case SELFTEST_STATE_HEATER:
    selftest_state = SELFTEST_STATE_STOPPED;
    break;
  }

  // init new state
  selftest_state_init();
}

// Lid Selftest

#define SELFTEST_LID_LOG_INTERVAL_MILLISECONDS 250
#define SELFTEST_LID_WAIT_TIMEOUT_MILLISECONDS 500

typedef enum __SelftestLidState
{
  SELFTEST_LID_STATE_CAL,
  SELFTEST_LID_AFTER_CAL_WAIT,
  SELFTEST_LID_STATE_UP,
  SELFTEST_LID_AFTER_UP_WAIT,
  SELFTEST_LID_STATE_DOWN,
} SelftestLidState;

uint32_t selftest_lid_last_logged_at;
uint32_t selftest_wait_started;
static SelftestLidState selftest_lid_state;
static bool selftest_lift_move_started;

static void selftest_lid_init()
{
  vpx_log("start lid selftest");

  selftest_lid_last_logged_at = HAL_GetTick();
  selftest_lid_state = SELFTEST_LID_STATE_CAL;
  selftest_lift_move_started = false;
  lid_calibrate();
}

static SelftestStateWorkerReturnCode selftest_lid_worker()
{

  if (lid_overcurrent_was_detected())
  {
    vpx_log("lid selftest overcurrent was set. lid position: %d",
            axis_position(&lid_axis));
    return SELFTEST_STATE_WORKER_RETURN_CODE_COMPLETED;
  }

  if (HAL_GetTick() - selftest_lid_last_logged_at >=
      SELFTEST_LID_LOG_INTERVAL_MILLISECONDS)
  {
    selftest_lid_last_logged_at = HAL_GetTick();

    vpx_log("lid raw current: %d", current_monitor_lid_value());
  }

  switch (selftest_lid_state)
  {
  case SELFTEST_LID_STATE_CAL:
  {
    if (axis_calibrated(&lid_axis))
    {
      vpx_log("lid position after calibration: %d", axis_position(&lid_axis));

      selftest_lid_state = SELFTEST_LID_AFTER_CAL_WAIT;
      selftest_wait_started = HAL_GetTick();
    }
    break;
  }

  case SELFTEST_LID_AFTER_CAL_WAIT:
  {
    if (HAL_GetTick() - selftest_wait_started >=
        SELFTEST_LID_WAIT_TIMEOUT_MILLISECONDS)
    {

      if (!selftest_lift_move_started)
      {
        axis_move_to(&lift_axis, LIFT_TOP_POSITION);
        selftest_lift_move_started = true;
      }
      if (axis_stopped(&lift_axis))
      {
        selftest_lid_state = SELFTEST_LID_STATE_UP;
        axis_move(&lid_axis, FWD);
      }
    }
    break;
  }

  case SELFTEST_LID_STATE_UP:
  {
    if (axis_stopped(&lid_axis))
    {

      vpx_log("lid position after move up: %d", axis_position(&lid_axis));

      selftest_lid_state = SELFTEST_LID_AFTER_UP_WAIT;
      selftest_wait_started = HAL_GetTick();
    }

    break;
  }

  case SELFTEST_LID_AFTER_UP_WAIT:
  {
    if (HAL_GetTick() - selftest_wait_started >=
        SELFTEST_LID_WAIT_TIMEOUT_MILLISECONDS)
    {

      selftest_lid_state = SELFTEST_LID_STATE_DOWN;
      axis_move(&lid_axis, BWD);
    }
    break;
  }

  case SELFTEST_LID_STATE_DOWN:
  {
    if (axis_stopped(&lid_axis))
    {
      vpx_log("lid position after move down: %d", axis_position(&lid_axis));

      return SELFTEST_STATE_WORKER_RETURN_CODE_COMPLETED;
    }
    break;
  }
  }

  return SELFTEST_STATE_WORKER_RETURN_CODE_CONTINUE;
}

static void selftest_lid_cleanup()
{
  motor_stop(lid_axis.motor);
  vpx_log("end lid selftest");
}

// Lift Selftest

#define SELFTEST_LIFT_LOG_INTERVAL_MILLISECONDS 250

typedef enum __SelftestLiftState
{
  SELFTEST_LIFT_STATE_CAL,
  SELFTEST_LIFT_STATE_DOWN,
  SELFTEST_LIFT_STATE_UP,
} SelftestLiftState;

uint32_t selftest_lift_last_logged_at;
static SelftestLiftState selftest_lift_state;

static void selftest_lift_init()
{
  vpx_log("start lift selftest");

  selftest_lift_last_logged_at = HAL_GetTick();
  selftest_lift_state = SELFTEST_LIFT_STATE_CAL;
  lift_calibrate();
}
static SelftestStateWorkerReturnCode selftest_lift_worker()
{
  if (HAL_GetTick() - selftest_lift_last_logged_at >=
      SELFTEST_LIFT_LOG_INTERVAL_MILLISECONDS)
  {
    selftest_lift_last_logged_at = HAL_GetTick();

    vpx_log("lift raw current: %d", current_monitor_lift_value());
  }

  switch (selftest_lift_state)
  {
  case SELFTEST_LIFT_STATE_CAL:
  {
    if (axis_calibrated(&lift_axis))
    {
      vpx_log("lift position after calibration: %d", axis_position(&lift_axis));

      selftest_lift_state = SELFTEST_LIFT_STATE_DOWN;
      axis_move(&lift_axis, BWD);
    }

    break;
  }

  case SELFTEST_LIFT_STATE_DOWN:
  {
    if (axis_stopped(&lift_axis))
    {
      vpx_log("lift position after move down: %d", axis_position(&lift_axis));

      selftest_lift_state = SELFTEST_LIFT_STATE_UP;
      axis_move(&lift_axis, FWD);
    }

    break;
  }

  case SELFTEST_LIFT_STATE_UP:
  {
    if (axis_stopped(&lift_axis))
    {
      vpx_log("lift position after move up: %d", axis_position(&lift_axis));

      return SELFTEST_STATE_WORKER_RETURN_CODE_COMPLETED;
    }

    break;
  }
  }

  return SELFTEST_STATE_WORKER_RETURN_CODE_CONTINUE;
}

static void selftest_lift_cleanup()
{
  motor_stop(lift_axis.motor);
  vpx_log("end lift selftest");
}

// Pump Selftest

#define SELFTEST_PUMP_DURATION_MILLISECONDS 2000
#define SELFTEST_PUMP_LOG_INTERVAL_MILLISECONDS 250
uint32_t selftest_pump_started_at;
uint32_t selftest_pump_last_logged_at;

static void selftest_pump_init()
{
  vpx_log("start pump selftest");
  selftest_pump_started_at = HAL_GetTick();
  selftest_pump_last_logged_at = selftest_pump_started_at;

  pump_set_auto_enabled(false);
  pump_enable();
}
static SelftestStateWorkerReturnCode selftest_pump_worker()
{
  if (HAL_GetTick() - selftest_pump_last_logged_at >=
      SELFTEST_PUMP_LOG_INTERVAL_MILLISECONDS)
  {
    selftest_pump_last_logged_at = HAL_GetTick();

    vpx_log("pump raw current: %d", current_monitor_pump_value());
  }

  return HAL_GetTick() - selftest_pump_started_at >=
         SELFTEST_PUMP_DURATION_MILLISECONDS;
}
static void selftest_pump_cleanup()
{
  pump_disable();
  pump_set_auto_enabled(true);

  vpx_log("end pump selftest");
}

// Radiator Fans Selftest

#define SELFTEST_RADIATOR_FANS_DURATION_MILLISECONDS 2000
#define SELFTEST_RADIATOR_FANS_LOG_INTERVAL_MILLISECONDS 250
uint32_t selftest_radiator_fans_started_at;
uint32_t selftest_radiator_fans_last_logged_at;

static void selftest_radiator_fans_init()
{
  vpx_log("start radiator fans selftest");
  selftest_radiator_fans_started_at = HAL_GetTick();
  selftest_radiator_fans_last_logged_at = selftest_radiator_fans_started_at;

  // disable pump auto control, as this controls the auto settings of the
  // radiator fans
  pump_set_auto_enabled(false);
  fan_set_auto_enabled(&hydro_cooler, false);
  fan_set_level(&hydro_cooler, 100);
}

static SelftestStateWorkerReturnCode selftest_radiator_fans_worker()
{
  if (HAL_GetTick() - selftest_radiator_fans_last_logged_at >=
      SELFTEST_RADIATOR_FANS_LOG_INTERVAL_MILLISECONDS)
  {
    selftest_radiator_fans_last_logged_at = HAL_GetTick();

    vpx_log("radiator fan raw current: %d", current_monitor_radiator_value());
  }

  return HAL_GetTick() - selftest_radiator_fans_started_at >=
         SELFTEST_RADIATOR_FANS_DURATION_MILLISECONDS;
}

static void selftest_radiator_fans_cleanup()
{
  fan_set_level(&hydro_cooler, hydro_cooler.target_level);
  // reenable pump auto control, this will enable the fan auto as required
  pump_set_auto_enabled(true);

  vpx_log("end pump selftest");
}

// Quick Cool Selftest

#define SELFTEST_QUICKCOOL_FANS_DURATION_MILLISECONDS 2000
#define SELFTEST_QUICKCOOL_FANS_LOG_INTERVAL_MILLISECONDS 250
uint32_t selftest_quickcool_fans_started_at;
uint32_t selftest_quickcool_fans_last_logged_at;

static void selftest_quickcool_fans_init()
{
  vpx_log("start quick cool fans selftest");
  selftest_quickcool_fans_started_at = HAL_GetTick();
  selftest_quickcool_fans_last_logged_at = selftest_quickcool_fans_started_at;

  fan_set_auto_enabled(&quick_cooler, false);
  fan_set_level(&quick_cooler, 1);
}
static SelftestStateWorkerReturnCode selftest_quickcool_fans_worker()
{
  if (HAL_GetTick() - selftest_quickcool_fans_last_logged_at >=
      SELFTEST_QUICKCOOL_FANS_LOG_INTERVAL_MILLISECONDS)
  {
    selftest_quickcool_fans_last_logged_at = HAL_GetTick();

    vpx_log("quick cool fan raw current: %d",
            current_monitor_quick_cool_value());
  }

  return HAL_GetTick() - selftest_quickcool_fans_started_at >=
         SELFTEST_QUICKCOOL_FANS_DURATION_MILLISECONDS;
}
static void selftest_quickcool_fans_cleanup()
{
  fan_set_level(&quick_cooler, quick_cooler.target_level);
  fan_set_auto_enabled(&quick_cooler, true);

  vpx_log("end quick cool fans selftest");
}

// Heater selftest

typedef enum __SelftestHeaterState
{
  SELFTEST_HEATER_HEAT_UP,
  SELFTEST_HEATER_COOL_DOWN,
} SelftestHeaterState;

#define SELFTEST_HEATER_TARGET_TEMPERATURE 150
#define SELFTEST_HEATER_COOL_DOWN_TEMPERATURE 70
#define SELFTEST_HEATER_LOG_INTERVAL_MILLISECONDS 250
static SelftestHeaterState selftest_heater_state;
uint32_t selftest_heater_fans_last_logged_at;

static void selftest_heater_init()
{
  vpx_log("start heater selftest");
  selftest_heater_fans_last_logged_at = HAL_GetTick();
  selftest_heater_state = SELFTEST_HEATER_HEAT_UP;

  fan_set_auto_enabled(&quick_cooler, false);
  fan_set_level(&quick_cooler, 0);
  heater_regulation_disable();
  heater_enable();
}

static SelftestStateWorkerReturnCode selftest_heater_worker()
{

  if (selftest_heater_state == SELFTEST_HEATER_COOL_DOWN)
  {
    return temperature_sensor_get_temperature(&temperature_sensor_galden) <
                   SELFTEST_HEATER_COOL_DOWN_TEMPERATURE
               ? SELFTEST_STATE_WORKER_RETURN_CODE_COMPLETED
               : SELFTEST_STATE_WORKER_RETURN_CODE_CONTINUE;
  }

  if (HAL_GetTick() - selftest_heater_fans_last_logged_at >=
      SELFTEST_HEATER_LOG_INTERVAL_MILLISECONDS)
  {
    selftest_heater_fans_last_logged_at = HAL_GetTick();

    vpx_log(
        "temperatures: H1: %d, H2: %d, G: %d, P: %d, W: %d",
        (int)temperature_sensor_get_temperature(&temperature_sensor_heater_1),
        (int)temperature_sensor_get_temperature(&temperature_sensor_heater_2),
        (int)temperature_sensor_get_temperature(&temperature_sensor_galden),
        (int)temperature_sensor_get_temperature(&temperature_sensor_pcb),
        (int)temperature_sensor_get_temperature(&temperature_sensor_water));

    vpx_log("otp active: %d", heater_otp_active());
  }

  float temp = temperature_sensor_get_temperature(&temperature_sensor_heater_1);
  float temp2 =
      temperature_sensor_get_temperature(&temperature_sensor_heater_2);

  // greater temperature is respected
  if (temp2 > temp)
  {
    temp = temp2;
  }

  if (temp > SELFTEST_HEATER_TARGET_TEMPERATURE)
  {
    heater_disable();

    fan_set_level(&quick_cooler, 1);

    selftest_heater_state = SELFTEST_HEATER_COOL_DOWN;
  }

  return SELFTEST_STATE_WORKER_RETURN_CODE_CONTINUE;
}

static void selftest_heater_cleanup()
{
  fan_set_level(&quick_cooler, quick_cooler.target_level);
  fan_set_auto_enabled(&quick_cooler, true);

  vpx_log("end heater selftest");
}
