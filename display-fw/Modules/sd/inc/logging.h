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

#ifndef __LOGGING_H
#define __LOGGING_H

// SD module internal, called from the SD task in sd.cpp only

#ifdef __cplusplus
extern "C"
{
#endif

  void SD_Log_Init(void);
  void SD_Log_Worker(void);
  // closes Selftest.log if open, call before unmounting
  void SD_Log_OnUnmount(void);

#ifdef __cplusplus
}
#endif

#endif /* __LOGGING_H */
