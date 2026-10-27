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

#ifndef MODULES_LIFT_H_
#define MODULES_LIFT_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

//=================================
// included dependencies
#include "modules/axis.h"

  //=================================
  // declarations

#define LIFT_MOTOR_MAX_SPEED 100

#define LIFT_LID_OPEN_POSITION -32000
// position of the top limit switch, set by the calibration
#define LIFT_TOP_POSITION 0

  extern struct axis_t lift_axis;

  typedef struct
  {
    int liftflag;
  } calibpos;

  void lift_worker(void);
  void lift_emergency_stop(void);

  // check whether lid fullfils requirements to leave boot state
  // does not have any defined meaning after system left boot state
  bool lift_boot_ready();

  // call to request lift to go to state to prepare opening of lid
  void lift_lid_open_state();
  bool lift_lid_open_state_reached();

  void lift_calibrate();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MODULES_LIFT_H_ */
