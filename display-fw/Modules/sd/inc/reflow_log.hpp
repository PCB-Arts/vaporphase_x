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

/*
 * Records every soldering session (the time the core reports reflow_active)
 * as reflows/<counter>_<profile>.csv on the SD card, one line per second,
 * while enabled.
 */

#ifndef MODULES_SD_INC_REFLOW_LOG_HPP_
#define MODULES_SD_INC_REFLOW_LOG_HPP_

// SD task: call every loop, also while unmounted (to track the session start)
void ReflowLog_Worker();

// SD task: call before unmounting, closes an open log file
void ReflowLog_OnUnmount();

// GUI task: switch recording on or off (setting "Reflow recording active",
// off after boot). Applies from the next session on, a running one continues.
void ReflowLog_SetEnabled(bool enabled);

// GUI task: call when a profile was selected. target points to the target
// temperature per second (index 0..duration_s) and must stay valid.
void ReflowLog_SetProfile(const char *profile_path, const float *target,
                          int duration_s);

#endif /* MODULES_SD_INC_REFLOW_LOG_HPP_ */
