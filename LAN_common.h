/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * common.c
 * Constants required for both internally and externally
 * Copyright (C) 2004-2006 Simon Newton
 */

#ifndef LAN_COMMON_H_
#define LAN_COMMON_H_

/*
 * libartnet error codes
 */
enum {
  ARTNET_EOK = 0,
  ARTNET_ENET = -1, // network error
  ARTNET_EMEM = -2, // memory error
  ARTNET_EARG = -3, // argument error
  ARTNET_ESTATE = -4, // state error
  ARTNET_EACTION = -5, // invalid action
};


// various constants used everywhere
extern int ARTNET_PORT;
extern int ARTNET_STRING_SIZE;
extern char ARTNET_STRING[];
extern uint8_t ARTNET_VERSION;
extern uint8_t OEM_HI;
extern uint8_t OEM_LO;
extern char ESTA_HI;
extern char ESTA_LO;
extern uint8_t TTM_BEHAVIOUR_MASK;
extern uint8_t TTM_REPLY_MASK;
extern uint8_t PORT_STATUS_LPT_MODE;
extern uint8_t PORT_STATUS_SHORT;
extern uint8_t PORT_STATUS_ERROR;
extern uint8_t PORT_STATUS_DISABLED_MASK;
extern uint8_t PORT_STATUS_MERGE;
extern uint8_t PORT_STATUS_DMX_TEXT;
extern uint8_t PORT_STATUS_DMX_SIP;
extern uint8_t PORT_STATUS_DMX_TEST;
extern uint8_t PORT_STATUS_ACT_MASK;
extern uint8_t PORT_DISABLE_MASK;
extern uint8_t MIN_PACKET_SIZE;

/*
 * The maximum ports per node built into the ArtNet protocol.
 * This is always 4. Don't change it unless you really know what your doing
 */
enum { ARTNET_MAX_PORTS = 4 };

/**
 * The length of the short name field. Always 18
 */
enum { ARTNET_SHORT_NAME_LENGTH = 18 };

/**
 * The length of the long name field. Always 64
 */
enum { ARTNET_LONG_NAME_LENGTH = 64 };

/**
 * The length of the report field. Always 64
 */
enum { ARTNET_REPORT_LENGTH = 64 };

/**
 * The length of the DMX field. Always 512
 */
enum { ARTNET_DMX_LENGTH = 512 };

/*
 * Number of bytes in a RDM UID
 */
enum { ARTNET_RDM_UID_WIDTH = 6 };

/*
 * Length of the hardware address
 */
enum { ARTNET_MAC_SIZE = 6 };

/*
 * Length of the ESTA field
 */
enum { ARTNET_ESTA_SIZE = 2 };

/*
 * Length of the IP field
 */
enum { ARTNET_IP_SIZE = 4 };

#define LAN_DMX_FOOTPRINT   (10)


// the node report codes
typedef enum {
  ARTNET_RCDEBUG,
  ARTNET_RCPOWEROK,
  ARTNET_RCPOWERFAIL,
  ARTNET_RCSOCKETWR1,
  ARTNET_RCPARSEFAIL,
  ARTNET_RCUDPFAIL,
  ARTNET_RCSHNAMEOK,
  ARTNET_RCLONAMEOK,
  ARTNET_RCDMXERROR,
  ARTNET_RCDMXUDPFULL,
  ARTNET_RCDMXRXFULL,
  ARTNET_RCSWITCHERR,
  ARTNET_RCCONFIGERR,
  ARTNET_RCDMXSHORT,
  ARTNET_RCFIRMWAREFAIL,
  ARTNET_RCUSERFAIL
} artnet_node_report_code;

// these define the types of node that can exist
// note it's different from artnet_node_type
typedef enum {
  STNODE = 0x00,
  STSERVER = 0x01,
  STMEDIA = 0x02,
  STROUTE = 0x03,
  STBACKUP = 0x04,
  STCONFIG = 0x05
} artnet_node_style_code;


// End packet enums
//-----------------------------------------------------------------------------


// the status of the node
typedef enum {
  ARTNET_OFF,
  ARTNET_STANDBY,
  ARTNET_ON
} node_status_t;

/**
 * The main node structure
 */
typedef struct artnet_node_s{
  uint8_t id[8];
  uint8_t mac_addr[ARTNET_MAC_SIZE];
  in_addr reply_addr;
  in_addr ip_addr;
  in_addr bcast_addr;
  in_addr gateway_addr;
  in_addr subnet_mask;
  char short_name[ARTNET_SHORT_NAME_LENGTH];
  char long_name[ARTNET_LONG_NAME_LENGTH];
  char report[ARTNET_REPORT_LENGTH];
  node_status_t status;
  struct ports_s {
    uint8_t  types[ARTNET_MAX_PORTS];    // type of port
    uint8_t output[ARTNET_MAX_PORTS]; // output ports
    uint8_t input[ARTNET_MAX_PORTS]; // input ports
  } ports;
  uint8_t subnet_hi;
  uint8_t subnet_lo;
  uint8_t oem_hi;
  uint8_t oem_lo;
  uint8_t esta_hi;
  uint8_t esta_lo;
  uint8_t fmw_hi;
  uint8_t fmw_lo;
  uint8_t ubea;
  uint8_t swin[ARTNET_MAX_PORTS];
  uint8_t swout[ARTNET_MAX_PORTS];
  uint8_t swvideo;
  uint8_t swmacro;
  uint8_t swremote;
  artnet_node_report_code report_code;
  void (*dmx_callback)(uint16_t portid, uint8_t *dmx);
  uint8_t dmx_start;
  uint8_t dmx_footprint;
} artnet_node_t;

#endif
