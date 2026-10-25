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
 * Based on STMicroelectronics example code from the STM32Cube
 * USB_Host/FWupgrade_Standalone example (command.c):
 *
 * Copyright (c) 2017 STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of other
 *    contributors to this software may be used to endorse or promote products
 *    derived from this software without specific written permission.
 * 4. This software, including modifications and/or derivative works of this
 *    software, must execute solely and exclusively on microcontroller or
 *    microprocessor devices manufactured by or for STMicroelectronics.
 * 5. Redistribution and use of this software other than as permitted under
 *    this license is void and will automatically terminate your rights under
 *    this license.
 *
 * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
 * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
 * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>

#include "bootloader.h"
#include "command.h"
#include "flash_if.h"
#include "memory_card.h"
#include "third_party.h"

#include "mbedtls/bignum.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/sha256.h"

struct firmware_file_header
{
  uint8_t sig_r[32];
  uint8_t sig_s[32];
  uint8_t sha256[32];
} __attribute__((packed));

#define CURVE_COORD_LEN 32
struct fw_upd_pk
{
  // just for information: typedef uint32_t mbedtls_mpi_uint
  mbedtls_mpi_uint x[CURVE_COORD_LEN / sizeof(mbedtls_mpi_uint)];
  mbedtls_mpi_uint y[CURVE_COORD_LEN / sizeof(mbedtls_mpi_uint)];
};

#define BYTES_TO_T_UINT_4(a, b, c, d)                                          \
  ((mbedtls_mpi_uint)(a) << 0) | ((mbedtls_mpi_uint)(b) << 8) |                \
      ((mbedtls_mpi_uint)(c) << 16) | ((mbedtls_mpi_uint)(d) << 24)

#define BYTES_TO_T_UINT_2(a, b) BYTES_TO_T_UINT_4(a, b, 0, 0)

#define BYTES_TO_T_UINT_8(a, b, c, d, e, f, g, h)                              \
  BYTES_TO_T_UINT_4(a, b, c, d), BYTES_TO_T_UINT_4(e, f, g, h)

static struct fw_upd_pk fw_upd_pk = {
#include "fw_update_sig_pk.c_inc"
};

#define BUFFERSIZE ((uint16_t)1 * 8192)

/* The SDMMC IDMA requires a word aligned buffer */
uint8_t RAMBuf[BUFFERSIZE] __attribute__((aligned(4))) = {0x00};

void COMMAND_JUMP(void)
{
  /* Software reset */
  NVIC_SystemReset();
}

/**
 * This commands loads the image from the provided file, validates its signature
 * and calls the callback while reading  the file. The file should contain a
 * header containing the signature, followed by the data to be flashed.
 * @param f pointer to file handle of image file
 * @param data_callback is called while reading the file to further process the
 * data from the image.
 */
uint32_t COMMAND_ProgramFlashMemory(FIL *f, COMMAND_dataCallback data_callback)
{
  __IO uint32_t read_size = 0x00, tmp_read_size = 0x00;
  uint32_t read_flag = TRUE;

  uint32_t offset = 0;

  mbedtls_mpi_uint one[] = {1};
  mbedtls_ecp_point pk_point = {
      .X = {.s = 1,
            .n = CURVE_COORD_LEN / sizeof(mbedtls_mpi_uint),
            .p = fw_upd_pk.x},

      .Y = {.s = 1,
            .n = CURVE_COORD_LEN / sizeof(mbedtls_mpi_uint),
            .p = fw_upd_pk.y},

      .Z = {.s = 1, .n = 1, .p = one}};

  mbedtls_mpi_uint r_uint[CURVE_COORD_LEN / sizeof(mbedtls_mpi_uint)];
  mbedtls_mpi sig_r = {
      .s = 1, .n = CURVE_COORD_LEN / sizeof(mbedtls_mpi_uint), .p = r_uint};

  mbedtls_mpi_uint s_uint[CURVE_COORD_LEN / sizeof(mbedtls_mpi_uint)];
  mbedtls_mpi sig_s = {
      .s = 1, .n = CURVE_COORD_LEN / sizeof(mbedtls_mpi_uint), .p = s_uint};

  struct firmware_file_header header;

  if (f_read(f, &header, sizeof(header), (uint_t *)&read_size) != FR_OK)
    return DOWNLOAD_FILE_FAIL;

  if (read_size != sizeof(header))
    return DOWNLOAD_FILE_FAIL;

  /* fix signature coordinate endianness */
  if (mbedtls_mpi_read_binary(&sig_r, header.sig_r, sizeof(header.sig_r)))
    return DOWNLOAD_FILE_FAIL;

  if (mbedtls_mpi_read_binary(&sig_s, header.sig_s, sizeof(header.sig_s)))
    return DOWNLOAD_FILE_FAIL;

  mbedtls_ecp_group grp;
  mbedtls_ecp_group_init(&grp);

  if (mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1))
  {
    mbedtls_ecp_group_free(&grp);
    return DOWNLOAD_FAIL;
  }

  if (!ThirdParty_Allowed() &&
      mbedtls_ecdsa_verify(&grp, header.sha256, sizeof(header.sha256),
                           &pk_point, &sig_r, &sig_s))
  {
    mbedtls_ecp_group_free(&grp);
    return DOWNLOAD_SIGNATURE_FAIL;
  }

  mbedtls_ecp_group_free(&grp);

  uint8_t calculated_sha256[32];
  mbedtls_sha256_context sha256_context;

  mbedtls_sha256_init(&sha256_context);
  mbedtls_sha256_starts(&sha256_context, 0); /* SHA-256, not 224 */

  /* While file still contain data */
  while (read_flag == TRUE)
  {

    if (f_read(f, RAMBuf, BUFFERSIZE, (uint_t *)&read_size) != FR_OK)
      return DOWNLOAD_FILE_FAIL;

    tmp_read_size = read_size;

    if (tmp_read_size < BUFFERSIZE)
      read_flag = FALSE;

    mbedtls_sha256_update(&sha256_context, RAMBuf, read_size);

    if (data_callback)
    {
      int rc = data_callback(RAMBuf, offset, tmp_read_size);

      if (rc != DOWNLOAD_OK)
      {
        return rc;
      }
    }

    offset += tmp_read_size;
  }

  mbedtls_sha256_finish(&sha256_context, calculated_sha256);
  mbedtls_sha256_free(&sha256_context);

  if (memcmp(calculated_sha256, header.sha256, sizeof(calculated_sha256)))
  {
    return DOWNLOAD_HASH_FAIL;
  }

  return DOWNLOAD_OK;
}
