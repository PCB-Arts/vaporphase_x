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

#ifndef CORE_INC_LOG_H_
#define CORE_INC_LOG_H_

#ifdef __cplusplus
extern "C"
{
#endif

  // Creates the lock; call before the scheduler starts
  void vpx_log_init(void);
  void vpx_component_log(const char *message, const char *component);
  void vpx_log(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* CORE_INC_LOG_H_ */
