/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h" /* Declarations of disk functions */
#include "ff.h"     /* Obtains integer types */
#include "sdmmc.h"
#include "stm32u5xx_hal.h"
#include <stdint.h>

/* Definitions of physical drive number for each drive */
#define DEV_RAM 0 /* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC 1 /* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB 2 /* Example: Map USB MSD to physical drive 2 */

#define SD_TIMEOUT (1000)

/**
 * Define which HAL interface mode should be used for \c ReadBlocks and \c
 * WriteBlocks Only one mode should be enabled at a time (otherwise compilation
 * error).
 */
#define USE_HAL_SD_POLL 0 /* Polling (blocking) interface */
#define USE_HAL_SD_POLL_CS                                                     \
  0 /* Polling (blocking) interface guarded with critical section */
#define USE_HAL_SD_DMA 1 /* DMA (non-blocking) interface*/

extern SD_HandleTypeDef hsd1;
static volatile DSTATUS Stat = STA_NOINIT;

/*-----------------------------------------------------------------------*/
/* HAL interface abstraction                                             */
/*-----------------------------------------------------------------------*/
#if USE_HAL_SD_POLL == 1
static HAL_StatusTypeDef ReadBlocks(uint8_t *pData, uint32_t BlockAdd,
                                    uint32_t NumberOfBlocks)
{
  HAL_StatusTypeDef status;
  status =
      HAL_SD_ReadBlocks(&hsd1, pData, BlockAdd, NumberOfBlocks, SD_TIMEOUT);
  return status;
}
static HAL_StatusTypeDef WriteBlocks(const uint8_t *pData, uint32_t BlockAdd,
                                     uint32_t NumberOfBlocks)
{
  HAL_StatusTypeDef status;
  status =
      HAL_SD_WriteBlocks(&hsd1, pData, BlockAdd, NumberOfBlocks, SD_TIMEOUT);
  return status;
}
#endif
/*-----------------------------------------------------------------------*/
#if USE_HAL_SD_POLL_CS == 1
/* Only this variant needs the RTOS, the bootloader builds without one */
#include "FreeRTOS.h"
#include "task.h"

static HAL_StatusTypeDef ReadBlocks(uint8_t *pData, uint32_t BlockAdd,
                                    uint32_t NumberOfBlocks)
{
  HAL_StatusTypeDef status;
  taskENTER_CRITICAL();
  status =
      HAL_SD_ReadBlocks(&hsd1, pData, BlockAdd, NumberOfBlocks, SD_TIMEOUT);
  taskEXIT_CRITICAL();
  return status;
}
static HAL_StatusTypeDef WriteBlocks(const uint8_t *pData, uint32_t BlockAdd,
                                     uint32_t NumberOfBlocks)
{
  HAL_StatusTypeDef status;
  taskENTER_CRITICAL();
  status =
      HAL_SD_WriteBlocks(&hsd1, pData, BlockAdd, NumberOfBlocks, SD_TIMEOUT);
  taskEXIT_CRITICAL();
  return status;
}
#endif
/*-----------------------------------------------------------------------*/
#if USE_HAL_SD_DMA == 1
typedef enum
{
  SD_DMA_OPERATION_BUSY = 0,
  SD_DMA_OPERATION_DONE,
  SD_DMA_OPERATION_ERROR,
} sd_dma_operation_t;
static volatile sd_dma_operation_t sd_dma_operation;

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  sd_dma_operation = SD_DMA_OPERATION_DONE;
}
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  sd_dma_operation = SD_DMA_OPERATION_DONE;
}
void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
{
  sd_dma_operation = SD_DMA_OPERATION_ERROR;
}
void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
  sd_dma_operation = SD_DMA_OPERATION_ERROR;
}
static HAL_StatusTypeDef WaitDmaBusy(uint32_t timeout_ms)
{
  /* Wait until operation is not busy anymore or until timeout */
  uint32_t tickstart = HAL_GetTick();
  while ((HAL_GetTick() - tickstart) < timeout_ms)
  {
    if (sd_dma_operation != SD_DMA_OPERATION_BUSY)
    {
      return (sd_dma_operation == SD_DMA_OPERATION_DONE) ? HAL_OK : HAL_ERROR;
    }
  }
  return HAL_TIMEOUT;
}
static HAL_StatusTypeDef ReadBlocks(uint8_t *pData, uint32_t BlockAdd,
                                    uint32_t NumberOfBlocks)
{
  HAL_StatusTypeDef status;
  sd_dma_operation = SD_DMA_OPERATION_BUSY;
  status = HAL_SD_ReadBlocks_DMA(&hsd1, pData, BlockAdd, NumberOfBlocks);
  if (status != HAL_OK)
  {
    return status;
  }
  status = WaitDmaBusy(SD_TIMEOUT);
  if (status == HAL_TIMEOUT)
  {
    /* Stop the transfer, else hsd1 stays busy and the next one fails */
    HAL_SD_Abort(&hsd1);
  }
  return status;
}
static HAL_StatusTypeDef WriteBlocks(const uint8_t *pData, uint32_t BlockAdd,
                                     uint32_t NumberOfBlocks)
{
  HAL_StatusTypeDef status;
  sd_dma_operation = SD_DMA_OPERATION_BUSY;
  status = HAL_SD_WriteBlocks_DMA(&hsd1, pData, BlockAdd, NumberOfBlocks);
  if (status != HAL_OK)
  {
    return status;
  }
  status = WaitDmaBusy(SD_TIMEOUT);
  if (status == HAL_TIMEOUT)
  {
    /* Stop the transfer, else hsd1 stays busy and the next one fails */
    HAL_SD_Abort(&hsd1);
  }
  return status;
}
#endif
/*-----------------------------------------------------------------------*/

/* Wait until the card is ready for the next transfer or until timeout */
static DRESULT WaitCardTransferState(void)
{
  uint32_t tickstart = HAL_GetTick();
  while (HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER)
  {
    if ((HAL_GetTick() - tickstart) >= SD_TIMEOUT)
    {
      return RES_ERROR;
    }
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
  Stat = STA_NOINIT;

  if (HAL_SD_GetCardState(&hsd1) == HAL_SD_CARD_TRANSFER)
  {
    Stat &= ~STA_NOINIT;
  }

  return Stat;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS
disk_initialize(BYTE lun /* Physical drive nmuber to identify the drive */
)
{
  Stat = STA_NOINIT;

#if (ENABLE_SD_INIT == 1)
  sdmmc_sd_init();
  Stat = SD_check_status(lun);
#else
  Stat = disk_status(lun);
#endif
  return Stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv,  /* Physical drive nmuber to identify the drive */
                  BYTE *buff, /* Data buffer to store read data */
                  LBA_t sector, /* Start sector in LBA */
                  UINT count    /* Number of sectors to read */
)
{
  if (pdrv != 0)
    return RES_PARERR;

  DRESULT res = RES_ERROR;

  if (ReadBlocks((uint8_t *)buff, sector, count) == HAL_OK)
  {
    res = WaitCardTransferState();
  }
  return res;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, /* Physical drive nmuber to identify the drive */
                   const BYTE *buff, /* Data to be written */
                   LBA_t sector,     /* Start sector in LBA */
                   UINT count        /* Number of sectors to write */
)
{

  if (pdrv != 0)
    return RES_PARERR;

  DRESULT res = RES_ERROR;

  if (WriteBlocks((const uint8_t *)buff, sector, count) == HAL_OK)
  {
    res = WaitCardTransferState();
  }
  return res;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0..) */
                   BYTE cmd,  /* Control code */
                   void *buff /* Buffer to send/receive control data */
)
{
  DRESULT res = RES_ERROR;
  HAL_SD_CardInfoTypeDef CardInfo;

  if (Stat & STA_NOINIT)
    return RES_NOTRDY;

  switch (cmd)
  {
  /* Make sure that no pending write process */
  case CTRL_SYNC:
    res = RES_OK;
    break;

  /* Get number of sectors on the disk (DWORD) */
  case GET_SECTOR_COUNT:
    HAL_SD_GetCardInfo(&hsd1, &CardInfo);
    *(DWORD *)buff = CardInfo.LogBlockNbr;
    res = RES_OK;
    break;

  /* Get R/W sector size (WORD) */
  case GET_SECTOR_SIZE:
    HAL_SD_GetCardInfo(&hsd1, &CardInfo);
    *(WORD *)buff = CardInfo.LogBlockSize;
    res = RES_OK;
    break;

  /* Get erase block size in unit of sector (DWORD) */
  case GET_BLOCK_SIZE:
    HAL_SD_GetCardInfo(&hsd1, &CardInfo);
    *(DWORD *)buff = CardInfo.LogBlockSize / BLOCKSIZE;
    res = RES_OK;
    break;

  default:
    res = RES_PARERR;
  }

  return res;
}
