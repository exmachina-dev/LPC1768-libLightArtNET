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
#include "AN_ethernet.h"
#include "NetworkInterface.h"
#include "nsapi_types.h"

#include "private.h"

extern EthernetInterface* ARTNET_ETH_PTR;

extern Serial* USBport;


enum { INITIAL_IFACE_COUNT = 10 };
enum { IFACE_COUNT_INC = 5 };
enum { IFNAME_SIZE = 32 }; // 32 sounds a reasonable size

typedef struct iface_s {
    struct SocketAddress ip_addr;
    struct SocketAddress bcast_addr;
    int8_t hw_addr[ARTNET_MAC_SIZE];
    char   if_name[IFNAME_SIZE];
    struct iface_s *next;
} iface_t;

unsigned long LOOPBACK_IP = 0x7F000001;




/*
 * Scan for interfaces, and work out which one the user wanted to use.
 */
int artnet_net_init(node n, const char *ip, const char *bcast, const char *gw) {

    int ret = ARTNET_EOK;
    int rtn;

    in_addr saddr;
    in_addr baddr;
    in_addr netmask;

    if (ip) {
        if (rtn = ARTNET_ETH_PTR->set_network(ip, bcast, gw) != 0) {
            artnet_error("Failed to set network: %d", rtn);
            return ARTNET_ENET;
        }
    }

    rtn = ARTNET_ETH_PTR->connect();

    if (rtn != 0) {
        artnet_error("Failed to set network: %d", rtn);
        return ARTNET_ENET;
    }

    saddr.s_addr = inet_addr(ARTNET_ETH_PTR->get_ip_address());
    netmask.s_addr = inet_addr(ARTNET_ETH_PTR->get_netmask());

    baddr.s_addr = (uint32_t) saddr.s_addr & (uint32_t) netmask.s_addr;
    baddr.s_addr |= (uint32_t) 0xffffffff & (uint32_t) ~netmask.s_addr;

    //baddr.s_addr = inet_addr("192.168.0.21");

    n->state.ip_addr = saddr;
    n->state.bcast_addr = baddr;
    if (n->state.verbose) {
        // ARTNET_PRINTF("AN_IP: %s\r\n" , inet_ntoa(n->state.ip_addr));
        // ARTNET_PRINTF("AN_BD: %s\r\n" , inet_ntoa(n->state.bcast_addr));
    }
    uint8_t mac[6] = {0x00, 0x02, 0xf7, 0xf2, 0xa8, 0x30};
    memcpy(&n->state.hw_addr, &mac, ARTNET_MAC_SIZE);

    return ret;
}


/*
 * Start listening on the socket
 */
int artnet_net_start(node n) {
    artnet_socket_t sock;
    node tmp;

    // only attempt to bind if we are the group master
    if (n->peering.master == TRUE) {

        // create socket
        sock = new UDPSocket();
        int nsapi_rtn;


        if ((nsapi_rtn = sock->open(ARTNET_ETH_PTR)) != 0) {
            artnet_error("Could not create socket: %d", nsapi_rtn);
            return ARTNET_ENET;
        }


        if (n->state.verbose)
            ARTNET_PRINTF("Binding to %d \n", ARTNET_PORT);

        // allow bcasting
        if(sock->set_broadcast(true) != 0) {
            artnet_error("Failed to bind to socket %s", artnet_net_last_error());
            artnet_net_close(sock);
            return ARTNET_ENET;
        }

        sock->set_blocking(false);
    }

    // allow reusing 6454 port _ 
    // Not possible with UDPSocket ?
    uint8_t value = 1;
    if (sock->setsockopt((int)NSAPI_SOCKET, (int)NSAPI_REUSEADDR, &value, sizeof(int))) {
        artnet_error("Failed to bind to socket %s", artnet_net_last_error());
        artnet_net_close(sock);
        return ARTNET_ENET;
    }

    if (n->state.verbose)
        ARTNET_PRINTF("Binding to %d \n", ARTNET_PORT);

    // bind sockets
    if (int ret = sock->bind(ARTNET_PORT) != 0) {
        artnet_error("Failed to bind to socket: %d %s", ret, artnet_net_last_error());
        artnet_net_close(sock);
        return ARTNET_ENET;
    }


    n->sd = sock;

    // Propagate the socket to all our peers
    for (tmp = n->peering.peer; tmp && tmp != n; tmp = tmp->peering.peer)
        tmp->sd = sock;

    return ARTNET_EOK;
}


/*
 * Receive a packet.
 */
int artnet_net_recv(node n, artnet_packet p, int delay) {
    ssize_t len;
    SocketAddress cliAddr;
    // int maxfdp1 = n->sd + 1;

    p->length = 0;

    /*
    switch (select(maxfdp1, &rset, NULL, NULL, &tv)) {
        case 0:
            // timeout
            return RECV_NO_DATA;
            break;
        case -1:
            if ( errno != EINTR) {
                artnet_error("Select error %s", artnet_net_last_error());
                return ARTNET_ENET;
            }
            return ARTNET_EOK;
            break;
        default:
            break;
    }
    */

    // need a check here for the amount of data read
    // should prob allow an extra byte after data, and pass the size as sizeof(Data) +1
    // then check the size read and if equal to size(data)+1 we have an error
    len = n->sd->recvfrom(
            &cliAddr,
            &(p->data), // char* for win32
            sizeof(p->data));

    if (len < 0) {
        artnet_error("Recvfrom error %s", artnet_net_last_error());
        return ARTNET_ENET;
    }

    in_addr_t _ip = ntohl((uint32_t)cliAddr.get_addr().bytes);
    
    if (_ip == n->state.ip_addr.s_addr ||
            ntohl(_ip) == LOOPBACK_IP) {
        p->length = 0;
        return ARTNET_EOK;
    }

    p->length = len;
    memcpy(&(p->from), &_ip, sizeof(struct in_addr));
    // should set to in here if we need it
    return ARTNET_EOK;
}


/*
 * Send a packet.
 */
int artnet_net_send(node n, artnet_packet p) {
    SocketAddress addr;
    uint8_t ip_bytes[ARTNET_IP_SIZE];
    int ret;

    if (n->state.mode != ARTNET_ON)
        return ARTNET_EACTION;

    addr.set_port(ARTNET_PORT);
    memcpy(&ip_bytes, &(p->to.s_addr), ARTNET_IP_SIZE);
    addr.set_ip_bytes(ip_bytes, NSAPI_IPv4);
    p->from = n->state.ip_addr;

    if (n->state.verbose)
        ARTNET_PRINTF("Sending to %s:%d, data size: %d\r\n" , addr.get_ip_address(), addr.get_port(), p->length);

    ret = n->sd->sendto(addr, (void*)&p->data, p->length);

    if (ret < 0) {
        artnet_error("Sendto failed: %d", ret);
        n->state.report_code = ARTNET_RCUDPFAIL;
        return ARTNET_ENET;

    } else if (p->length != ret) {
        artnet_error("failed to send full datagram");
        n->state.report_code = ARTNET_RCSOCKETWR1;
        return ARTNET_ENET;
    }

    if (n->callbacks.send.fh) {
        get_type(p);
        n->callbacks.send.fh(n, p, n->callbacks.send.data);
    }
    return ARTNET_EOK;
}


int artnet_net_set_fdset(node n, fd_set *fdset) {
    // FD_SET((unsigned int) n->sd, fdset);
    return ARTNET_EOK;
}


/*
 * Close a socket
 */
int artnet_net_close(artnet_socket_t sock) {
    if (sock->close()) {
        artnet_error(artnet_net_last_error());
        return ARTNET_ENET;
    }
    return ARTNET_EOK;
}


/*
 * Convert a string to an in_addr
 */
int artnet_net_inet_aton(const char *ip_address, struct in_addr *address) {
#ifdef HAVE_INET_ATON
    if (!inet_aton(ip_address, address)) {
#else
        in_addr_t *addr = (in_addr_t*) address;
        if ((*addr = inet_addr(ip_address)) == INADDR_NONE &&
                strcmp(ip_address, "255.255.255.255")) {
#endif
            artnet_error("IP conversion from %s failed", ip_address);
            return ARTNET_EARG;
        }
        return ARTNET_EOK;
    }


    /*
     *
     */
    const char *artnet_net_last_error() {
        return strerror(errno);
    }

