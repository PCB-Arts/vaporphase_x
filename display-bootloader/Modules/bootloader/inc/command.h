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

#ifndef _COMMAND_H
#define _COMMAND_H

#include <stdbool.h>

#include "ff.h"
#include "flash_if.h"

enum download_error
{
  DOWNLOAD_OK = 0,
  DOWNLOAD_FAIL,
  DOWNLOAD_FILE_FAIL,
  DOWNLOAD_SIGNATURE_FAIL,
  DOWNLOAD_ERASE_FAIL,
  DOWNLOAD_WRITE_FAIL,
  DOWNLOAD_SIZE_FAIL,
  DOWNLOAD_HASH_FAIL,
};

/**
 * callback for consuming data of a file.
 * @param pointer to the data buffer
 * @offset offset of the first byte in the buffer to the beginning of the
 * underlying data stream.
 * @length number of bytes available in the buffer.
 */
typedef int (*COMMAND_dataCallback)(uint8_t *buffer, size_t offset,
                                    size_t length);

void COMMAND_JUMP(void);
uint32_t COMMAND_ProgramFlashMemory(FIL *f, COMMAND_dataCallback data_callback);

#endif /* _COMMAND_H */
