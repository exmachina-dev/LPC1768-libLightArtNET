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
 * transmit.c
 * Functions to handle sending datagrams
 * Copyright (C) 2004-2005 Simon Newton
 */

#include "LAN.h"
#include "LAN_common.h"

/*
 * Send an ArtPollReply
 * @param n the node
 * @param response true if this reply is in response to a network packet
 *            false if this reply is due to the node changing it's conditions
 */
int LAN_send_poll_reply(artnet_node_t *node, int response) {
  int i;

  LAN_fill_poll_reply(node, &LAN_packet->data.ar);

  LAN_packet->to = node->reply_addr;

  LAN_packet->type = ARTNET_REPLY;
  LAN_packet->length = sizeof(artnet_reply_t);

  // for (i=0; i< ARTNET_MAX_PORTS; i++) {
  //   LAN_packet.data.ar.goodinput[i] = 0x00;
  //   LAN_packet.data.ar.goodoutput[i] = node->ports.output[i];
  // }

  snprintf((char *) &LAN_packet->data.ar.nodereport,
           sizeof(LAN_packet->data.ar.nodereport),
           "%04x [%04i] libartnet",
           node->report_code,
           0);

  return LAN_send(node, LAN_packet);
}

void LAN_fill_poll_reply(artnet_node_t *node, artnet_reply_t *poll_reply)
{
    //fill to 0's
    memset (poll_reply, 0, sizeof(poll_reply));

    // TODO: Finish fill_poll_reply
    //copy data from node
    memcpy (poll_reply->ip, &node->ip_addr.s_addr, sizeof(poll_reply->ip));
    memcpy (poll_reply->mac, node->mac_addr, sizeof(poll_reply->mac)); 
    memcpy (poll_reply->shortname, node->short_name, sizeof(poll_reply->shortname)); 
    memcpy (poll_reply->longname, node->long_name, sizeof(poll_reply->longname));
    memcpy (poll_reply->nodereport, node->report, sizeof(poll_reply->nodereport));
    memcpy (poll_reply->porttypes, node->ports.types, sizeof(poll_reply->porttypes));

    memcpy (poll_reply->goodinput, node->ports.input, sizeof(poll_reply->goodinput));
    memcpy (poll_reply->goodoutput, node->ports.output, sizeof(poll_reply->goodoutput));

    memcpy (poll_reply->swin, node->swin, sizeof(poll_reply->swin));
    memcpy (poll_reply->swout, node->swout, sizeof(poll_reply->swout));
    poll_reply->etsaman[0] = node->esta_hi;
    poll_reply->etsaman[1] = node->esta_lo;

    memcpy(poll_reply->id, node->id, sizeof(poll_reply->id));

    poll_reply->goodoutput[0]   = 0x80;
    poll_reply->opCode          = ARTNET_REPLY;  // ARTNET_REPLY
    poll_reply->port            = ARTNET_PORT;
    poll_reply->verH            = node->fmw_hi;
    poll_reply->ver             = node->fmw_lo;
    poll_reply->subH            = node->subnet_hi;
    poll_reply->sub             = node->subnet_lo;
    poll_reply->oemH            = node->oem_hi;
    poll_reply->oem             = node->oem_lo;
    poll_reply->status          = node->status;
    poll_reply->numbportsH      = 0x00;
    poll_reply->numbports       = 0x01;
    poll_reply->style           = ARTNET_NODE; 
}
