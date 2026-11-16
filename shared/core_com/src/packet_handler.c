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

#include "packet_handler.h"
#include <string.h>

#include <version.h>

const char *packet_errmsg;
volatile struct ll_pkt *volatile packet_res_buf = NULL;
volatile size_t packet_res_payload_len = 0;
char remote_errmsg_buf[256];
volatile const char *const remote_errmsg = remote_errmsg_buf;

static volatile uint8_t packet_counter = 0;

#ifndef MAX_WATCHED_PROPS
#define MAX_WATCHED_PROPS 64
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

struct prop_watch_desc
{
  int prop_id;
  union prop_value *out_buf;
  void (*callback)(int);
};

struct prop_watch_desc prop_watches[MAX_WATCHED_PROPS] = {0};

int packet_received(struct ll_pkt *pkt, size_t payload_len)
{
  int rc = 0;
  packet_errmsg = "Unknown Error";

  switch (pkt->header.packet_type)
  {

  case PTYPE_ERROR:
  case PTYPE_RESPONSE:
    if (packet_res_buf)
    {
      rc = PRES_COMMS_ERROR;
      packet_errmsg = "Response reception buffer overflow";
    }
    else
    {
      packet_res_payload_len = payload_len;
      packet_res_buf = pkt;
      return 0; /* suppress response */
    }
    break;

  case PTYPE_IDENTIFY:
  {
    /* Ignore payload, may contain host info in future protocol versions */
    struct pkt_res_identify *res = &(pkt->payload.res_identify);
    res->sys_time_ms = HAL_GetTick();
    memcpy(res->device_id, (uint8_t *)UID_BASE, 12);
    res->flash_size_kbytes = *(uint16_t *)FLASHSIZE_BASE;
    res->chip_rev =
        (DBGMCU->IDCODE & DBGMCU_IDCODE_REV_ID_Msk) >> DBGMCU_IDCODE_REV_ID_Pos;
    res->chip_id =
        (DBGMCU->IDCODE & DBGMCU_IDCODE_DEV_ID_Msk) >> DBGMCU_IDCODE_DEV_ID_Pos;

    res->fw_id = FW_ID;
    res->fw_ver_major = FW_VER_MAJOR;
    res->fw_ver_minor = FW_VER_MINOR;
    res->hw_id = HW_ID;
    res->fw_rev_major = HW_REV_MAJOR;
    res->fw_rev_minor = HW_REV_MINOR;
    rc = sizeof(pkt->payload.res_identify);
    break;
  }

  case PTYPE_PROP_SET:
  {
    packet_errmsg = "Error setting property";
    if (payload_len < sizeof(pkt->payload.req_prop_set.prop_id))
    {
      rc = -PRES_INVALID_REQUEST;
      break;
    }

    const size_t val_len =
        payload_len - sizeof(pkt->payload.req_prop_set.prop_id);
    union prop_value val = {0};
    memcpy(&val, &pkt->payload.req_prop_set.val,
           val_len < sizeof(val) ? val_len : sizeof(val));

    rc = app_prop_set(pkt->payload.req_prop_set.prop_id, &val, val_len);

    if (rc >= 0)
    {
      return 0;
    }
  }
    /* No break, kept as-is from the original: when app_prop_set() fails,
     * control drops into PTYPE_PROP_GET, which overwrites packet_errmsg
     * and answers with a property *read*. This looks unintended but is
     * protocol-visible and shared with core-fw, so it is not changed here. */

  case PTYPE_PROP_GET:
  {
    packet_errmsg = "Error getting property";
    if (payload_len < sizeof(pkt->payload.req_prop_get.prop_id))
    {
      rc = -PRES_INVALID_REQUEST;
      break;
    }

    union prop_value val = {0};
    rc = app_prop_get(pkt->payload.req_prop_get.prop_id, &val);

    if (rc >= 0)
    {
      memcpy(&pkt->payload.res_prop_get.val, &val, sizeof(val));
      // set response size
      rc = sizeof(pkt->payload.res_prop_get);
    }
    break;
  }
  case PTYPE_PROFILE_PACKAGE:
    if (payload_len < sizeof(ProfileTransmissionPacket))
    {
      rc = -PRES_INVALID_REQUEST;
      break;
    }

    ProfileTransmissionPacket *packet =
        (ProfileTransmissionPacket *)&pkt->payload;
    app_profile_frame_received(packet);

    return 0;

    break;

  case PTYPE_GALDEN_PACKAGE: // galden package that is being received by the
                             // core (written into global struct)
    if (payload_len < sizeof(GaldenTransmissionPacket))
    {
      rc = -PRES_INVALID_REQUEST;
      break;
    }
    GaldenTransmissionPacket *galdenpacket =
        (GaldenTransmissionPacket *)&pkt->payload;
    app_galden_frame_received(galdenpacket);

    return 0;
    break;

  case PTYPE_ACT_PACKAGE: // act package that is being received by the core
                          // (written into global struct)
    if (payload_len < sizeof(ACTTransmissionPacket))
    {
      rc = -PRES_INVALID_REQUEST;
      break;
    }
    ACTTransmissionPacket *actpackage = (ACTTransmissionPacket *)&pkt->payload;
    app_act_frame_received(actpackage);
    return 0;
    break;

  case PTYPE_NOP:
    return 0;

  case PTYPE_LOG_MSG:
    app_log_handler(pkt, payload_len);
    return 0;

  default:
    rc = app_packet_handler(pkt, payload_len);
    break;
  }

  if (rc < 0)
  {
    usart_respond_error(pkt, -rc, packet_errmsg);
  }
  else
  {
    usart_respond(pkt, rc);
  }
  return 0;
}

int usart_respond(struct ll_pkt *req, size_t payload_len)
{
  req->header.packet_type = PTYPE_RESPONSE;
  return Packet_SendNonBlocking(req, payload_len);
}

int usart_respond_error(struct ll_pkt *req, enum packet_error_codes err,
                        const char *msg)
{
  req->header.packet_type = PTYPE_ERROR;
  req->payload.res_error.rc = err;
  strcpy((char *)req->payload.res_error.msg, msg);
  return Packet_SendNonBlocking(req,
                                sizeof(struct pkt_res_error) + strlen(msg));
}

int usart_send_log_msg(uint8_t prio, const char *msg)
{
  struct __attribute__((__packed__))
  {
    struct ll_pkt_header ll;
    struct pkt_evt_log_msg evt;
    char msg[256];
  } evt = {.ll = {.packet_id = 0, .packet_type = PTYPE_LOG_MSG},
           .evt = {.prio = prio}};

  strncpy(evt.msg, msg, sizeof(evt.msg));
  return Packet_SendNonBlocking((struct ll_pkt *)&evt,
                                offsetof(typeof(evt.evt), msg) +
                                    strnlen(msg, sizeof(evt.msg)) +
                                    1 /* 0 limiter */);
}

int usart_sync_await_response(void *out, size_t out_len, uint8_t packet_id)
{
  // TODO: improve

  uint32_t start = HAL_GetTick();
  while (HAL_GetTick() - start < DEFAULT_TIMEOUT_MS)
  {
    Packet_Worker();
    if (!packet_res_buf)
    {
      Packet_Delay(1);
      continue;
    }

    if (packet_res_buf->header.packet_id != packet_id)
    {
      // discard old packets
      packet_res_buf = NULL;
      continue;
    }

    if (packet_res_buf->header.packet_type == PTYPE_ERROR)
    {
      // every error response starts with the return code, the message follows
      const size_t rc_len = sizeof(packet_res_buf->payload.res_error.rc);
      if (packet_res_payload_len < rc_len)
      {
        packet_res_buf = NULL;
        return -PRES_COMMS_ERROR;
      }
      const int32_t rc = packet_res_buf->payload.res_error.rc;
      const size_t msg_len =
          MIN(packet_res_payload_len - rc_len, sizeof(remote_errmsg_buf) - 1);
      memcpy(remote_errmsg_buf, (char *)packet_res_buf->payload.res_error.msg,
             msg_len);
      remote_errmsg_buf[msg_len] = '\0';

      packet_res_buf = NULL;
      return -(PRES_REMOTE_ERROR | rc);
    }
    else
    {
      if (packet_res_payload_len != out_len)
      {
        packet_res_buf = NULL;
        return -PRES_COMMS_ERROR;
      }

      if (out)
        memcpy(out, (char *)&packet_res_buf->payload, out_len);
      packet_res_buf = NULL;
      return 0;
    }
  }
  return -PRES_TIMEOUT;
}

int usart_sync_call_identify(struct pkt_res_identify *out)
{
  uint8_t packet_id = packet_counter++;

  struct ll_pkt identify_pkt = {
      .header = {.packet_id = packet_id, .packet_type = PTYPE_IDENTIFY}};

  int rc = Packet_SendNonBlocking(&identify_pkt, 0);
  if (rc)
    return rc;

  return usart_sync_await_response(out, sizeof(*out), packet_id);
}

int usart_async_prop_set(int prop_id, const union prop_value *in_buf)
{
  struct ll_pkt set_pkt = {
      .header = {.packet_id = 0, .packet_type = PTYPE_PROP_SET},
      .payload = {
          .req_prop_set = {.prop_id = prop_id, .val = {.u64 = in_buf->u64}}}};

  return Packet_SendNonBlocking(&set_pkt, sizeof(set_pkt.payload.req_prop_set));
}

int usart_sync_prop_get(int prop_id, union prop_value *out_buf)
{
  uint8_t packet_id = ++packet_counter;
  struct ll_pkt get_pkt = {
      .header = {.packet_id = packet_id, .packet_type = PTYPE_PROP_GET},
      .payload = {.req_prop_get = {.prop_id = prop_id}}};

  int rc = Packet_SendNonBlocking(&get_pkt, sizeof(struct pkt_req_prop_get));
  if (rc)
    return rc;

  rc = usart_sync_await_response((struct pkt_res_prop_get *)out_buf,
                                 sizeof(*out_buf), packet_id);
  return rc;
}

/* Weak symbols for default implementations */

int __attribute__((weak)) app_packet_handler(struct ll_pkt *pkt,
                                             size_t payload_len)
{
  switch (pkt->header.packet_type)
  {
  default:
    return -PRES_COMMAND_NOT_FOUND;
  }
}

int __attribute__((weak)) app_prop_set(int prop_id, const union prop_value *val,
                                       size_t val_len)
{
  switch (prop_id)
  {
  default:
    return -PRES_PROPERTY_NOT_FOUND;
  }
}

int __attribute__((weak)) app_prop_get(int prop_id, union prop_value *val)
{
  switch (prop_id)
  {
  default:
    return -PRES_PROPERTY_NOT_FOUND;
  }
}

int __attribute__((weak))
app_profile_frame_received(ProfileTransmissionPacket *packet)
{
  return -PRES_INVALID_REQUEST;
}

int __attribute__((weak))
app_galden_frame_received(GaldenTransmissionPacket *galdenpacket)
{
  return -PRES_INVALID_REQUEST;
}
int __attribute__((weak))
app_act_frame_received(ACTTransmissionPacket *actpacket)
{
  return -PRES_INVALID_REQUEST;
}

void __attribute__((weak)) app_log_handler(struct ll_pkt *pkt,
                                           size_t payload_len)
{
}
