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

#ifndef __LID_CONTROL_H
#define __LID_CONTROL_H

#include "vpx_types.h"
#include <stdbool.h>

void lid_control_init();
void lid_control_worker();

bool lid_control_lid_closed();

// check whether lid fullfils requirements to leave boot state
// does not have any defined meaning after system left boot state
bool lid_control_lid_boot_ready();

void lid_control_lid_open();
void lid_control_lid_close();

// move the lift to the top, then open the lid (if it is closed)
void lid_control_service_position();
LidState lid_control_lid_state();

void lid_control_set_lid_open_mode(LidOpenMode lidOpenMode);
LidOpenMode lid_control_get_lid_open_mode();
bool lid_control_lid_open_requirements_met();

float lid_control_lid_open_temperature();

#endif
