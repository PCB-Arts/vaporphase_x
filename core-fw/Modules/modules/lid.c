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

//=================================
// included dependencies
#include "modules/lid.h"
#include "log.h"
#include "main.h"
#include "modules/encoder.h"
#include "modules/event.h"
#include "modules/lid_isr.h"
#include "modules/lift.h"
#include "modules/motor.h"
#include "peripherals/pwm.h"

//=================================
// definitions

#define LID_MAX INT32_MAX
#define LID_MIN INT32_MIN

static struct encoder_t lid_encoder;
static bool lid_overcurrent = false;

// use an unused pin to get this compiled
static struct motor_t lid_motor = MOTOR_TPL(
    IO_PIN_TPL(LID_MOTOR_HALFBRIDGE_A_GPIO_Port, LID_MOTOR_HALFBRIDGE_A_Pin),
    IO_PIN_TPL(LID_MOTOR_HALFBRIDGE_B_GPIO_Port, LID_MOTOR_HALFBRIDGE_B_Pin));

struct axis_t lid_axis =
    AXIS_TPL(LID_MIN, LID_MAX, &lid_motor, &lid_encoder,
             IO_PIN_TPL(LID_END_TOP_GPIO_Port, LID_END_TOP_Pin),
             IO_PIN_TPL(LID_END_BOTTOM_GPIO_Port, LID_END_BOTTOM_Pin));

static const IO_pin_t overcurrent_pin =
    IO_PIN_TPL(LID_OVERCURRENT_GPIO_Port, LID_OVERCURRENT_Pin);

void lid_init()
{
  lid_axis.lim_sw_bwd.trigger_propagation_delay_ms = 50;
  lid_axis.lim_sw_fwd.trigger_propagation_delay_ms = 50;
}

// Logs start and stop of every lid move, so a lid that reaches its limit
// switch late or pushes against something (overcurrent) shows in the log
static enum direction_t lid_logged_direction = DIRECTION_NONE;
static uint32_t lid_move_started_ms;
static bool lid_move_overcurrent_logged;

static void lid_move_log_worker()
{
  const enum direction_t direction = lid_motor.direction;
  const int elapsed_ms = (int)(HAL_GetTick() - lid_move_started_ms);

  if (direction == lid_logged_direction)
  {
    if (direction != DIRECTION_NONE && !lid_move_overcurrent_logged &&
        iopin_read(overcurrent_pin) == GPIO_PIN_SET)
    {
      lid_move_overcurrent_logged = true;
      vpx_log("Lid overcurrent after %d ms", elapsed_ms);
    }
    return;
  }

  if (lid_logged_direction != DIRECTION_NONE)
  {
    const bool opening = lid_logged_direction == FWD;
    const bool at_switch = opening
                               ? axis_forward_limit_switch_active(&lid_axis)
                               : axis_backward_limit_switch_active(&lid_axis);
    vpx_log("Lid %s stopped after %d ms: %s", opening ? "opening" : "closing",
            elapsed_ms,
            at_switch ? "limit switch reached" : "limit switch not reached");
  }

  if (direction != DIRECTION_NONE)
  {
    vpx_log("Lid %s started", direction == FWD ? "opening" : "closing");
    lid_move_started_ms = HAL_GetTick();
    lid_move_overcurrent_logged = false;
  }

  lid_logged_direction = direction;
}

void lid_worker()
{
  if (iopin_read(overcurrent_pin) == GPIO_PIN_SET)
  {
    lid_overcurrent_isr();
  }
  axis_worker(&lid_axis);
  lid_move_log_worker();
}

const char *lid_status()
{
  enum axis_mode_t status = axis_status(&lid_axis);
  enum direction_t dir = lid_motor.direction;

  if (status == AXIS_ERROR)
    return "error stop";

  if (status == AXIS_NORMAL)
  {
    if (dir == FWD)
      return "ok fwd";
    if (dir == BWD)
      return "ok bwd";
    return "ok stop";
  }
  else
  {
    if (dir == FWD)
      return "cal fwd";
    if (dir == BWD)
      return "cal bwd";
    return "cal stop";
  }

  return "unknown";
}

void lid_encoder_worker() { encoder_worker(&lid_encoder); }

void lid_emergency_stop(void) { axis_emergency_stop(&lid_axis); }

void lid_overcurrent_isr()
{
  // According to hardware development no software action needed
  if (!lid_overcurrent)
  {
    vpx_log("Lid overcurrent detected");
    lid_overcurrent = true;
  }
}

void lid_calibrate() { axis_cal(&lid_axis, BWD); }

bool lid_overcurrent_was_detected() { return lid_overcurrent; }
