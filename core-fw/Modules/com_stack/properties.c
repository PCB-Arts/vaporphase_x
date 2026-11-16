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

#include "main.h"

#include "log.h"
#include "packet_handler.h"
#include "version.h"
#include "vpx_protocol.h"

#include "control/state.h"

#include "control/sequence.h"
#include "modules/temp_sensors.h"

int app_prop_set(int prop_id, const union prop_value *val, size_t val_len)
{
  switch (prop_id)
  {
  case VPX_PROP_TEMP_LIQUID:
  case VPX_PROP_TEMP_COOLANT:
  case VPX_PROP_CORE_STATE:
    return -PRES_INVALID_REQUEST;

  case VPX_PROP_REFLOW_ACTIVE:
    if (val_len < sizeof(bool))
    {
      return -PRES_INVALID_REQUEST;
    }
    State_SetReflow(val->bl);
    return 0;

  case VPX_PROP_LID_OPEN:
    if (val_len < sizeof(bool))
    {
      return -PRES_INVALID_REQUEST;
    }
    State_SetLidOpen(val->bl);
    return 0;

  case VPX_PROP_LID_MODE:
    State_SetLidOpenMode((LidOpenMode)val->i8);
    return 0;

  case VPX_PROP_SELFTEST_ACTIVE:
    if (val->bl)
    {
      State_StartSelftest();
    }
    return 0;

  case VPX_PROP_SERVICE_POSITION:
    if (val_len < sizeof(bool))
    {
      return -PRES_INVALID_REQUEST;
    }
    if (val->bl)
    {
      State_SetServicePosition();
    }
    return 0;

  default:
    return -PRES_PROPERTY_NOT_FOUND;
  }
}

static bool first_read = true;
void on_first_read()
{
  vpx_log("Version v%d.%d", FW_VER_MAJOR, FW_VER_MINOR);
  vpx_log("Third party pin state %d",
          HAL_GPIO_ReadPin(THIRD_PARTY_GPIO_Port, THIRD_PARTY_Pin));
}

int app_prop_get(int prop_id, union prop_value *val)
{
  // workaround to do stuff on first communication with display
  if (first_read)
  {
    first_read = false;
    on_first_read();
  }

  Vpx_CoreStateTypeDef tempState;
  Vpx_CoreMonitorState1 tempMonitor1State;
  Vpx_CoreMonitorState2 tempMonitor2State;
  Vpx_CoreMonitorState3 tempMonitor3State;
  switch (prop_id)
  {
  case VPX_PROP_REFLOW_TIME:
    val->i64 = sequence.sequence_time;
    break;
  case VPX_PROP_TEMP_LIQUID:
    val->i64 =
        temperature_sensor_get_temperature(&temperature_sensor_galden) * 1000;
    break;
  case VPX_PROP_TEMP_COOLANT:
    val->i64 =
        temperature_sensor_get_temperature(&temperature_sensor_water) * 1000;
    break;
  case VPX_PROP_TEMP_PCB:
    val->i64 =
        temperature_sensor_get_temperature(&temperature_sensor_pcb) * 1000;
    break;
  case VPX_PROP_CORE_STATE:
    memset(val->data, 0, sizeof(val->data));

    tempState = State_getState();
    memcpy(val->data, &tempState, sizeof(tempState));
    break;
  case VPX_PROP_MONITOR_1:
    memset(val->data, 0, sizeof(val->data));

    tempMonitor1State = State_getMonitor1State();
    memcpy(val->data, &tempMonitor1State, sizeof(tempMonitor1State));
    break;
  case VPX_PROP_MONITOR_2:
    memset(val->data, 0, sizeof(val->data));

    tempMonitor2State = State_getMonitor2State();
    memcpy(val->data, &tempMonitor2State, sizeof(tempMonitor2State));
    break;
  case VPX_PROP_MONITOR_3:
    memset(val->data, 0, sizeof(val->data));

    tempMonitor3State = State_getMonitor3State();
    memcpy(val->data, &tempMonitor3State, sizeof(tempMonitor3State));
    break;
  default:
    return -PRES_PROPERTY_NOT_FOUND;
  }

  return 0;
}
