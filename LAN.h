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
 * artnet.h
 * Interface to libartnet
 * Copyright (C) 2004-2007 Simon Newton
 */

#ifndef LAN_H_
#define LAN_H_

#include "mbed.h"

#include <stdint.h>
// order is important here for osx
#include <sys/types.h>

#include "UDPSocket.h"

#include "LAN_packets.h"

extern UDPSocket* LAN_sock;
extern artnet_packet_t* LAN_packet;

/**
 * An enum for setting the behaviour of a port.
 * Ports can either input data (DMX -> ArtNet) or
 * output (ArtNet -> DMX) data.
 */
typedef enum {
  ARTNET_ENABLE_INPUT = 0x40,  /**< Enables the input for this port */
  ARTNET_ENABLE_OUTPUT = 0x80  /**< Enables the output for this port */
} artnet_port_settings_t;


typedef enum {
  ARTNET_PC_NONE = 0x00,
  ARTNET_PC_CANCEL = 0x01,
  ARTNET_PC_LED_NORMAL = 0x02,
  ARTNET_PC_LED_MUTE = 0x03,
  ARTNET_PC_LED_LOCATE = 0x04,
  ARTNET_PC_RESET = 0x05,
  ARTNET_PC_MERGE_LTP_O = 0x10,
  ARTNET_PC_MERGE_LTP_1 = 0x11,
  ARTNET_PC_MERGE_LTP_2 = 0x12,
  ARTNET_PC_MERGE_LTP_3 = 0x13,
  ARTNET_PC_MERGE_HTP_0 = 0x50,
  ARTNET_PC_MERGE_HTP_1 = 0x51,
  ARTNET_PC_MERGE_HTP_2 = 0x52,
  ARTNET_PC_MERGE_HTP_3 = 0x53,
  ARTNET_PC_CLR_0 = 0x93,
  ARTNET_PC_CLR_1 = 0x93,
  ARTNET_PC_CLR_2 = 0x93,
  ARTNET_PC_CLR_3 = 0x93,
} artnet_port_command_t;


/*
 * An enum for the type of data transmitted on a port.
 * As far as I know, only DMX-512 is supported
 */
typedef enum  {
  ARTNET_PORT_DMX = 0x00,    /**< Data is DMX-512 */
  ARTNET_PORT_MIDI = 0x01,  /**< Data is MIDI */
  ARTNET_PORT_AVAB = 0x02,  /**< Data is Avab */
  ARTNET_PORT_CMX = 0x03,    /**< Data is Colortran CMX */
  ARTNET_PORT_ADB = 0x04,    /**< Data is ABD 62.5 */
  ARTNET_PORT_ARTNET = 0x05  /**< Data is ArtNet */
} artnet_port_data_code;


/**
 * An enum for refering to a particular input or output port.
 */
typedef enum {
  ARTNET_INPUT_PORT = 1,    /**< The input port */
  ARTNET_OUTPUT_PORT,      /**< The output port */
} artnet_port_dir_t;


/*
 * Enum describing the type of node
 */
typedef enum {
  ARTNET_SRV,      /**< An ArtNet server (transmitts DMX data) */
  ARTNET_NODE,    /**< An ArtNet node   (dmx reciever) */
  ARTNET_MSRV,    /**< A Media Server */
  ARTNET_ROUTE,    /**< No Effect currently */
  ARTNET_BACKUP,    /**< No Effect currently */
  ARTNET_RAW      /**< Raw Node - used for diagnostics */
} artnet_node_type;


/*
 * Enum for the talk-to-me value
 * These values can be &'ed togeather, so for example to set private replies
 * and auto replying use :
 *   (ARTNET_TTM_PRIVATE & ARTNET_TTM_AUTO)
 */
typedef enum {
  ARTNET_TTM_DEFAULT = 0xFF,  /**< default, ArtPollReplies are broadcast, and nodes won't send a ArtPollReply when conditions change */
  ARTNET_TTM_PRIVATE = 0xFE,  /**< ArtPollReplies aren't broadcast */
  ARTNET_TTM_AUTO = 0xFD    /**< ArtPollReplies are send when node conditions chang */
} artnet_ttm_value_t;

// functions

// LAN_transmit.cpp
extern int LAN_send_poll_reply(artnet_node_t *node, int response);
extern void LAN_fill_poll_reply(artnet_node_t *node, artnet_reply_t *poll_reply);

// LAN_network.cpp
extern int LAN_recv(artnet_node_t *node, artnet_packet_t *p);
extern int LAN_send(artnet_node_t *node, artnet_packet_t *packet);

// LAN.cpp
extern int LAN_init(artnet_node_t *node);
extern void LAN_set_port(artnet_node_t *node, uint8_t subnet_hi, uint8_t subnet_lo);
extern void LAN_set_dmx(artnet_node_t *node, uint8_t dstart, uint8_t dfootprint);
extern void LAN_set_dmx_callback(artnet_node_t *node, void (*cb)(uint16_t port, uint8_t *dmx));
extern void LAN_set_network(
        artnet_node_t *node, in_addr ip,
        in_addr bcast, in_addr gateway, in_addr netmask, uint8_t *mac_addr);
extern void LAN_announce(artnet_node_t *node);
extern void LAN_set_name(artnet_node_t *node, char *short_name, char *long_name);
extern void LAN_handle_poll(artnet_node_t *node, artnet_packet_t *p);
extern void LAN_handle_dmx(artnet_node_t *node, artnet_packet_t *p);

// LAN_receive.cpp
extern int LAN_read(artnet_node_t *node, artnet_packet_t *p);
extern int LAN_handle(artnet_node_t *node, artnet_packet_t *p);
extern int16_t LAN_get_type(artnet_packet_t *p);

// void artnet_error(arnet_node *node, const char *fmt, ...);

#endif
