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
 * network.c
 * Network code for libartnet
 * Copyright (C) 2004-2007 Simon Newton
 *
 */

//#include <errno.h>


#include "mbed.h"
#include "NetworkInterface.h"
#include "nsapi_types.h"

#include "LAN.h"
#include "LAN_common.h"

#define LOOPBACK_IP  (0x0100007F)

/*
 * Receive a packet.
 */
int LAN_recv(artnet_node_t *node, artnet_packet_t *p) {
    ssize_t len;
    SocketAddress client_addr;
    in_addr_t client_ip;

    if (LAN_sock == NULL)
        return ARTNET_ENET;

    p->length = 0;

    len = LAN_sock->recvfrom(&client_addr, &(p->data), sizeof(p->data));

    if (len < 0) {
        return (int)len;
    }

    client_ip = inet_addr(client_addr.get_ip_address());

    if (client_ip == node->ip_addr.s_addr || client_ip == LOOPBACK_IP) {
        return ARTNET_EOK;
    }

    p->length = len;

    memcpy(&(p->from), &client_ip, sizeof(struct in_addr));
    // should set to in here if we need it

    return ARTNET_EOK;
}

/*
 * Send a packet.
 */
int LAN_send(artnet_node_t *node, artnet_packet_t *packet) {
    SocketAddress addr;
    uint8_t ip_bytes[ARTNET_IP_SIZE];
    int ret;

    if (LAN_sock == NULL)
        return ARTNET_ENET;

    if (node->status != ARTNET_ON)
        return ARTNET_EACTION;

    addr.set_port(ARTNET_PORT);
    memcpy(&ip_bytes, &(packet->to.s_addr), ARTNET_IP_SIZE);
    addr.set_ip_bytes(ip_bytes, NSAPI_IPv4);
    packet->from = node->ip_addr;

    ret = LAN_sock->sendto(addr, (void*)&packet->data, packet->length);

    if (ret < 0) {
        // artnet_error("Sendto failed: %d", ret);
        node->report_code = ARTNET_RCUDPFAIL;
        return ARTNET_ENET;

    } else if (packet->length != ret) {
        // artnet_error("failed to send full datagram");
        node->report_code = ARTNET_RCSOCKETWR1;
        return ARTNET_ENET;
    }

    return ARTNET_EOK;
}
