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
 * Based on STMicroelectronics example code from the STM32Cube IAP example
 *(main.c):
 *
 * Copyright (c) 2016 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without
 *modification, are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *notice, this list of conditions and the following disclaimer in the
 *documentation and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *POSSIBILITY OF SUCH DAMAGE.
 */

/* Includes ------------------------------------------------------------------*/
#include "bootloader.h"
#include "bootloader_definitions.h"
#include "command.h"
#include "flash_if.h"
#include "lcd.h"
#include "memory_card.h"
#include "ospi_if.h"
#include "third_party.h"
#include "usart.h"

#include <packet_handler.h>

#include <tinyprintf.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define MAX_MENU_INDEX ((uint32_t)3)
#define MENU_ACTIVE ((uint32_t)1)
#define MENU_NOT_ACTIVE ((uint32_t)0)
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#define BGCOLOR LCD_COLOR_BLACK

FIL DownloadFile;
const char *filename_disp_fw = "VPX_DISP.BIN";
const char *filename_disp_fw_external = "VPX_DISP_ASSETS.BIN";
const char *filename_core_fw = "VPX_CORE.BIN";

static void Main_Menu(void);
static void await_user_button(void);

static int Main_lcd_line = 1;
static char Main_line[64] = {' '};

static int retry_counter = 3;

void Main_LCD_printf(const char *format, ...)
{
  va_list format_arguments;

  va_start(format_arguments, format);

  // keep single space at the beginning
  tfp_vsnprintf(&Main_line[1], sizeof(Main_line) - 1, format, format_arguments);

  va_end(format_arguments);

  LCD_DisplayStringAtLine(Main_lcd_line++, Main_line);
}

void Main_LCD_Clear()
{
  LCD_Clear(BGCOLOR);
  Main_lcd_line = 1;
}

/* BOOT_SEL (PA10): high boots the user firmware, low stays in the bootloader.
 * It also serves as the "USER button" (active low) in the update dialog. */
#define BOOTLOADER_CHECK_Pin BOOT_SEL_Pin
#define BOOTLOADER_CHECK_GPIO_Port BOOT_SEL_GPIO_Port

/* Runs before MX_GPIO_Init(), which configures the pin the same way later */
void Main_BootloaderSwitchInit()
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = BOOTLOADER_CHECK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BOOTLOADER_CHECK_GPIO_Port, &GPIO_InitStruct);
}

void Bootloader_CheckBootSelect(void)
{
  Main_BootloaderSwitchInit();

  /* Jump into bootloader mode if Key push-button on board is pressed, or if the
   * application interrupt table looks invalid. */
  if (HAL_GPIO_ReadPin(BOOTLOADER_CHECK_GPIO_Port, BOOTLOADER_CHECK_Pin) ==
      GPIO_PIN_SET)
  {
    jump_to_user_fw();
  }
}

void Bootloader_Run(void)
{
  Packet_Init(&huart1);

  /* Init the user interface */
  LCD_Init();

  /* Clear the LCD */
  LCD_Clear(BGCOLOR);
  /* Set the LCD Back Color */
  LCD_SetBackColor(BGCOLOR);
  /* Set the LCD Text Color */
  LCD_SetTextColor(LCD_COLOR_WHITE);
  /* Set the font size */
  LCD_SetFont(&Font20);
  Main_LCD_printf("VaporPhaseX Bootloader");

  /* Initialize CARD */
  if (Card_Init() == FR_OK)
  {
    /* Run main menu */
    Main_Menu();
  }
  else
  {
    Card_Unlink();
    /*  Error message */
    Main_LCD_printf("Cannot open micro SD card");
    Main_LCD_printf("### FATAL ERROR ###");
    Main_LCD_printf("Please power-cycle or reset device.");
  }

  /* Infinite loop */
  while (1)
  {
  }
}

/* Called from Error_Handler() */
void Bootloader_ErrorHandler(void)
{
  if (LCD_IsInitialized())
  {
    Main_LCD_printf("### FATAL ERROR ###");
    Main_LCD_printf("Please power-cycle or reset device.");
  }
}

void Main_print_usart_error(int rc)
{
  if (rc < -PRES_REMOTE_ERROR)
  {
    Main_LCD_printf("Remote error: %d", (-rc) & ~PRES_REMOTE_ERROR);
  }
  else
  {
    Main_LCD_printf("Communication error: %d", -rc);
  }
}

int usart_sync_cmd_flash_clear()
{
  struct ll_pkt pkt = {
      .header = {.packet_id = 0, .packet_type = PTYPE_BL_FLASH_CLEAR},
      .payload = {.req_bl_flash_clear = {.magic = 0xDEAD5732}}};

  int rc = Packet_SendNonBlocking(&pkt, sizeof(struct pkt_req_bl_flash_clear));
  if (rc)
    return rc;

  return usart_sync_await_response(NULL, 0, 0);
}

int usart_sync_cmd_reboot()
{
  struct ll_pkt pkt = {.header = {.packet_id = 0, .packet_type = PTYPE_REBOOT}};

  return Packet_SendNonBlocking(&pkt, 0);
}

/**
 * sends one paket of data to flash to the core controlller.
 * @param buffer pointer to data to read from
 * @param available_data number of bytes available for reading from buffer
 * @param addr of core controller to write to
 * @returns amount of bytes send in written command (<= available_data) or error
 * code
 */
int usart_sync_cmd_flash_write(uint8_t *buffer, size_t available_data,
                               size_t addr)
{
  struct ll_pkt pkt = {
      .header = {.packet_id = 0, .packet_type = PTYPE_BL_FLASH_WRITE},
      .payload = {.req_bl_flash_write = {
                      .magic = 0x00131200,
                      .address = addr,
                  }}};

  size_t max_paket_data_size = sizeof(pkt.payload.req_bl_flash_write.data);

  size_t bytes_to_send = available_data < max_paket_data_size
                             ? available_data
                             : max_paket_data_size;

  memcpy(pkt.payload.req_bl_flash_write.data, buffer, bytes_to_send);

  if (bytes_to_send == 0)
    return -PRES_EOF;

  int rc = Packet_SendNonBlocking(
      &pkt, offsetof(struct pkt_req_bl_flash_write, data) + bytes_to_send);
  if (rc)
    return rc;

  rc = usart_sync_await_response(NULL, 0, 0);
  if (rc < 0)
    return rc;

  return bytes_to_send;
}

static void await_user_button()
{
  /* Wait for button to be released */
  while (HAL_GPIO_ReadPin(BOOTLOADER_CHECK_GPIO_Port, BOOTLOADER_CHECK_Pin) ==
         GPIO_PIN_RESET)
  {
    HAL_Delay(1);
  }

  /* Wait for button to be pressed */
  while (HAL_GPIO_ReadPin(BOOTLOADER_CHECK_GPIO_Port, BOOTLOADER_CHECK_Pin) ==
         GPIO_PIN_SET)
  {
    HAL_Delay(1);
  }
}

int Main_FlashUserCoreFirmwareCallback(uint8_t *data, size_t offset,
                                       size_t length)
{
  size_t bytes_written = 0;

  while (length - bytes_written > 0)
  {
    int result = usart_sync_cmd_flash_write(
        &data[bytes_written], length - bytes_written, offset + bytes_written);

    if (result < 0)
    {
      Main_print_usart_error(result);
      return DOWNLOAD_WRITE_FAIL;
    }

    bytes_written += result;
  }

  return DOWNLOAD_OK;
}

/**
 * Print why COMMAND_ProgramFlashMemory failed.
 * @param image name of the image shown in front of the message
 * @param rc return code of COMMAND_ProgramFlashMemory
 */
static void Main_print_download_error(const char *image, uint32_t rc)
{
  switch (rc)
  {
  case DOWNLOAD_FILE_FAIL:
    Main_LCD_printf("%s: SD card read error", image);
    break;
  case DOWNLOAD_SIGNATURE_FAIL:
    Main_LCD_printf("%s: invalid signature", image);
    break;
  case DOWNLOAD_HASH_FAIL:
    Main_LCD_printf("%s: checksum mismatch", image);
    break;
  case DOWNLOAD_ERASE_FAIL:
    Main_LCD_printf("%s: flash erase error", image);
    break;
  case DOWNLOAD_WRITE_FAIL:
    Main_LCD_printf("%s: flash write error", image);
    break;
  case DOWNLOAD_SIZE_FAIL:
    Main_LCD_printf("%s: image too large", image);
    break;
  default:
    Main_LCD_printf("%s: internal error", image);
    break;
  }
}

static int Main_FlashUserCoreFirmwareFile()
{
  int rc;

  /* Do a dry-run first to make sure the signature is valid */
  uint32_t download_rc = COMMAND_ProgramFlashMemory(&DownloadFile, NULL);
  if (download_rc != DOWNLOAD_OK)
  {
    Main_print_download_error("Core image check", download_rc);
    return 1;
  }

  if (f_lseek(&DownloadFile, 0) != FR_OK)
  {
    return 1;
  }

  Main_LCD_printf("Erasing core controller...");
  if ((rc = usart_sync_cmd_flash_clear()))
  {
    Main_print_usart_error(rc);
    return 1;
  }

  Main_LCD_printf("Erase complete. Programming core controller...");

  /* Now do the actual programming. Verify the signature again to protect
   * against TOCTOU. */
  download_rc = COMMAND_ProgramFlashMemory(&DownloadFile,
                                           Main_FlashUserCoreFirmwareCallback);
  if (download_rc != DOWNLOAD_OK)
  {
    Main_print_download_error("Core programming", download_rc);
    return 1;
  }

  return 0;
}

int Main_FlashUserCoreFirmware()
{
  if (f_open(&DownloadFile, filename_core_fw, FA_READ) != FR_OK)
  {
    Main_LCD_printf("Cannot open core file: %s", filename_core_fw);
    return 1;
  }

  const int rc = Main_FlashUserCoreFirmwareFile();

  f_close(&DownloadFile);

  return rc;
}

int Main_FlashUserDisplayFirmwareCallback(uint8_t *data, size_t offset,
                                          size_t length)
{
  /* Program flash memory. offset is a multiple of the 8 KiB read buffer, so
   * quad-word aligned; only the last chunk can have a partial quad-word. */
  if (FLASH_If_Write(APPLICATION_ADDRESS + offset, data, length) != 0x00)
  {
    return DOWNLOAD_WRITE_FAIL;
  }

  return DOWNLOAD_OK;
}

static int Main_FlashUserDisplayFirmwareInternalFile()
{
  /* Do a dry-run first to make sure the signature is valid */
  uint32_t download_rc = COMMAND_ProgramFlashMemory(&DownloadFile, NULL);
  if (download_rc != DOWNLOAD_OK)
  {
    Main_print_download_error("Display image check", download_rc);
    return 1;
  }

  if (f_lseek(&DownloadFile, 0) != FR_OK)
  {
    return 1;
  }

  Main_LCD_printf("Erasing display controller...");

  if (FLASH_If_EraseSectors(APPLICATION_ADDRESS) != 0)
  {
    Main_LCD_printf("Flash erase error");
    return 1;
  }

  Main_LCD_printf("Erase complete. Programming display controller...");
  /* Now do the actual programming. Verify the signature again to protect
   * against TOCTOU. */
  download_rc = COMMAND_ProgramFlashMemory(
      &DownloadFile, Main_FlashUserDisplayFirmwareCallback);
  if (download_rc != DOWNLOAD_OK)
  {
    Main_print_download_error("Display programming", download_rc);
    return 1;
  }

  return 0;
}

int Main_FlashUserDisplayFirmwareInternal()
{
  if (f_open(&DownloadFile, filename_disp_fw, FA_READ) != FR_OK)
  {
    Main_LCD_printf("Cannot open display file: %s", filename_disp_fw);
    return 1;
  }

  FLASH_If_FlashUnlock();

  const int rc = Main_FlashUserDisplayFirmwareInternalFile();

  FLASH_IF_FlashLock();
  f_close(&DownloadFile);

  return rc;
}

int Main_FlashUserDisplayExternalCallback(uint8_t *data, size_t offset,
                                          size_t length)
{
  return OSPI_If_Write(offset, data, length) == 0 ? DOWNLOAD_OK
                                                  : DOWNLOAD_WRITE_FAIL;
}

static int Main_FlashUserDisplayFirmwareExternalFile()
{
  /* Do a dry-run first to make sure the signature is valid */
  uint32_t download_rc = COMMAND_ProgramFlashMemory(&DownloadFile, NULL);
  if (download_rc != DOWNLOAD_OK)
  {
    Main_print_download_error("Asset image check", download_rc);
    return 1;
  }

  Main_LCD_printf("Signature checked successfully");

  if (f_lseek(&DownloadFile, 0) != FR_OK)
  {
    Main_LCD_printf("f_lseek problem");
    return 1;
  }

  Main_LCD_printf("Erasing external display flash...");

  /* Only the sectors the image needs: a chip erase of the 64 MiB part takes
   * minutes. The file size includes the signature header, which only makes
   * the erased range slightly larger. */
  if (OSPI_If_Erase(0, f_size(&DownloadFile)) != 0)
  {
    Main_LCD_printf("Flash erase error");
    return 1;
  }
  Main_LCD_printf("Erase complete. Programming external display flash...");

  /* Now do the actual programming. Verify the signature again to protect
   * against TOCTOU. */
  download_rc = COMMAND_ProgramFlashMemory(
      &DownloadFile, Main_FlashUserDisplayExternalCallback);
  if (download_rc != DOWNLOAD_OK)
  {
    Main_print_download_error("Asset programming", download_rc);
    return 1;
  }

  return 0;
}

int Main_FlashUserDisplayFirmwareExternal()
{
  Main_LCD_printf("External display flash: ");

  if (OSPI_If_Init() != 0)
  {
    Main_LCD_printf("Error initializing OSPI.");
    return 1;
  }

  if (f_open(&DownloadFile, filename_disp_fw_external, FA_READ) != FR_OK)
  {
    Main_LCD_printf("Cannot open file: %s", filename_disp_fw_external);
    return 1;
  }

  Main_LCD_printf("Opened Downloadfile successfully");

  const int rc = Main_FlashUserDisplayFirmwareExternalFile();

  f_close(&DownloadFile);

  return rc;
}

/**
 * @brief  Display the Main Menu on Display
 * @param  None
 * @retval None
 */
void Main_Menu(void)
{
  struct pkt_res_identify core_id_response;
  bool flash_only_display = false;

  int rc = usart_sync_call_identify(&core_id_response);
  if (rc < 0)
  {
    Main_LCD_printf("Cannot reach core controller");
    Main_print_usart_error(rc);
    Main_LCD_printf("Press USER button to ignore");
    await_user_button();
    Main_LCD_printf("continuing");
    flash_only_display = true;
  }
  else
  {
    Main_LCD_printf("Core connected: fw %d/hw %d", core_id_response.fw_id,
                    core_id_response.hw_id);
    if (core_id_response.fw_id != FW_ID_VPX_CORE_BOOTLOADER)
    {
      Main_LCD_printf("Error: Core is not running the bootloader!");
      Main_LCD_printf(
          "Please set the core into bootloader state and try again.");
      return;
    }

    rc = ThirdParty_Init();

    if (rc != 0)
    {
      Main_LCD_printf("Error reading third party allowed state.");
      return;
    }

    if (ThirdParty_Allowed())
    {
      Main_LCD_printf("Signature verification is disabled!");
    }
  }

  fileinfo_t finfno = {0};
  if (f_stat(filename_disp_fw, &finfno) != FR_OK)
  {
    Main_LCD_printf("Display firmware image \"%s\" not found",
                    filename_disp_fw);
    return;
  }

  if (finfno.fsize > USER_FLASH_SIZE)
  {
    Main_LCD_printf("Display firmware image too large");
    return;
  }

  if (f_stat(filename_disp_fw_external, &finfno) != FR_OK)
  {
    Main_LCD_printf("Display firmware asset image \"%s\"",
                    filename_disp_fw_external);
    Main_LCD_printf("not found");
    return;
  }

  if (!flash_only_display)
  {
    fileinfo_t finfno_core = {0};
    if (f_stat(filename_core_fw, &finfno_core) != FR_OK)
    {
      Main_LCD_printf("Core firmware image \"%s\" not found", filename_core_fw);
      return;
    }
  }

  Main_LCD_printf("Firmware update files found");
  Main_LCD_printf("Press user button to update firmware");
  if (flash_only_display)
  {
    Main_LCD_printf("### CAUTION! Communication failure! ###");
    Main_LCD_printf("### This will ONLY FLASH THE DISPLAY CONTROLLER ###");
  }
  await_user_button();

  Main_LCD_Clear();
  Main_LCD_printf("VaporPhaseX Bootloader");

  if (!flash_only_display)
  {
    rc = Main_FlashUserCoreFirmware();
    // When flashing didn't work
    if (rc != 0)
    {
      Main_LCD_Clear();
      for (int i = 0; i <= retry_counter; i++)
      {
        Main_LCD_printf("Problem during core flashing. Retry %d of %d", i + 1,
                        retry_counter + 1);
        rc = Main_FlashUserCoreFirmware();
        if (rc == 0)
        {
          // leave the retry
          break;
        }
        if (retry_counter == i)
        {
          // return after retring
          Main_LCD_printf("Problem during flashing, please try again");
          return;
        }
      }
    }
  }

  rc = Main_FlashUserDisplayFirmwareInternal();
  // When flashing didn't work
  if (rc != 0)
  {
    Main_LCD_Clear();
    for (int i = 0; i <= retry_counter; i++)
    {
      Main_LCD_printf("Problem during display flashing. Retry %d of %d", i + 1,
                      retry_counter + 1);
      rc = Main_FlashUserDisplayFirmwareInternal();
      if (rc == 0)
      {
        // leave the retry
        break;
      }
      if (retry_counter == i)
      {
        // return after retring
        Main_LCD_printf("Problem during flashing, please try again");
        return;
      }
    }
  }

  rc = Main_FlashUserDisplayFirmwareExternal();
  // When flashing didn't work
  if (rc != 0)
  {
    Main_LCD_Clear();
    for (int i = 0; i <= retry_counter; i++)
    {
      Main_LCD_printf("Problem during ext_display flashing. Retry %d of %d",
                      i + 1, retry_counter + 1);
      rc = Main_FlashUserDisplayFirmwareExternal();
      if (rc == 0)
      {
        // leave the retry
        break;
      }
      if (retry_counter == i)
      {
        // return after retring
        Main_LCD_printf("Problem during flashing, please try again");
        return;
      }
    }
  }

  Main_LCD_printf("Programming complete");
  Main_LCD_printf("Please power-cycle device to reboot into new firmware");
}
