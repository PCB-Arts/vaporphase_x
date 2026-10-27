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
#include "sequence.h"
#include "conf.h"
#include "lid_control.h"
#include "log.h"
#include "modules/axis.h"
#include "modules/fan.h"
#include "modules/fans.h"
#include "modules/heater.h"
#include "modules/lid.h"
#include "modules/lift.h"
#include "modules/pid.h"
#include "modules/pump.h"
#include "modules/regulator.h"
#include "modules/temp_sensors.h"

//=================================
// definitions

// TAL phase is reached if target temperature over
// TAL_PHASE_THRESHOLD_TEMPERATURE in °C
#define TAL_PHASE_THRESHOLD_TEMPERATURE(x) (x - 90)

enum
{
  SEQ_IDLE,            // not running
  SEQ_SAFETY_CHECK,    // lid closed?
  SEQ_GENERATE_VAPOUR, // bring the galden up to temperature
  SEQ_PRE_HEAT,        // bring the PCB up to temperature
  SEQ_RUN,             // run temperature profile
  SEQ_COOL_DOWN,       // wait until the galden has cooled down
  SEQ_CLEANUP,         // wait until lift is up
} static seq_state = SEQ_IDLE;

static uint16_t flags;
static bool is_in_tal = false;

struct sequence_t sequence;
currentState cS;

static void seq_fsm();
static int safety_checks();
static void start_cool_down();
static int next_step();
static void regulator_prepage_configuration();
static void update_regulator();
static void try_interpolation();
static int calc_interpolaion();
static void condensate_prevention();
static void fans_run_mode();
static void fans_cool_mode();
static void raise_lift();
static void raise_lift_to_access();
static bool seq_in_tal();
static void control_heater();

void seq_init()
{
  sequence.sequence_time = -1;
  seq_state = SEQ_IDLE;
}

void seq_worker()
{
  if (seq_running())
  {
    seq_fsm();
  }
}

void seq_seconds_sync()
{
  if (seq_running())
  {
    if (sequence.step_time)
    {
      sequence.step_time--;
      try_interpolation();
    }

    // perform checks => abort on fail
    if (seq_state != SEQ_COOL_DOWN && !safety_checks())
    {
      seq_abort();
    }

    if (seq_state == SEQ_RUN || seq_state == SEQ_COOL_DOWN)
    {
      sequence.sequence_time += 1000;
    }
  }
}

static void seq_prepare()
{
  sequence.galden_temp =
      globalGalden.galdenTemp + 7; // use galdentemp of selected profile
  sequence.anti_condensation_temp =
      globalACT.acTemp; // use anticondensationtemperature of selected profile

  pump_set_auto_enabled(false);
  pump_enable();
  fan_set_auto_enabled(&lid_fan, true);
  fan_set_level(&lid_fan, 0);
  fan_set_auto_enabled(&quick_cooler, false);
  fan_set_level(&quick_cooler, 0);
  heater_set_temperature(sequence.galden_temp);
  heater_enable();
  raise_lift_to_access();
  fans_run_mode();

  // start regulating to the temperature of the first step
  regulator_prepage_configuration();
  regulator_enable();
}

static void seq_cleanup()
{
  sequence.sequence_time = -1;

  pump_set_auto_enabled(true);
  fan_set_auto_enabled(&quick_cooler, true);
  fan_set_auto_enabled(&lid_fan, true);
  heater_regulation_disable();
  heater_disable();
  vpx_log("seq_cleanup");
}

static void seq_fsm()
{
  switch (seq_state)
  {
  default:
  case SEQ_IDLE:
    break;

  case SEQ_SAFETY_CHECK:
    if (safety_checks())
    {
      seq_prepare();
      vpx_log("Profile in preheat");
      cS.currentState = 1;
      seq_state = SEQ_GENERATE_VAPOUR;
    }
    else
    {
      vpx_log("profile aborted - in preheat");
      cS.currentState = 0;
      seq_state = SEQ_IDLE;
    }
    break;

  case SEQ_GENERATE_VAPOUR:
    // wait until the heater was able to generate vapour
    if (heater_temperature_reached())
    {
      vpx_log("Profile preheat ended");
      seq_state = SEQ_PRE_HEAT;
    }
    break;

  case SEQ_PRE_HEAT:
    // wait until the regulator was able to reach the
    // target temperature of the first step
    if (regulator_temperature_reached())
    {
      update_regulator();
      vpx_log("Profile started");
      cS.currentState = 2;
      seq_state = SEQ_RUN;
      sequence.sequence_time = 0;
    }
    break;

  case SEQ_RUN:
    if (!sequence.step_time)
    {
      if (next_step())
      {
        sequence.index++;
        update_regulator();
        control_heater();
        vpx_log("x:%i", cS.x);
        vpx_log("y:%i", cS.y);
      }
      else
      {
        start_cool_down();
      }
    }
    break;

  case SEQ_COOL_DOWN:;
    // wait until the chamber has cooled down and the vapour condensated
    //  TODO: is this check enough? // This should be the Open LID Temperature
    //  -SK
    float lid_control_open_temperature = lid_control_lid_open_temperature();

    if (temperature_sensor_get_temperature(&temperature_sensor_galden) <
        lid_control_open_temperature)
    {
      vpx_log("Profile ended");

      raise_lift_to_access();
      seq_cleanup();
      seq_state = SEQ_CLEANUP;
    }
    break;
  case SEQ_CLEANUP:
    if (lift_axis.motor->direction == DIRECTION_NONE)
    {
      cS.currentState = 0;
      seq_state = SEQ_IDLE;
    }
    break;
  }
}

static int safety_checks()
{
  int rc = 1;

  // check axis calibration was performed
  if (!lid_axis.cal_done || !lift_axis.cal_done)
    rc = 0;

  // check if lid is closed
  if (!lid_control_lid_closed())
    rc = 0;

  return rc;
}

static int next_step()
{
  const int next = sequence.index + 1;

  // end sequence at max or at first zero duration
  return (next <= sequence.profile->totalDatapoints &&
                  sequence.profile->datapoints[next].time
              ? 1
              : 0);
}
static void try_interpolation()
{
  if (next_step())
  {
    int temp = calc_interpolaion();
    if (temp)
    {
      if ((sequence.profile->datapoints[sequence.index + 1].temperature >
           temperature_sensor_get_temperature(&temperature_sensor_galden)) &&
          sequence.profile->datapoints[sequence.index].temperature >
              temperature_sensor_get_temperature(&temperature_sensor_galden))
      {
        temp = (temperature_sensor_get_temperature(&temperature_sensor_galden) -
                2.0);
      }
      regulator_set_temp(temp);
    }
    cS.y = temp;
    vpx_log("x:%i", cS.x);
    vpx_log("y:%i", cS.y);
  }
  else
  {
    regulator_set_temp(0); // neu hinzugefügt, Versuch am ende nach 0 zu regeln
  }
}

static int calc_interpolaion()
{
  const int dx = sequence.profile->datapoints[sequence.index].time;
  const int xp = sequence.step_time;
  const int y0 = sequence.profile->datapoints[sequence.index].temperature;
  const int y1 = sequence.profile->datapoints[sequence.index + 1].temperature;

  // calculate temperature slope between the current and the next step
  const float g = ((float)(y1 - y0)) / dx;

  // calculate linear interpolated temperature
  return (((dx - xp) * g) + y0);
}

static void condensate_prevention()
{
  vpx_log("Anti condensation phase started");
  // prevent condensation by keeping the PCB "warm"
  regulator_set_temp(sequence.anti_condensation_temp);
  regulator_enable();
}

static void start_cool_down()
{
  cS.currentState = 3;
  vpx_log("Quick cool begins");
  seq_state = SEQ_COOL_DOWN;
  heater_regulation_disable();
  heater_disable();
  condensate_prevention();
  fans_cool_mode();
}

static void fans_run_mode() { fan_set_level(&quick_cooler, 0); }

static void fans_cool_mode() { fan_set_level(&quick_cooler, 100); }

static void raise_lift()
{
  // disable lift height/temperature regulation
  regulator_disable();

  // raise lift to top
  axis_move_to(&lift_axis, 0);
}

static void raise_lift_to_access()
{
  // disable lift height/temperature regulation
  regulator_disable();

  // raise lift to top
  axis_move_to(&lift_axis, -32000);
}

// set regulator to configuration of prepare phase
static void regulator_prepage_configuration()
{
  if (0 >= sequence.profile->totalDatapoints)
    return;

  regulator_set_temp(sequence.profile->datapoints[0].temperature);
}

static void update_regulator()
{
  // check sequence limits, max steps
  if (sequence.index > sequence.profile->totalDatapoints)
    return;

  // reload step countdown timer
  sequence.step_time = sequence.profile->datapoints[sequence.index].time;

  regulator_set_temp(sequence.profile->datapoints[sequence.index].temperature);
}

void seq_start(TemperatureProfile *profile)
{
  if (profile == NULL || profile->profileId < -1)
  {
    vpx_log("Seq Start - Profile null or ID not valid");
    return;
  }

  sequence.profile = profile;

  sequence.index = 0;
  seq_state = SEQ_SAFETY_CHECK;

  sequence.sequence_time = -1;
}

void seq_abort()
{
  if (!seq_can_abort())
  {
    return;
  }
  vpx_log("Reflow aborted.");
  heater_regulation_disable();
  heater_disable();
  raise_lift();
  seq_cleanup();
  seq_state = SEQ_CLEANUP;
}

// TODO: is this still required
int seq_progress()
{
  int total = 0, done = 0;

  for (int i = 0; i < sequence.profile->totalDatapoints; ++i)
  {
    int dur = sequence.profile->datapoints[i].time;

    if (!dur)
      break;

    if (i <= sequence.index)
      done += dur;

    total += dur;
  }

  done -= sequence.step_time;

  // calculate percent completed eg. 65%
  return (seq_state == SEQ_RUN ? (int)(((float)done / total) * 100) : 0);
}

const char *seq_get_state_str()
{
  switch (seq_state)
  {
  default:
    return " - - ";

  case SEQ_IDLE:
  case SEQ_SAFETY_CHECK:
    return "idle";

  case SEQ_GENERATE_VAPOUR:
    return "warm up";

  case SEQ_PRE_HEAT:
    return "pre heat";

  case SEQ_RUN:
    return "run";

  case SEQ_COOL_DOWN:
    return "cooling";
  }
}

uint16_t seq_get_flags() { return flags; }

extern void seq_get_limits(uint16_t *min_temp, uint16_t *max_temp,
                           uint16_t *total_dur)
{

  uint16_t tmax = 0, tmin = UINT16_MAX, total = 0;

  for (int i = 0; i < sequence.profile->totalDatapoints; ++i)
  {
    int dur = sequence.profile->datapoints[i].time;

    if (!dur)
      break;

    total += dur;

    if (sequence.profile->datapoints[i].temperature > tmax)
      tmax = sequence.profile->datapoints[i].temperature;

    if (sequence.profile->datapoints[i].temperature < tmin)
      tmin = sequence.profile->datapoints[i].temperature;
  }

  if (max_temp)
    *max_temp = tmax;

  if (min_temp)
    *min_temp = tmin;

  if (total_dur)
    *total_dur = total;
}

bool seq_running() { return seq_state != SEQ_IDLE; }

static bool seq_in_tal()
{
  return regulator_get_target_temperature() >=
         TAL_PHASE_THRESHOLD_TEMPERATURE(globalGalden.galdenTemp);
}

static void control_heater()
{
  // during tal phase the heater is always on and not regulated
  if (temperature_sensor_get_temperature(&temperature_sensor_pcb) <
          sequence.profile->datapoints[sequence.index].temperature &&
      (!heater_enabled()))
  {
    heater_regulation_disable();
    heater_enable();
  }

  sequence.index++;

  if ((temperature_sensor_get_temperature(&temperature_sensor_pcb) >
       sequence.profile->datapoints[sequence.index].temperature) &&
      ((sequence.profile->datapoints[sequence.index].temperature) == 0 ||
       (axis_position(&lift_axis) > -5000)))
  {
    heater_disable();
  }
  sequence.index--;
}

bool seq_can_abort()
{
  return seq_state == SEQ_SAFETY_CHECK || seq_state == SEQ_GENERATE_VAPOUR ||
         seq_state == SEQ_PRE_HEAT || seq_state == SEQ_RUN ||
         seq_state == SEQ_COOL_DOWN;
}
