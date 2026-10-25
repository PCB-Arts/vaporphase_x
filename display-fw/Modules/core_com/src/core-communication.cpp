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

#include "core-communication.hpp"
#include <limits>
#include <math.h>
#include <sd.hpp>

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "conf.h"
#include "log.h"
#include "task.h"
#include "version.h"
#include "vpx_protocol.h"

osThreadId_t CoreCom_TaskHandle;
const osThreadAttr_t CoreCom_Task_attributes = []
{
  osThreadAttr_t attr;
  attr.name = "CoreComTask";
  attr.stack_size = 2048;
  attr.priority = (osPriority_t)osPriorityHigh;
  return attr;
}();

static int32_t core_operation_time = -1;
static int64_t core_pcb_temperature = 0;
static int64_t core_liquid_temperature = 0;
static int64_t core_coolant_temperature = 0;

static Vpx_CoreStateTypeDef core_state;

static bool monitor_state_new = false;
static Vpx_CoreMonitorStateComplete monitor_state;

void app_log_handler(struct ll_pkt *pkt, size_t payload_len)
{
  struct pkt_evt_log_msg *log_pkt;
  log_pkt = (struct pkt_evt_log_msg *)&pkt->payload;

  vpx_component_log((char *)log_pkt->msg, "CORE");
}
static CoreCom_ErrorState com_error_state;

static void CoreCom_UpdateCoreTime()
{
  union prop_value time_prop;
  if (usart_sync_prop_get(VPX_PROP_REFLOW_TIME, &time_prop))
  {
    return;
  }

  taskENTER_CRITICAL();
  core_operation_time = time_prop.i64;
  taskEXIT_CRITICAL();
}

static void CoreCom_UpdatePcbTemperature()
{
  union prop_value pcb_temp_prop;
  if (usart_sync_prop_get(VPX_PROP_TEMP_PCB, &pcb_temp_prop))
  {
    return;
  }

  taskENTER_CRITICAL();
  core_pcb_temperature = pcb_temp_prop.i64;
  taskEXIT_CRITICAL();
}

static void CoreCom_UpdateLiquidTemperature()
{
  union prop_value galden_temp_prop;
  if (usart_sync_prop_get(VPX_PROP_TEMP_LIQUID, &galden_temp_prop))
  {
    return;
  }

  taskENTER_CRITICAL();
  core_liquid_temperature = galden_temp_prop.i64;
  taskEXIT_CRITICAL();
}

static void CoreCom_UpdateCoolantTemperature()
{
  union prop_value coolant_temperature_prop;
  if (usart_sync_prop_get(VPX_PROP_TEMP_COOLANT, &coolant_temperature_prop))
  {
    return;
  }

  taskENTER_CRITICAL();
  core_coolant_temperature = coolant_temperature_prop.i64;
  taskEXIT_CRITICAL();
}

static void CoreCom_UpdateMonitorState()
{
  union prop_value monitor1_state_prop;
  union prop_value monitor2_state_prop;
  union prop_value monitor3_state_prop;
  if (usart_sync_prop_get(VPX_PROP_MONITOR_1, &monitor1_state_prop))
  {
    return;
  }

  if (usart_sync_prop_get(VPX_PROP_MONITOR_2, &monitor2_state_prop))
  {
    return;
  }

  if (usart_sync_prop_get(VPX_PROP_MONITOR_3, &monitor3_state_prop))
  {
    return;
  }

  Vpx_CoreMonitorStateComplete state;

  Vpx_CoreMonitorState1 state1;
  memcpy(&state1, monitor1_state_prop.data, sizeof(state1));

  Vpx_CoreMonitorState2 state2;
  memcpy(&state2, monitor2_state_prop.data, sizeof(state2));

  Vpx_CoreMonitorState3 state3;
  memcpy(&state3, monitor3_state_prop.data, sizeof(state3));

  state.heater1_temp = state1.heater1_temp;
  state.heater2_temp = state1.heater2_temp;
  state.galden_temp = state1.galden_temp;
  state.pcb_temp = state1.pcb_temp;
  state.coolant_temp = state2.coolant_temp;

  state.radiator_fan_speed = state2.radiator_fan_speed;

  state.lid_cooling_fan_on = state2.lid_cooling_fan_on;
  state.coolant_pump_on = state2.coolant_pump_on;
  state.quick_cool_fan_on = state2.quick_cool_fan_on;
  state.heater_on = state2.heater_on;
  state.heater_otp_active = state2.heater_otp_active;
  state.coolant_otp_active = state2.coolant_otp_active;
  state.lid_lock_active = state2.lid_lock_active;

  state.lid_position = state3.lid_position;
  state.lift_position = state3.lift_position;

  taskENTER_CRITICAL();
  monitor_state = state;
  monitor_state_new = true;
  taskEXIT_CRITICAL();
}

static void CoreCom_UpdateState()
{
  union prop_value state_prop;
  Vpx_CoreStateTypeDef temp_state;

  if (usart_sync_prop_get(VPX_PROP_CORE_STATE, &state_prop))
  {
    return;
  }

  memcpy(&temp_state, state_prop.data, sizeof(temp_state));
  taskENTER_CRITICAL();
  core_state = temp_state;
  taskEXIT_CRITICAL();

  SD_SetSelftestActive(core_state.selftest_active);
}

#if MOCK_COMMUNICATION
static void CoreCom_CommunicationMock()
{
  while (1)
  {

    while (!core_state.reflow_active)
    {
      osDelay(100);
    }

    core_operation_time = 0;

    int startTicks = HAL_GetTick();

    vpx_log("Started reflow with mocked communication.");

    while (core_operation_time <= 605000 && core_state.reflow_active)
    {
      core_operation_time = (HAL_GetTick() - startTicks) * 100;

#if MOCK_ERRORS
      if (core_operation_time > 300000 && core_operation_time < 500000)
      {
        com_error_state.communication_error = true;
        osDelay(10);
        continue;
      }
#endif

      core_liquid_temperature =
          sin(core_operation_time * M_2_PI / (60000.0)) * 100000 + 120000;

      core_coolant_temperature =
          -sin(core_operation_time * M_2_PI / (60000.0)) * 10000 + 50000;

      osDelay(10);
    }
  }
}
#endif

static void CoreCom_Task(void *argument)
{
#if MOCK_COMMUNICATION
  vpx_log("Communication is mocked!!!");

  core_state.can_open_lid = true;
  core_state.can_start_reflow = true;

  CoreCom_CommunicationMock();
#else
  int lastFastTick = HAL_GetTick();

  while (1)
  {
    CoreCom_UpdateCoreTime();
    CoreCom_UpdatePcbTemperature();
    CoreCom_UpdateLiquidTemperature();
    CoreCom_UpdateCoolantTemperature();
    CoreCom_UpdateState();

    int currentTick = HAL_GetTick();

    int intermediateTime =
        core_state.reflow_active ? 2500 : 20 * 1000; // 10Hz if active, 0,1 else

    if (currentTick - lastFastTick > intermediateTime)
    {
      lastFastTick = currentTick;
      CoreCom_UpdateMonitorState();
    }

    osDelay(10); // Perhaps not here if monitor state is updated?
  }
#endif
}

void CoreCom_Init()
{
  CoreCom_TaskHandle =
      osThreadNew(CoreCom_Task, NULL, &CoreCom_Task_attributes);
}

int32_t CoreCom_CoreOperationTime()
{
  taskENTER_CRITICAL();
  const int32_t value = core_operation_time;
  taskEXIT_CRITICAL();
  return value;
}

uint64_t CoreCom_PcbTemperature()
{
  taskENTER_CRITICAL();
  const int64_t value = core_pcb_temperature;
  taskEXIT_CRITICAL();
  return value;
}

uint64_t CoreCom_LiquidTemperature()
{
  taskENTER_CRITICAL();
  const int64_t value = core_liquid_temperature;
  taskEXIT_CRITICAL();
  return value;
}

uint64_t CoreCom_CoolantTemperature()
{
  taskENTER_CRITICAL();
  const int64_t value = core_coolant_temperature;
  taskEXIT_CRITICAL();
  return value;
}

Vpx_CoreStateTypeDef CoreCom_CoreState()
{
  taskENTER_CRITICAL();
  const Vpx_CoreStateTypeDef value = core_state;
  taskEXIT_CRITICAL();
  return value;
}

void CoreCom_ReadMonitor()
{
  taskENTER_CRITICAL();
  monitor_state_new = false;
  taskEXIT_CRITICAL();
}

bool CoreCom_HasNewMonitor()
{
  taskENTER_CRITICAL();
  const bool value = monitor_state_new;
  taskEXIT_CRITICAL();
  return value;
}

Vpx_CoreMonitorStateComplete CoreCom_GetMonitorState()
{
  taskENTER_CRITICAL();
  const Vpx_CoreMonitorStateComplete value = monitor_state;
  taskEXIT_CRITICAL();
  return value;
}

static void CoreCom_SetBool(int prop_id, bool value)
{
  union prop_value prop;
  prop.bl = value;
  usart_async_prop_set(prop_id, &prop);
}

static void CoreCom_SetInt8(int prop_id, int8_t value)
{
  union prop_value prop;
  prop.i8 = value;
  usart_async_prop_set(prop_id, &prop);
}

void CoreCom_StartReflow()
{
  if (core_state.error || !core_state.can_start_reflow)
  {
    return;
  }

#if MOCK_COMMUNICATION
  core_state.can_start_reflow = false;
  core_state.can_open_lid = false;
  core_state.reflow_active = true;
#else
  CoreCom_SetBool(VPX_PROP_REFLOW_ACTIVE, true);
#endif
}

void CoreCom_StopReflow()
{
#if MOCK_COMMUNICATION
  core_state.reflow_active = false;
  if (!core_state.error)
  {
    core_state.can_start_reflow = true;
    core_state.can_open_lid = true;
  }
#else
  CoreCom_SetBool(VPX_PROP_REFLOW_ACTIVE, false);
#endif
}

void CoreCom_OpenLid()
{
  if (core_state.error || !core_state.can_open_lid)
  {
    return;
  }
#if MOCK_COMMUNICATION
  core_state.can_open_lid = false;
  core_state.lid_open = true;
  core_state.can_start_reflow = false;
#else
  CoreCom_SetBool(VPX_PROP_LID_OPEN, true);
#endif
}

void CoreCom_ServicePosition()
{
  if (core_state.error)
  {
    return;
  }
#if MOCK_COMMUNICATION
  core_state.can_open_lid = false;
  core_state.lid_state = LID_STATE_OPEN;
  core_state.can_start_reflow = false;
#else
  CoreCom_SetBool(VPX_PROP_SERVICE_POSITION, true);
#endif
}

void CoreCom_CloseLid()
{
#if MOCK_COMMUNICATION
#if MOCK_ERRORS
  core_state.lid_blocked_error = true;
#else
  core_state.lid_open = false;
  if (!core_state.error)
  {
    core_state.can_start_reflow = true;
    core_state.can_open_lid = true;
  }
#endif
#else
  CoreCom_SetBool(VPX_PROP_LID_OPEN, false);
#endif
}

void CoreCom_SetLidMode(LidOpenMode mode)
{
#if MOCK_COMMUNICATION
  core_state.lid_open_mode = mode;
#else
  CoreCom_SetInt8(VPX_PROP_LID_MODE, (int8_t)mode);
#endif
}

CoreCom_ErrorState CoreCom_GetErrorState() { return com_error_state; }

#ifdef __cplusplus

static int usart_send_profile_frame(ProfileTransmissionPacket *in_buf)
{
  struct __attribute__((__packed__)) ProfilePackage
  {
    struct ll_pkt_header ll;
    ProfileTransmissionPacket packet;
  };
  ProfilePackage pkg;
  ll_pkt_header header;
  header.packet_id = 0;
  header.packet_type = PTYPE_PROFILE_PACKAGE;
  pkg.ll = header;
  pkg.packet = *in_buf;

  return Packet_SendNonBlocking(reinterpret_cast<ll_pkt *>(&pkg),
                                sizeof(ProfileTransmissionPacket));
}

static int usart_send_galden_temp(
    GaldenTransmissionPacket *in_buf) // usart core communication galden
{
  struct __attribute__((__packed__)) GaldenTemp
  {
    struct ll_pkt_header ll;
    GaldenTransmissionPacket galdenpacket;
  };
  GaldenTemp gt;
  ll_pkt_header header;
  header.packet_id = 0;
  header.packet_type = PTYPE_GALDEN_PACKAGE;
  gt.ll = header;
  gt.galdenpacket = *in_buf;

  return Packet_SendNonBlocking(reinterpret_cast<ll_pkt *>(&gt),
                                sizeof(GaldenTransmissionPacket));
}

static int usart_send_ACT(ACTTransmissionPacket *in_buf) // usart core
                                                         // communication atc
{
  struct __attribute__((__packed__)) ACTemp
  {
    struct ll_pkt_header ll;
    ACTTransmissionPacket ACTpacket;
  };
  ACTemp ac;
  ll_pkt_header header;
  header.packet_id = 0;
  header.packet_type = PTYPE_ACT_PACKAGE;
  ac.ll = header;
  ac.ACTpacket = *in_buf;
  return Packet_SendNonBlocking(reinterpret_cast<ll_pkt *>(&ac),
                                sizeof(ACTTransmissionPacket));
}

/*
 * Function to send Galden Temperature to Core via usart.
 * Information from galdenpacket
 */
bool CoreCom_SendGalden(SolderProfile &profile)
{
  static int8_t profileId = 0;
  if (profileId < 0)
  {
    profileId = 0;
  }
  static_assert(std::numeric_limits<int>::max() > 1000);
  if (profile.boilingtemperature.size() > 20)
  {
    vpx_log("Error Galden-Temperature size");
    return false;
  }
  GaldenTransmissionPacket galdenpacket;
  galdenpacket.galdenTemp =
      static_cast<int16_t>(profile.boilingtemperature[0].boilingTemperature);

  if (usart_send_galden_temp(&galdenpacket))
  {
    return false;
  }
  return true;
}
bool CoreCom_SendACT(SolderProfile &profile)
{
  static int8_t profileId = 0;
  if (profileId < 0)
  {
    profileId = 0;
  }
  static_assert(std::numeric_limits<int>::max() > 1000);
  if (profile.actemperature.size() > 20)
  {
    vpx_log("Error ACT-Temperature size");
    return false;
  }
  ACTTransmissionPacket ACTPacket;
  ACTPacket.ACT = static_cast<int16_t>(profile.actemperature[0].ACT); // 16?

  if (usart_send_ACT(&ACTPacket))
  {
    return false;
  }

  return true;
}

bool CoreCom_StartProfile(SolderProfile &profile)
{
  static int8_t profileId = 0;
  if (profileId < 0)
  {
    profileId = 0;
  }
  static_assert(std::numeric_limits<int>::max() > 1000);
  if (profile.timeTemperatureData.size() > 600)
  {
    vpx_log("Too many datapoints, cannot start profile");
    return false;
  }
  int numberOfDatapoints = static_cast<int>(profile.timeTemperatureData.size());

  for (int i = 0; i < numberOfDatapoints; i += 60)
  {
    ProfileTransmissionPacket packet;
    int j = 0;
    for (; i + j < numberOfDatapoints; j++)
    {
      // Assumes temperature is in degree with no decimal places and time is
      // limited by 2^15
      // TODO: Should this be checked?
      if (i + j == numberOfDatapoints - 1)
      {
        packet.times[j] = 0;
      }
      else
      {
        packet.times[j] =
            static_cast<int16_t>(profile.timeTemperatureData[i + j + 1].time -
                                 profile.timeTemperatureData[i + j].time);
      }
      packet.temperatures[j] =
          static_cast<int16_t>(profile.timeTemperatureData[i + j].temperature);
    }
    packet.entriesOffset = i;
    packet.numberEntries = j;
    packet.totalEntries = numberOfDatapoints;
    packet.profileId = profileId;
    if (usart_send_profile_frame(&packet))
    {
      return false;
    }
  }

  profileId += 1;
  // TODO: check correct transmission

  CoreCom_StartReflow();
  return true;
}
#endif

void CoreCom_StartSelftest()
{
  if (core_state.error || !core_state.can_start_selftest)
  {
    return;
  }
  vpx_log("Starting Self Test");
#if !MOCK_COMMUNICATION
  SD_SetSelftestActive(true);

  taskENTER_CRITICAL();
  core_state.can_start_reflow = false;
  core_state.can_start_selftest = false;
  core_state.can_open_lid = false;
  taskEXIT_CRITICAL();
  CoreCom_SetBool(VPX_PROP_SELFTEST_ACTIVE, true);

#endif
  vpx_log("FW Ver: v%d.%d", FW_VER_MAJOR, FW_VER_MINOR);
}
