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

#ifndef __VPX_PROTOCOL_H__
#define __VPX_PROTOCOL_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#include "vpx_types.h"

  enum Vpx_Properties
  {
    VPX_PROP_REFLOW_TIME = 1, /* Time of the current reflow operation */
    VPX_PROP_TEMP_LIQUID =
        2, /* Galden unit: 0.001 Degrees celsius, format: i32 */
    VPX_PROP_TEMP_COOLANT =
        3, /* Coolant unit: 0.001 Degrees celsius, format: i32 */
    VPX_PROP_TEMP_PCB = 7,
    VPX_PROP_CORE_STATE = 4,
    VPX_PROP_REFLOW_ACTIVE = 5,
    VPX_PROP_LID_OPEN = 6,
    VPX_PROP_LID_MODE = 8,
    VPX_PROP_MONITOR_1 = 9,
    VPX_PROP_MONITOR_2 = 10,
    VPX_PROP_MONITOR_3 = 11,
    VPX_PROP_SELFTEST_ACTIVE = 12,
    VPX_PROP_SERVICE_POSITION = 13,

    VPX_PROP_THIRD_PARTY_SOFTWARE = 0xFF,
  };

#ifdef __cplusplus
}
#endif
#endif /* __VPX_PROTOCOL_H__ */
