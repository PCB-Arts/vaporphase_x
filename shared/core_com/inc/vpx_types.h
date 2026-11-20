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

#ifndef __VPX_TYPES_H
#define __VPX_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum __LidState
{
  LID_STATE_UNDEFINED = 0,
  LID_STATE_INITIALISING = 1,
  LID_STATE_CLOSED = 2,
  LID_STATE_OPEN = 3,
  LID_STATE_OPENING = 4,
  LID_STATE_CLOSING = 5,
  LID_STATE_ERROR = 6
} LidState;

typedef enum __CurrentState
{
  CS_Idle = 0,
  CS_Preheat = 1,
  CS_Soldering = 2,
  CS_AntiCondensation = 3,
  CS_Calibration = 4,
} CurrentState;

enum _LidOpenMode
{
  VPX_STANDARD = 0,
  VPX_ECO = 1,
  VPX_FAST = 2
};

typedef enum _LidOpenMode LidOpenMode;

typedef struct __attribute__((__packed__)) __Vpx_CoreStateTypeDef
{
  bool error : 1; // any error occured (core in error state)
  bool reflow_active : 1;
  bool can_start_reflow : 1;
  bool can_stop_reflow : 1;
  bool can_update_profile : 1;
  LidState lid_state : 3;
  bool can_open_lid : 1;
  LidOpenMode lid_open_mode : 2;
  bool error_lid_ocp : 1;
  bool error_heater_otp : 1;
  bool error_coolant_otp : 1;
  bool can_start_selftest : 1;
  bool selftest_active : 1;
  CurrentState current_state : 5;
  // from starting the service position until the lift is moved down again
  bool service_position_active : 1;
} Vpx_CoreStateTypeDef;

typedef struct __attribute__((__packed__)) __Vpx_CoreMonitorStateComplete
{
  int16_t heater1_temp : 16;
  int16_t heater2_temp : 16;
  int16_t galden_temp : 16;
  int16_t pcb_temp : 16;
  int16_t coolant_temp : 16;

  int8_t radiator_fan_speed : 8;

  bool lid_cooling_fan_on : 1;
  bool coolant_pump_on : 1;
  bool quick_cool_fan_on : 1;
  bool heater_on : 1;
  bool heater_otp_active : 1;
  bool coolant_otp_active : 1;
  bool lid_lock_active : 1;

  int32_t lid_position : 32;
  int32_t lift_position : 32;
} Vpx_CoreMonitorStateComplete;

typedef struct __attribute__((__packed__)) __Vpx_CoreMonitorState1
{
  int16_t heater1_temp : 16;
  int16_t heater2_temp : 16;
  int16_t galden_temp : 16;
  int16_t pcb_temp : 16;
} Vpx_CoreMonitorState1;

typedef struct __attribute__((__packed__)) __Vpx_CoreMonitorState2
{
  int16_t coolant_temp : 16;

  int8_t radiator_fan_speed : 8;

  bool lid_cooling_fan_on : 1;
  bool coolant_pump_on : 1;
  bool quick_cool_fan_on : 1;
  bool heater_on : 1;
  bool heater_otp_active : 1;
  bool coolant_otp_active : 1;
  bool lid_lock_active : 1;
} Vpx_CoreMonitorState2;

typedef struct __attribute__((__packed__)) __Vpx_CoreMonitorState3
{
  int32_t lid_position : 32;
  int32_t lift_position : 32;
} Vpx_CoreMonitorState3;

#endif
