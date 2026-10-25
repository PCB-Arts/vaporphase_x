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

#include "memory_card.h"
#include "main.h"
#include "sdmmc.h"

#include <string.h>

FATFS SDFatFs; /* File system object for SD card logical drive */
FIL g_logfile; /* File object */
FIL g_selftestLogfile;
char SDPath[4]; /* SD card logical drive path */

static int sd_open_logfile(void);
static int sd_close_logfile(void);

/* Initialize the card from scratch, so a card inserted after boot or swapped
 * after an eject works. main() doesn't call MX_SDMMC1_SD_Init() (disabled in
 * CubeMX); its USER CODE turns a failed init into sdmmc1_init_failed instead
 * of Error_Handler(), which would hang without a card. */
static int sd_card_init(void)
{
  if (hsd1.State != HAL_SD_STATE_RESET)
  {
    HAL_SD_DeInit(&hsd1);
  }

  MX_SDMMC1_SD_Init();
  return sdmmc1_init_failed ? -1 : 0;
}

int sd_mount(void)
{
  if (sd_card_init() != 0)
  {
    return -1;
  }

  /* Register the file system object to the FatFs module */
  FRESULT result = f_mount(&SDFatFs, (TCHAR const *)SDPath, 1);
  if (result != FR_OK)
  {
    /* FatFs Initialization Error */
    return -1;
  }

  return sd_open_logfile();
}

int sd_unmount(void)
{
  sd_close_logfile();
  f_mount(NULL, "", 0);
  HAL_SD_DeInit(&hsd1);
  return 0;
}

bool sd_card_removed(void)
{
  /* f_opendir() checks the card (disk_status(), CMD13) under the FatFs volume
   * lock, so the check can't interfere with a transfer of another task. A lock
   * timeout (another task is busy with the card) doesn't mean it was removed.
   */
  DIR dir;
  const FRESULT result = f_opendir(&dir, "/");
  if (result == FR_OK)
  {
    f_closedir(&dir);
  }
  return result == FR_NOT_READY || result == FR_DISK_ERR;
}

int sd_open_logfile()
{
  FRESULT result =
      f_open(&g_logfile, LOGFILE_FILENAME, FA_WRITE | FA_OPEN_APPEND);
  if (result != FR_OK)
    return -1;
  return 0;
}

int sd_close_logfile()
{
  if (f_close(&g_logfile) != FR_OK)
    return -1;
  return 0;
}

int sd_open_selftest_logfile()
{
  FRESULT result = f_open(&g_selftestLogfile, SELFTEST_LOGFILE_FILENAME,
                          FA_WRITE | FA_CREATE_ALWAYS);
  if (result != FR_OK)
    return -1;
  return 0;
}

int sd_close_selftest_logfile()
{
  if (f_close(&g_selftestLogfile) != FR_OK)
    return -1;
  return 0;
}

/* Write zero-terminated string to logfile and append newline. */
int sd_log_write_line(size_t n, const char *log_line)
{
  unsigned int bytes_written;
  if (f_write(&g_logfile, log_line, strnlen(log_line, n), &bytes_written) !=
      FR_OK)
  {
    return -1;
  }
  if (f_write(&g_logfile, "\n", 1, &bytes_written) != FR_OK)
  {
    return -1;
  }
  if (f_sync(&g_logfile) != FR_OK)
  {
    return -1;
  }
  return 0;
}
