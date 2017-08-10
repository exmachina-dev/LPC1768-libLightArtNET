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
 * artnet.c
 * Implementes the external functions for libartnet
 * Copyright (C) 2004-2007 Simon Newton
 */
#include "LAN.h"
#include "LAN_common.h"

// various constants used everywhere
int ARTNET_PORT = 6454;
int ARTNET_STRING_SIZE = 8;
char ARTNET_STRING[] = "Art-Net";
uint8_t ARTNET_VERSION = 14;
uint8_t OEM_HI = 0x04;
uint8_t OEM_LO = 0x30;
char ESTA_HI = 'z';
char ESTA_LO = 'p';
uint8_t TTM_BEHAVIOUR_MASK = 0x02;
uint8_t TTM_REPLY_MASK = 0x01;
uint8_t PORT_STATUS_LPT_MODE = 0x02;
uint8_t PORT_STATUS_SHORT = 0x04;
uint8_t PORT_STATUS_ERROR = 0x04;
uint8_t PORT_STATUS_DISABLED_MASK = 0x08;
uint8_t PORT_STATUS_MERGE = 0x08;
uint8_t PORT_STATUS_DMX_TEXT = 0x10;
uint8_t PORT_STATUS_DMX_SIP = 0x20;
uint8_t PORT_STATUS_DMX_TEST = 0x40;
uint8_t PORT_STATUS_ACT_MASK = 0x80;
uint8_t PORT_DISABLE_MASK = 0x01;
uint8_t MIN_PACKET_SIZE = 10;

/*
 * Init a new ArtNet node.
 */
int LAN_init(artnet_node_t *node) {

    memset(node, 0x00, sizeof(node));

    node->swin[0] = 0x00;
    node->swin[1] = 0x01;
    node->swin[2] = 0x02;
    node->swin[3] = 0x03;
    node->swout[0] = 0x00;
    node->swout[1] = 0x01;
    node->swout[2] = 0x02;
    node->swout[3] = 0x03;

    memset(node->ports.types, 0x00, 4);
    node->ports.types[0] = 0x80;

    memset(node->ports.input, 0x00, 4);
    memset(node->ports.output, 0x00, 4);
    node->ports.input[0] = 0x02;


    sprintf((char *)node->id, ARTNET_STRING);
    node->esta_hi = (uint8_t)'E';        // The ESTA manufacturer code.
    node->esta_lo = (uint8_t)'M';        // The ESTA manufacturer code.
    node->fmw_hi       = 0;        // high byte of Node firmware revision number.
    node->fmw_lo        = 1;        // low byte of Node firmware revision number.
    node->oem_hi       = 0;        // high byte of the oem value.
    node->oem_lo        = 0xFF;     // low byte of the oem value. (0x00FF = developer code)
    node->ubea       = 0;        // This field contains the firmware version of the User Bios Extension Area (UBEA). 0 if not used
    node->status     = ARTNET_STANDBY;
    node->swvideo    = 0;
    node->swmacro    = 0;
    node->swremote   = 0;

    node->dmx_callback = NULL;

    node->status = ARTNET_ON;

    return ARTNET_EOK;
}

void LAN_set_port(artnet_node_t *node, uint8_t subnet_hi, uint8_t subnet_lo) {
    node->subnet_hi = subnet_hi;
    node->subnet_lo = subnet_lo;
}

void LAN_set_dmx(artnet_node_t *node, uint8_t dstart, uint8_t dfootprint) {
    node->dmx_start = dstart;
    node->dmx_footprint = dfootprint;
}

void LAN_set_dmx_callback(artnet_node_t *node, void (*cb)(uint16_t port, uint8_t *dmx)) {
    node->dmx_callback = cb;
}

void LAN_set_network(artnet_node_t *node, in_addr ip,
        in_addr bcast, in_addr gateway, in_addr netmask, uint8_t *mac_addr) {

    node->ip_addr = ip;
    node->bcast_addr = bcast;
    node->gateway_addr = gateway;
    node->subnet_mask = netmask;

    memcpy(node->mac_addr, mac_addr, ARTNET_MAC_SIZE);
}

void LAN_announce(artnet_node_t *node) {
    node->reply_addr = node->bcast_addr;
    LAN_send_poll_reply(node, 0);
}

void LAN_set_name(artnet_node_t *node, const char *short_name, const char *long_name) {
    memcpy(node->short_name, short_name, ARTNET_SHORT_NAME_LENGTH);
    memcpy(node->long_name, long_name, ARTNET_LONG_NAME_LENGTH);
}

void LAN_handle_poll(artnet_node_t *node, artnet_packet_t *p) {
    node->reply_addr = p->from;
    LAN_send_poll_reply(node, 1);
}

void LAN_handle_dmx(artnet_node_t *node, artnet_packet_t *p) {
    uint8_t dmx_data[LAN_DMX_FOOTPRINT];
    uint16_t port;
    memcpy(&dmx_data, p->data.admx.data + node->dmx_start, node->dmx_footprint);
    if (node->dmx_callback != NULL)
        node->dmx_callback(port, dmx_data);
}
