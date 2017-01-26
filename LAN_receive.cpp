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
 * receive.c
 * Handles the receiving of datagrams
 * Copyright (C) 2004-2007 Simon Newton
 */

#include "LAN.h"
#include "LAN_common.h"

int LAN_read(artnet_node_t *node, artnet_packet_t *p) {
    int rtn;

    while (true) {
        memset(&p->data, 0x00, sizeof(p->data));

        if ((rtn = LAN_recv(node, p)) < 0)
            return rtn;

        // nothing to read
        if (rtn == ARTNET_ENET)
            break;

        // skip this packet (filtered)
        if (p->length == 0)
            continue;

        if (p->length > 12 && LAN_get_type(p)) {
            LAN_handle(node, p);
        }
    }
    return ARTNET_EOK;
}

int LAN_handle(artnet_node_t *node, artnet_packet_t *p) {
    switch (p->type) {
        case ARTNET_POLL:
            LAN_handle_poll(node, p);
            break;
        case ARTNET_ADDRESS:
            printf("address change");
            break;
        case ARTNET_DMX:
            LAN_handle_dmx(node, p);
            break;
    }

    p->length = 0;
    return ARTNET_EOK;
}

int16_t LAN_get_type(artnet_packet_t *p) {
    uint8_t *data;

    if (p->length < 10)
        return 0;
    if (!memcmp(&p->data, ARTNET_STRING, ARTNET_STRING_SIZE)) {
        data = (uint8_t *) &p->data;
        p->type = (artnet_packet_type_t) (((data[9]) << 8) + (data[8]));
        return p->type;
    }

    return 0;
}
