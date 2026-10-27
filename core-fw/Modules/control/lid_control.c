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

#include "lid_control.h"

#include "log.h"
#include "modules/lid.h"
#include "modules/lift.h"
#include "modules/motor.h"
#include "modules/temp_sensors.h"

#define LID_OPEN_TEMPERATURE_ECO_CELSIUS 70
#define LID_OPEN_TEMPERATURE_NORMAL_CELSIUS 80
#define LID_OPEN_TEMPERATURE_FAST_CELSIUS 90

#define LID_OPEN_TEMPERATURE_HYSTERESE_CELCIUS 2

static bool lid_prepare_opening = false;

enum service_step_t
{
  SERVICE_NONE,
  SERVICE_LIFT_TO_TOP,
  SERVICE_OPEN_LID,
};
static enum service_step_t service_step = SERVICE_NONE;
static LidOpenMode lid_control_lid_open_mode = VPX_STANDARD;
static bool lid_temperature_requirements_met = false;

static inline void update_lid_temperature_requirements()
{
  if (!lid_temperature_requirements_met)
  {
    lid_temperature_requirements_met =
        temperature_sensor_get_temperature(&temperature_sensor_galden) <
        lid_control_lid_open_temperature() -
            LID_OPEN_TEMPERATURE_HYSTERESE_CELCIUS;
  }
  else
  {
    lid_temperature_requirements_met =
        temperature_sensor_get_temperature(&temperature_sensor_galden) <
        lid_control_lid_open_temperature();
  }
}

static void service_position_worker()
{
  if (axis_has_error(&lid_axis) || axis_has_error(&lift_axis))
  {
    vpx_log("Service position aborted: axis error");
    service_step = SERVICE_NONE;
    return;
  }

  switch (service_step)
  {
  case SERVICE_NONE:
    break;

  case SERVICE_LIFT_TO_TOP:
    if (axis_stopped(&lift_axis))
    {
      axis_move(&lid_axis, FWD);
      service_step = SERVICE_OPEN_LID;
    }
    break;

  case SERVICE_OPEN_LID:
    if (axis_stopped(&lid_axis))
    {
      vpx_log(axis_forward_limit_switch_active(&lid_axis)
                  ? "Service position reached"
                  : "Service position: lid stopped before fully open");
      service_step = SERVICE_NONE;
    }
    break;
  }
}

void lid_control_init()
{
  lid_prepare_opening = false;
  service_step = SERVICE_NONE;
}

void lid_control_worker()
{
  if (lid_prepare_opening)
  {
    if (lift_lid_open_state_reached())
    {
      lid_prepare_opening = false;
      axis_move(&lid_axis, FWD);
    }
  }

  service_position_worker();

  update_lid_temperature_requirements();
}

/*
 * requirements:
 * - lid is closed
 * - lid is not in motion
 * - lid is not calibrating
 * - no errors
 */
bool lid_control_lid_closed()
{
  return lid_control_lid_state() == LID_STATE_CLOSED;
};

bool lid_control_lid_boot_ready() { return lid_control_lid_closed(); }

void lid_control_lid_open()
{
  if (lid_control_lid_state() != LID_STATE_CLOSED)
  {
    return;
  }

  lid_prepare_opening = true;
  lift_lid_open_state();
}

void lid_control_lid_close()
{
  // cancels a pending open or service sequence
  service_step = SERVICE_NONE;
  lid_prepare_opening = false;

  // The lift stays where it is, e.g. at the top after the service position.
  // Lowering it first with the open lid didn't move it on the device (the
  // step count ran down anyway). The next open moves it to the access height
  // first.
  axis_move(&lid_axis, BWD);
}

void lid_control_service_position()
{
  // With the lid open the lift doesn't move on the device, but the step count
  // runs anyway and the lift position would be wrong afterwards
  if (lid_control_lid_state() != LID_STATE_CLOSED)
  {
    return;
  }

  motor_set_speed(lift_axis.motor, LIFT_MOTOR_MAX_SPEED);
  axis_move_to(&lift_axis, LIFT_TOP_POSITION);
  service_step = SERVICE_LIFT_TO_TOP;
}

LidState lid_control_lid_state()
{
  enum axis_mode_t status = axis_status(&lid_axis);
  enum direction_t dir = lid_axis.motor->direction;

  if (status == AXIS_ERROR)
    return LID_STATE_ERROR;

  if (lid_prepare_opening || service_step != SERVICE_NONE)
    return LID_STATE_OPENING;

  if (status != AXIS_NORMAL)
    return LID_STATE_INITIALISING;

  if (dir == FWD)
    return LID_STATE_OPENING;
  if (dir == BWD)
    return LID_STATE_CLOSING;

  if (axis_backward_limit_switch_active(&lid_axis))
    return LID_STATE_CLOSED;

  return LID_STATE_OPEN;
}

void lid_control_set_lid_open_mode(LidOpenMode lidOpenMode)
{
  lid_control_lid_open_mode = lidOpenMode;
}

LidOpenMode lid_control_get_lid_open_mode()
{
  return lid_control_lid_open_mode;
}

float lid_control_lid_open_temperature()
{
  switch (lid_control_lid_open_mode)
  {
  case VPX_ECO:
    return LID_OPEN_TEMPERATURE_ECO_CELSIUS;
  case VPX_FAST:
    return LID_OPEN_TEMPERATURE_FAST_CELSIUS;
  default:
    return LID_OPEN_TEMPERATURE_NORMAL_CELSIUS;
  }
}

bool lid_control_lid_open_requirements_met()
{
  return lid_control_lid_closed() && lid_temperature_requirements_met;
}
