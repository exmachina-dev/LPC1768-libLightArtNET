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
#include "private.h"
#include "serial.h"

// various constants used everywhere
int ARTNET_ADDRESS_NO_CHANGE = 0x7f;
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
uint8_t PROGRAM_NO_CHANGE = 0x7f;
uint8_t PROGRAM_DEFAULTS = 0x00;
uint8_t PROGRAM_CHANGE_MASK = 0x80;
uint8_t HIGH_NIBBLE = 0xF0;
uint8_t LOW_NIBBLE = 0x0F;
uint8_t STATUS_PROG_AUTH_MASK = 0x30;
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
uint8_t TOD_RESPONSE_FULL = 0x00;
uint8_t TOD_RESPONSE_NAK = 0x00;
uint8_t MIN_PACKET_SIZE = 10;
uint8_t MERGE_TIMEOUT_SECONDS = 10;
uint8_t FIRMWARE_TIMEOUT_SECONDS = 20;
uint8_t RECV_NO_DATA = 1;
uint8_t MAX_NODE_BCAST_LIMIT = 30; // always bcast after this point

#ifndef TRUE
int TRUE = 1;
int FALSE = 0;
#endif

uint16_t LOW_BYTE = 0x00FF;
uint16_t HIGH_BYTE = 0xFF00;

node AN_NODE = NULL;

/*
 * Creates a new ArtNet node.
 * Takes a string containing the ip address to bind to, if the string is NULL
 * it uses the first non loopback address
 *
 * @param ip the IP address to bind to
 * @param debug level of logging provided 0: none
 * @return an artnet_node, or NULL on failure
 */
artnet_node artnet_new(const char *ip, const char *bcast, const char *gw, int verbose) {
    node n;
    //UDPSocket _sock;
    int i;

    n = (artnet_node_s *) malloc(sizeof(artnet_node_t));

    printf("node_t: %d, %d\r\n", n, sizeof(artnet_node_t));
    printf("packet_t: %d, %d\r\n", n, sizeof(artnet_packet_t));
    printf("socket_t: %d, %d\r\n", n, sizeof(artnet_socket_t));
    printf("state_t: %d, %d\r\n", n, sizeof(node_state_t));
    printf("callbacks_t: %d, %d\r\n", n, sizeof(node_callbacks_t));
    printf("reply_t: %d, %d\r\n", n, sizeof(artnet_reply_t));
    ARTNET_PRINTF("peering_t: %d, %d\r\n", n, sizeof(node_peering_t));

    if (!n) {
        artnet_error("malloc failure");
        return NULL;
    }

    memset(n, 0x0, sizeof(artnet_node_t));

    // init node listing
    n->state.verbose = verbose;
    n->state.oem_hi = OEM_HI;
    n->state.oem_lo = OEM_LO;
    n->state.esta_hi = ESTA_HI;
    n->state.esta_lo = ESTA_LO;
    n->state.bcast_limit = 0;

    n->peering.peer = NULL;
    n->peering.master = TRUE;

    //n->sd = &_sock;

    if (artnet_net_init(n, ip, bcast, gw)) {
        free(n);
        return NULL;
    }

    // now setup the default parameters
    n->state.send_apr_on_change = FALSE;
    n->state.ar_count = 0;
    n->state.report_code = ARTNET_RCPOWEROK;
    n->state.reply_addr.s_addr = 0x0;
    n->state.mode = ARTNET_STANDBY;

    // set all ports to MERGE HTP mode and disable
    for (i=0; i < ARTNET_MAX_PORTS; i++) {
        n->ports.out[i].merge_mode = ARTNET_MERGE_HTP;
        n->ports.out[i].port_enabled = FALSE;
        n->ports.in[i].port_enabled = FALSE;

        // reset tods
        reset_tod(&n->ports.in[i].port_tod);
        reset_tod(&n->ports.out[i].port_tod);
    }
    return n;
}


/*
 * Starts the ArtNet node.
 * Binds the network socket and sends an ArtPoll
 * @param vn the artnet_node
 * @return 0 on success, non 0 on failure
 *
 */
int artnet_start(artnet_node vn) {
    node n = (node) vn;
    int ret;
    check_nullnode(vn);

    if (n->state.mode != ARTNET_STANDBY)
        return ARTNET_ESTATE;

    if ((ret = artnet_net_start(n)))
        return ret;

    n->state.mode = ARTNET_ON;

    if (n->state.reply_addr.s_addr == 0) {
        n->state.reply_addr = n->state.bcast_addr;
    } else {
        // ARTNET_PRINTF("%d\r\n", n->state.reply_addr);
    }

    // build the initial reply
    if ((ret = artnet_tx_build_art_poll_reply(n)))
        return ret;

    if (n->state.node_type == ARTNET_SRV) {
        // poll the network
        if ((ret = artnet_tx_poll(n,NULL, ARTNET_TTM_AUTO)))
            return ret;

    } else {
        // send a reply on startup
        if (n->state.verbose)
            // ARTNET_PRINTF("Sending POLL REPLY.\r\n");

        if ((ret = artnet_tx_poll_reply(n, FALSE)))
            return ret;
    }
    return ret;
}


/*
 * Stops the ArtNet node. This closes the network sockets held by the node
 * @param vn the artnet_node
 * @return 0 on success, non-0 on failure
 */
int artnet_stop(artnet_node vn) {
    node n = (node) vn;
    check_nullnode(vn);

    if (n->state.mode != ARTNET_ON)
        return ARTNET_EACTION;

    artnet_net_close(n->sd);
    n->state.mode = ARTNET_STANDBY;
    return ARTNET_EOK;
}


/*
 * Free the memory associated with this node
 */
int artnet_destroy(artnet_node vn) {
    node n = (node) vn;
    int i;

    check_nullnode(vn);

    free(vn);
    return ARTNET_EOK;
}


/*
 * Set the OEM code
 * This can only be done in the standby state
 */
int artnet_setoem(artnet_node vn, uint8_t hi, uint8_t lo) {
    node n = (node) vn;
    check_nullnode(vn);

    if (n->state.mode != ARTNET_STANDBY)
        return ARTNET_ESTATE;

    n->state.oem_hi = hi;
    n->state.oem_lo = lo;
    return ARTNET_EOK;
}


/*
 * Set the ESTA code
 * This can only be done in the standby state
 */
int artnet_setesta(artnet_node vn, char hi, char lo) {
    node n = (node) vn;
    check_nullnode(vn);

    if (n->state.mode != ARTNET_STANDBY)
        return ARTNET_ESTATE;

    n->state.esta_hi = hi;
    n->state.esta_lo = lo;
    return ARTNET_EOK;
}


/*
 * Set the number of nodes above which we start to bcast data
 * @param vn the artnet_node
 * @param limit 0 to always broadcast
 */
int artnet_set_bcast_limit(artnet_node vn, int limit) {
    node n = (node) vn;
    check_nullnode(vn);

    if (limit > MAX_NODE_BCAST_LIMIT) {
        artnet_error("attempt to set bcast limit > %d", MAX_NODE_BCAST_LIMIT);
        return ARTNET_EARG;
    }

    n->state.bcast_limit = limit;
    return ARTNET_EOK;
}

/*
 * Handle any received packets.
 * This function is the workhorse of libartnet. You have a couple of options:
 *   - use artnet_get_sd() to retrieve the socket descriptors and select to
 *     detect network activity. Then call artnet_read(node,0)
 *     when activity is detected.
 *   - call artnet_read repeatedly from within a loop with an appropriate
 *     timeout
 *
 * @param vn the artnet_node
 * @param timeout the number of seconds to block for if nothing is pending
 * @return 0 on success, -1 on failure
 */
int artnet_read(artnet_node vn, int timeout) {
    node n = (node) vn;
    node tmp;
    artnet_packet_t p;
    int ret;

    check_nullnode(vn);

    if (n->state.mode != ARTNET_ON)
        return ARTNET_EACTION;

    while (1) {
        memset(&p.data, 0x0, sizeof(p.data));

        if ((ret = artnet_net_recv(n, &p, timeout)) < 0)
            return ret;

        // nothing to read
        if (ret == RECV_NO_DATA)
            break;

        // skip this packet (filtered)
        if (p.length == 0)
            continue;

        if (p.length > MIN_PACKET_SIZE && get_type(&p)) {
            handle(n, &p);
            for (tmp = n->peering.peer; tmp != NULL && tmp != n; tmp = tmp->peering.peer) {
                handle(tmp, &p);
            }
        }
    }
    return ARTNET_EOK;
}


/*
 * To get around the 4 universes per node limitation , we can start more than
 * one node on different ip addresses - you'll need to add aliases to your
 * network interface something like:
 *
 * $ ifconfig eth0:1 10.0.0.10 netmask 255.255.255.0
 *
 * Then the nodes must be joined so that they can share the socket
 * bound to the broadcast address.
 * TODO: use IP_PKTINFO so that packets are sent from the correct source ip
 *
 * @param vn1 The artnet node
 * @param vn2 The second artnet node
 *
 * @return 0 on sucess, non 0 on failure
 */
int artnet_join(artnet_node vn1, artnet_node vn2) {

    check_nullnode(vn1);
    check_nullnode(vn2);

    node n1 = (node) vn1;
    node n2 = (node) vn2;
    node tmp, n;

    if (n1->state.mode == ARTNET_ON || n2->state.mode == ARTNET_ON) {
        artnet_error("%s called after artnet_start", __FUNCTION__);
        return ARTNET_EACTION;
    }

    tmp = n1->peering.peer == NULL ? n1 : n1->peering.peer;
    n1->peering.peer = n2;
    for (n = n2; n->peering.peer != NULL && n->peering.peer != n2; n = n->peering.peer) ;
    n->peering.peer = tmp;

    // make sure there is only 1 master
    for (n = n1->peering.peer; n != n1; n = n->peering.peer)
        n->peering.master = FALSE;

    n1->peering.master = TRUE;

    return ARTNET_ESTATE;
}


/*
 * This is used to set handlers for sent/received artnet packets.
 * If you're using a stock standard node you more than likely don't want
 * to set these. See the artnet_set_dmx_callback and artnet_set_firmware_callback.
 * If you want to get down and dirty with artnet packets, you can set this
 * read / manipulate packets as they arrive (or get sent)
 *
 * @param vn The artnet_node
 * @param handler The handler to set
 * @param fh A pointer to a function, set to NULL to turn off
 *           The function should return 0,
 * @param data Data to be passed to the handler when its called
 * @return 0 on sucess, non 0 on failure
 */
int artnet_set_handler(artnet_node vn,
        artnet_handler_name_t handler,
        int (*fh)(artnet_node n, void *pp, void * d),
        void *data) {
    node n = (node) vn;
    callback_t *callback;
    check_nullnode(vn);

    switch(handler) {
        case ARTNET_RECV_HANDLER:
            callback = &n->callbacks.recv;
            break;
        case ARTNET_SEND_HANDLER:
            callback = &n->callbacks.send;
            break;
        case ARTNET_POLL_HANDLER:
            callback = &n->callbacks.poll;
            break;
        case ARTNET_REPLY_HANDLER:
            callback = &n->callbacks.reply;
            break;
        case ARTNET_ADDRESS_HANDLER:
            callback = &n->callbacks.address;
            break;
        case ARTNET_DMX_HANDLER:
            callback = &n->callbacks.dmx;
            break;
        default:
            artnet_error("%s : Invalid handler defined", __FUNCTION__);
            return ARTNET_EARG;
    }
    callback->fh = fh;
    callback->data = data;
    return ARTNET_EOK;
}


/*
 * This is a special callback which is invoked when dmx data is received.
 *
 * @param vn The artnet_node
 * @param fh The callback to invoke (parameters passwd are the artnet_node, the port_id
 *           that received the dmx, and some user data
 * @param data    Data to be passed to the handler when its called
 */
int artnet_set_dmx_handler(artnet_node vn,
        int (*fh)(artnet_node n, int port, void *d),
        void *data) {
    node n = (node) vn;
    check_nullnode(vn);

    n->callbacks.dmx_c.fh = fh;
    n->callbacks.dmx_c.data = data;
    return ARTNET_EOK;
}


/*
 * Sends an artpoll reply
 *
 * @param vn the artnet_node
 */
int artnet_send_poll_reply(artnet_node vn) {
    node n = (node) vn;
    check_nullnode(vn);

    if (n->state.mode != ARTNET_ON)
        return ARTNET_EACTION;

    return artnet_tx_poll_reply(n, FALSE);
}


#ifdef ARTNET_FEATURE_NODE_LIST
/*
 * Sends some dmx data
 *
 * @param vn the artnet_node
 */
int artnet_send_dmx(artnet_node vn,
        int port_id,
        int16_t length,
        const uint8_t *data) {
    node n = (node) vn;
    artnet_packet_t p;
    int ret;
    input_port_t *port;

    check_nullnode(vn);

    if (n->state.mode != ARTNET_ON)
        return ARTNET_EACTION;

    if (port_id < 0 || port_id >= ARTNET_MAX_PORTS) {
        artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port_id);
        return ARTNET_EARG;
    }
    port = &n->ports.in[port_id];

    if (length < 1 || length > ARTNET_DMX_LENGTH) {
        artnet_error("%s : Length of dmx data out of bounds (%i < 1 || %i > ARTNET_MAX_DMX)", __FUNCTION__, length);
        return ARTNET_EARG;
    }

    if (port->port_status & PORT_STATUS_DISABLED_MASK) {
        artnet_error("%s : attempt to send on a disabled port (id:%i)", __FUNCTION__, port_id);
        return ARTNET_EARG;
    }

    // ok we're going to send now, make sure we turn the activity bit on
    port->port_status = port->port_status | PORT_STATUS_ACT_MASK;

    p.length = sizeof(artnet_dmx_t) - (ARTNET_DMX_LENGTH - length);

    // now build packet
    memcpy(&p.data.admx.id, ARTNET_STRING, ARTNET_STRING_SIZE);
    p.data.admx.opCode =  htols(ARTNET_DMX);
    p.data.admx.verH = 0;
    p.data.admx.ver = ARTNET_VERSION;
    p.data.admx.sequence = port->seq;
    p.data.admx.physical = port_id;
    p.data.admx.universe = htols(port->port_addr);

    // set length
    p.data.admx.lengthHi = short_get_high_byte(length);
    p.data.admx.length = short_get_low_byte(length);
    memcpy(&p.data.admx.data, data, length);

    // default to bcast
    p.to = n->state.bcast_addr;

    if (n->state.bcast_limit == 0) {
        if ((ret = artnet_net_send(n, &p)))
            return ret;
    } else {
        int nodes;
        // find the number of ports for this uni
        SI *ips = (in_addr *) malloc(sizeof(SI) * n->state.bcast_limit);

        if (!ips) {
            // Fallback to broadcast mode
            if ((ret = artnet_net_send(n, &p)))
                return ret;
        }

        nodes = find_nodes_from_uni(&n->node_list,
                port->port_addr,
                ips,
                n->state.bcast_limit);

        if (nodes > n->state.bcast_limit) {
            // fall back to broadcast
            free(ips);
            if ((ret = artnet_net_send(n, &p))) {
                return ret;
            }
        } else {
            // unicast to the specified nodes
            int i;
            for (i =0; i < nodes; i++) {
                p.to = ips[i];
                artnet_net_send(n, &p);
            }
            free(ips);
        }
    }
    port->seq++;
    return ARTNET_EOK;
}


/*
 * Use for performance testing.
 * This allows data to be sent on any universe, not just the ones that have
 * ports configured.
 */
int artnet_raw_send_dmx(artnet_node vn,
        uint8_t uni,
        int16_t length,
        const uint8_t *data) {
    node n = (node) vn;
    artnet_packet_t p;

    check_nullnode(vn);

    if (n->state.mode != ARTNET_ON)
        return ARTNET_EACTION;

    if (n->state.node_type != ARTNET_RAW)
        return ARTNET_ESTATE;

    if ( length < 1 || length > ARTNET_DMX_LENGTH) {
        artnet_error("%s : Length of dmx data out of bounds (%i < 1 || %i > ARTNET_MAX_DMX)", __FUNCTION__, length);
        return ARTNET_EARG;
    }

    // set dst addr and length
    p.to = n->state.bcast_addr;

    p.length = sizeof(artnet_dmx_t) - (ARTNET_DMX_LENGTH - length);

    // now build packet
    memcpy( &p.data.admx.id, ARTNET_STRING, ARTNET_STRING_SIZE);
    p.data.admx.opCode = htols(ARTNET_DMX);
    p.data.admx.verH = 0;
    p.data.admx.ver = ARTNET_VERSION;
    p.data.admx.sequence = 0;
    p.data.admx.physical = 0;
    p.data.admx.universe = uni;

    // set length
    p.data.admx.lengthHi = short_get_high_byte(length);
    p.data.admx.length = short_get_low_byte(length);
    memcpy(&p.data.admx.data, data, length);

    return artnet_net_send(n, &p);
}



/*
 * Reads the latest dmx data
 * @param vn the artnet node
 * @param port_id the port to read data from
 * @param length
 * @return a pointer to the dmx data, NULL on error
 */

uint8_t *artnet_read_dmx(artnet_node vn, int port_id, int *length) {
    node n = (node) vn;

    if (n == NULL)
        return NULL;

    if (port_id < 0 || port_id >= ARTNET_MAX_PORTS) {
        artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port_id);
        return NULL;
    }

    *length = n->ports.out[port_id].length;
    return &n->ports.out[port_id].data[0];
}


//--------------------------------------
// Functions to change the node state (setters)

// type : server, node, mserver, raw
int artnet_set_node_type(artnet_node vn, artnet_node_type type) {
    node n = (node) vn;
    check_nullnode(vn);

    n->state.node_type = type;
    return ARTNET_EOK;
}


/**
 * Sets the artnet subnet address for this node.
 * The subnet address has nothing to do with IP addresses). An ArtNet subnet is a grouping of 16 DMX universes
 * (ie. ports)
 *
 * The subnet address is between 0 and 15. If the supplied address is larger than 15, the
 * lower 4 bits will be used in setting the address.
 *
 * It will have no effect if the node is under network control.
 *
 * Note that changing the subnet address will cause the universe addresses of all ports to change.
 *
 * @param vn the artnet_node
 * @param subnet new subnet address
 */
int artnet_set_subnet_addr(artnet_node vn, uint8_t subnet) {
    node n = (node) vn;
    int i, ret;

    check_nullnode(vn);

    n->state.default_subnet = subnet;

    // if not under network control, and the subnet is different from the current one
    if (!n->state.subnet_net_ctl && subnet != n->state.subnet) {
        n->state.subnet = subnet;

        // redo the addresses for each port
        for (i =0; i < ARTNET_MAX_PORTS; i++) {
            n->ports.in[i].port_addr = ((n->state.subnet & LOW_NIBBLE) << 4) | (n->ports.in[i].port_addr & LOW_NIBBLE);
            // reset dmx sequence number
            n->ports.in[i].seq = 0;

            n->ports.out[i].port_addr = ((n->state.subnet & LOW_NIBBLE) << 4) | (n->ports.out[i].port_addr & LOW_NIBBLE);
        }

        if (n->state.mode == ARTNET_ON) {

            if ((ret = artnet_tx_build_art_poll_reply(n)))
                return ret;

            return artnet_tx_poll_reply(n,FALSE);
        }
    } else if (n->state.subnet_net_ctl ) {
        //  trying to change subnet addr while under network control
        n->state.report_code = ARTNET_RCUSERFAIL;
    }

    return ARTNET_EOK;
}


/**
 * Sets the short name of the node.
 * The string should be null terminated and a maxmium of 18 Characters will be used
 *
 * @param vn the artnet_node
 * @param name the short name of the node.
 */
int artnet_set_short_name(artnet_node vn, const char *name) {
    node n = (node) vn;
    check_nullnode(vn);

    strncpy((char *) &n->state.short_name, name, ARTNET_SHORT_NAME_LENGTH);
    n->state.short_name[ARTNET_SHORT_NAME_LENGTH-1] = 0x00;
    return artnet_tx_build_art_poll_reply(n);
}


/*
 * Sets the long name of the node.
 * The string should be null terminated and a maximium of 64 characters will be used
 *
 * @param vn the artnet_node
 * @param name the node's long name
 */
int artnet_set_long_name(artnet_node vn, const char *name) {
    node n = (node) vn;
    check_nullnode(vn);

    strncpy((char *) &n->state.long_name, name, ARTNET_LONG_NAME_LENGTH);
    n->state.long_name[ARTNET_LONG_NAME_LENGTH-1] = 0x00;
    return artnet_tx_build_art_poll_reply(n);
}


/*
 * Sets the direction and type of port
 * @param vn the artnet_node
 * @param id
 * @param direction
 * @param data
 */
int artnet_set_port_type(artnet_node vn,
        int port_id,
        artnet_port_settings_t settings,
        artnet_port_data_code data) {
    node n = (node) vn;
    check_nullnode(vn);

    if (port_id < 0 || port_id >= ARTNET_MAX_PORTS) {
        artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port_id);
        return ARTNET_EARG;
    }

    n->ports.types[port_id] = settings | data;
    return ARTNET_EOK;
}


/*
 * Sets the port address of the port.
 *
 * Just to set some terms straight:
 *  - subnet address, is 4 bits, set on a per-node basis
 *  - port address, 4 bits, set on a per-port basis
 *  - universe address, 8 bits derrived from the subnet and port addresses, specific (but may
 *  not be unique) to a port.
 *
 * The upper four bits of the universe address are from the subnet address, while the lower
 * four are from the port address.
 *
 * So for example, if the subnet address of the node is 0x03, and the port address is
 * 0x02, the universe address for the port will be 0x32.
 *
 * As the port address is between 0 and 15, only the lower 4 bits of the addr argument
 * will be used.
 *
 * The operation may have no affect if the port is under network control.
 *
 * @param vn the artnet_node
 * @param id the phyiscal port number (from 0 to ARTNET_MAX_PORTS-1 )
 * @param dir either ARTNET_INPUT_PORT or ARTNET_OUTPUT_PORT
 * @param addr the new port address
 */
int artnet_set_port_addr(artnet_node vn,
        int id,
        artnet_port_dir_t dir,
        uint8_t addr) {
    node n = (node) vn;
    int ret;
    int changed = 0;

    g_port_t *port;

    check_nullnode(vn);

    if (id < 0 || id >= ARTNET_MAX_PORTS) {
        artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, id);
        return ARTNET_EARG;
    }

    if (addr > 16) {
        artnet_error("%s : Attempt to set port %i to invalid address %#hhx\n", __FUNCTION__, id, addr);
        return ARTNET_EARG;
    }

    if (dir == ARTNET_INPUT_PORT) {
        port = &n->ports.in[id].port;
        changed = n->ports.in[id].port_enabled?0:1;
        n->ports.in[id].port_enabled = TRUE;
    } else if (dir == ARTNET_OUTPUT_PORT) {
        port = &n->ports.out[id].port;
        changed = n->ports.out[id].port_enabled?0:1;
        n->ports.out[id].port_enabled = TRUE;
    } else {
        artnet_error("%s : Invalid port direction\n", __FUNCTION__);
        return ARTNET_EARG;
    }

    port->default_addr = addr;

    // if not under network control and address is changing
    if (!port->net_ctl &&
            (changed || (addr & LOW_NIBBLE) != (port->addr & LOW_NIBBLE))) {
        port->addr = ((n->state.subnet & LOW_NIBBLE) << 4) | (addr & LOW_NIBBLE);

        // reset seq if input port
        if (dir == ARTNET_INPUT_PORT)
            n->ports.in[id].seq = 0;

        if (n->state.mode == ARTNET_ON) {
            if ((ret = artnet_tx_build_art_poll_reply(n)))
                return ret;

            return artnet_tx_poll_reply(n,FALSE);
        }
    } else if (port->net_ctl) {
        //  trying to change port addr while under network control
        n->state.report_code = ARTNET_RCUSERFAIL;
    }
    return ARTNET_EOK;
}


/*
 * Returns the universe address of this port
 *
 * @param vn the artnet_node
 * @param id the phyiscal port number (from 0 to ARTNET_MAX_PORTS-1 )
 * @param dir either ARTNET_INPUT_PORT or ARTNET_OUTPUT_PO
 *
 * @return the universe address, or < 0 on error
 */
int artnet_get_universe_addr(artnet_node vn, int id, artnet_port_dir_t dir) {
    node n = (node) vn;
    check_nullnode(vn);

    if (id < 0 || id >= ARTNET_MAX_PORTS) {
        artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, id);
        return ARTNET_EARG;
    }

    if (dir == ARTNET_INPUT_PORT)
        return n->ports.in[id].port.addr;
    else if (dir == ARTNET_OUTPUT_PORT)
        return n->ports.out[id].port.addr;
    else {
        artnet_error("%s : Invalid port direction\n", __FUNCTION__);
        return ARTNET_EARG;
    }
}

int artnet_get_config(artnet_node vn, artnet_node_config_t *config) {
    int i;
    node n = (node) vn;
    check_nullnode(vn);

    strncpy(config->short_name, n->state.short_name, ARTNET_SHORT_NAME_LENGTH);
    strncpy(config->long_name, n->state.long_name, ARTNET_LONG_NAME_LENGTH);
    config->subnet = n->state.subnet;

    for (i = 0; i < ARTNET_MAX_PORTS; i++) {
        config->in_ports[i] = n->ports.in[i].port.addr & LOW_NIBBLE;
        config->out_ports[i] = n->ports.out[i].port.addr & LOW_NIBBLE;
    }

    return ARTNET_EOK;
}


/*
 * Dumps the node config to stdout.
 *
 * @param vn the artnet_node
 */
int artnet_dump_config(artnet_node vn) {
    node n = (node) vn;
    check_nullnode(vn);

    // ARTNET_PRINTF("#### NODE CONFIG ####\n");
    // ARTNET_PRINTF("Node Type: %i\n", n->state.node_type);
    // ARTNET_PRINTF("Short Name: %s\n", n->state.short_name);
    // ARTNET_PRINTF("Long Name: %s\n", n->state.long_name);
    // ARTNET_PRINTF("Subnet: %#02x\n", n->state.subnet);
    // ARTNET_PRINTF("Default Subnet: %#02x\n", n->state.default_subnet);
    // ARTNET_PRINTF("Net Ctl: %i\n", n->state.subnet_net_ctl);
    // ARTNET_PRINTF("#####################\n");

    return ARTNET_EOK;
}


/*
 * Returns the socket descriptor associated with this artnet_node.
 * libartnet currently uses two descriptors per node, one bound
 * to the network address and one bound to the subnet broadcast address
 *
 * @param vn the artnet_node
 * @param socket the index of the socket descriptor to fetch (0 or 1)
 * @return the socket descriptor
 */
int8_t artnet_get_sd(artnet_node vn, artnet_socket_t *fd) {
    node n = (node) vn;
    check_nullnode(vn);

    if (n->state.mode != ARTNET_ON)
        return ARTNET_EACTION;

    fd = &n->sd;
}


/**
 * Sets the file descriptors in the fdset that we are interested in.
 *
 * @param vn the artnet_node
 * @param fdset pointer to the fdset to change
 * @return the maxfd+1
 */
int artnet_set_fdset(artnet_node vn, fd_set *fdset) {
    node n = (node) vn;
    check_nullnode(vn);

    if (!fdset)
        return ARTNET_EARG;

    if (n->state.mode != ARTNET_ON)
        return ARTNET_EACTION;

    return artnet_net_set_fdset(n, fdset);
}


/*
 * Return a pointer to the staticly allocated error string
 */
char *artnet_strerror() {
    return artnet_errstr;
}
