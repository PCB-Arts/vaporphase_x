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

#ifndef CORE_INC_SD_CARD_SD_HPP_
#define CORE_INC_SD_CARD_SD_HPP_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

  typedef struct
  {
    bool mount_failed : 1;
    bool removed_without_unmount : 1;
    bool logging_module_error : 1;
  } SD_ErrorTypeDef;

  extern volatile SD_ErrorTypeDef SD_error_state;

  void SD_Init();
  bool SD_Mounted();
  SD_ErrorTypeDef SD_ErrorState();
  void SD_SheduleUnmount();
  void SD_SheduleMount();
  void SD_Log(const char *s);

  // Workaround to let SD module log core logs into selftest.log
  void SD_SetSelftestActive(bool active);

  int GetFileLoadCounter();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <string>
#include <vector>

std::vector<std::string> GetAvailableCSVFiles();
#endif

#endif /* CORE_INC_SD_CARD_SD_HPP_ */
