/*
 * Copyright (c) 2001-2002, Adam Dunkels.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Adam Dunkels.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: uip.c,v 1.1 2003/03/19 14:16:06 adamdunkels Exp $
 *
 */

/*
This is a small implementation of the IP and TCP protocols (as well as
some basic ICMP stuff). The implementation couples the IP, TCP and the
application layers very tightly. To keep the size of the compiled code
down, this code also features heavy usage of the goto statement.

The principle is that we have a small buffer, called the uip_buf, in
which the device driver puts an incoming packet. The TCP/IP stack
parses the headers in the packet, and calls upon the application. If
the remote host has sent data to the application, this data is present
in the uip_buf and the application read the data from there. It is up
to the application to put this data into a byte stream if needed. The
application will not be fed with data that is out of sequence.

If the application whishes to send data to the peer, it should put its
data into the uip_buf, 40 bytes from the start of the buffer. The
TCP/IP stack will calculate the checksums, and fill in the necessary
header fields and finally send the packet back to the peer.
*/

#include "uip.h"
#include "uipopt.h"
#include "uip_arch.h"

/*-----------------------------------------------------------------------------------*/
/* Variable definitions. */


/* The IP address of this host. If it is defined to be fixed (by setting UIP_FIXEDADDR to 1 in uipopt.h), the address is set here. Otherwise, the address */
#if UIP_FIXEDADDR > 0
#if UIP_IPV6
const u16_t uip_hostaddr[8] =
  {UIP_IP6ADDR0, UIP_IP6ADDR1,
   UIP_IP6ADDR2, UIP_IP6ADDR3,
   UIP_IP6ADDR4, UIP_IP6ADDR5,
   UIP_IP6ADDR6, UIP_IP6ADDR7};
#else /* UIP_IPV6 */
const u16_t uip_hostaddr[2] =
  {htons((UIP_IPADDR0 << 8) | UIP_IPADDR1),
   htons((UIP_IPADDR2 << 8) | UIP_IPADDR3)};
#endif /* UIP_IPV6 */
#else
#if UIP_IPV6
u16_t uip_hostaddr[8];       
#else /* UIP_IPV6 */
u16_t uip_hostaddr[2];       
#endif /* UIP_IPV6 */
#endif /* UIP_FIXEDADDR */

u8_t uip_buf[UIP_BUFSIZE];   /* The packet buffer that contains
				incoming packets. */
volatile u8_t *uip_appdata;  /* The uip_appdata pointer points to
				application data. */
#if UIP_URGDATA > 0
volatile u8_t *uip_urgdata;  /* The uip_urgdata pointer points to
				urgent data (out-of-band data), if
				present. */
volatile u8_t uip_urglen, uip_surglen;
#endif /* UIP_URGDATA > 0 */

#if UIP_BUFSIZE > 255
volatile u16_t uip_len, uip_slen;
                             /* The uip_len is either 8 or 16 bits,
				depending on the maximum packet
				size. */
#else
volatile u8_t uip_len, uip_slen;
#endif /* UIP_BUFSIZE > 255 */

volatile u8_t uip_flags;     /* The uip_flags variable is used for
				communication between the TCP/IP stack
				and the application program. */
struct uip_conn *uip_conn;   /* uip_conn always points to the current
				connection. */

struct uip_conn uip_conns[UIP_CONNS];
                             /* The uip_conns array holds all TCP
				connections. */
u16_t uip_listenports[UIP_LISTENPORTS];
                             /* The uip_listenports list all currently
				listning ports. */
#if UIP_UDP
struct uip_udp_conn *uip_udp_conn;
struct uip_udp_conn uip_udp_conns[UIP_UDP_CONNS];
#endif /* UIP_UDP */


static u16_t ipid;           /* Ths ipid variable is an increasing
				number that is used for the IP ID
				field. */

static u8_t iss[4];          /* The iss variable is used for the TCP
				initial sequence number. */

#if UIP_ACTIVE_OPEN
/* XXX static */ u16_t lastport;       /* Keeps track of the last port used for
				a new connection. */
#endif /* UIP_ACTIVE_OPEN */

/* Temporary variables. */
volatile u8_t uip_acc32[4];
static u8_t c, opt;
static u16_t tmpport;

/* Structures and definitions. */
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20
#define TCP_CTL 0x3f

#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO       8     

/* Macros. */
#define BUF ((uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])
#define FBUF ((uip_tcpip_hdr *)&uip_reassbuf[0])
#define ICMPBUF ((uip_icmpip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UDPBUF ((uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

#if UIP_STATISTICS == 1
struct uip_stats uip_stat;
#define UIP_STAT(s) s
#else
#define UIP_STAT(s)
#endif /* UIP_STATISTICS == 1 */

#if UIP_LOGGING == 1
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif /* UIP_LOGGING == 1 */

/*-----------------------------------------------------------------------------------*/
void
uip_init(void)
{
  for(c = 0; c < UIP_LISTENPORTS; ++c) {
    uip_listenports[c] = 0;
  }
  for(c = 0; c < UIP_CONNS; ++c) {
    uip_conns[c].tcpstateflags = CLOSED;
  }
#if UIP_ACTIVE_OPEN
  lastport = 1024;
#endif /* UIP_ACTIVE_OPEN */

#if UIP_UDP
  for(c = 0; c < UIP_UDP_CONNS; ++c) {
    uip_udp_conns[c].lport = 0;
  }
#endif /* UIP_UDP */
  
#if UIP_IPV6
  /* IPv6 initialization. */  
#if UIP_FIXEDADDR == 0
  uip_hostaddr[0] = uip_hostaddr[1] =
    uip_hostaddr[2] = uip_hostaddr[3] =
    uip_hostaddr[4] = uip_hostaddr[5] =
    uip_hostaddr[6] = uip_hostaddr[7] = 0;
#endif /* UIP_FIXEDADDR */
  
#else /* UIP_IPV6 */

  /* IPv4 initialization. */
#if UIP_FIXEDADDR == 0
  uip_hostaddr[0] = uip_hostaddr[1] = 0;
#endif /* UIP_FIXEDADDR */
  
#endif /* UIP_IPV6 */
}
/*-----------------------------------------------------------------------------------*/
#if UIP_ACTIVE_OPEN
struct uip_conn *
uip_connect(u16_t *ripaddr, u16_t rport)
{
  struct uip_conn *conn;
  
  /* Find an unused local port. */
 again:
  ++lastport;

  if(lastport >= 32000) {
    lastport = 4096;
  }
  
  for(c = 0; c < UIP_CONNS; ++c) {
    if(uip_conns[c].tcpstateflags != CLOSED &&
       uip_conns[c].lport == lastport)
      goto again;
  }


  conn = 0;
  for(c = 0; c < UIP_CONNS; ++c) {
    if(uip_conns[c].tcpstateflags == CLOSED) {
      conn = &uip_conns[c]; 
      break;
    }
    if(uip_conns[c].tcpstateflags == TIME_WAIT) {
      if(conn == 0 ||
	 uip_conns[c].timer > uip_conn->timer) {
	conn = &uip_conns[c];
      }
    }
  }

  if(conn == 0) {
    return 0;
  }
  
  conn->tcpstateflags = SYN_SENT;

  conn->snd_nxt[0] = iss[0];
  conn->snd_nxt[1] = iss[1];
  conn->snd_nxt[2] = iss[2];
  conn->snd_nxt[3] = iss[3];

  conn->len = 1;   /* TCP length of the SYN is one. */
  conn->nrtx = 0;
  conn->timer = 1; /* Send the SYN next time around. */
  conn->lport = htons(lastport);
  conn->rport = htons(rport);
#if UIP_IPV6
  conn->ripaddr[0] = ripaddr[0];
  conn->ripaddr[1] = ripaddr[1];
  conn->ripaddr[2] = ripaddr[2];
  conn->ripaddr[3] = ripaddr[3];
  conn->ripaddr[4] = ripaddr[4];
  conn->ripaddr[5] = ripaddr[5];
  conn->ripaddr[6] = ripaddr[6];
  conn->ripaddr[7] = ripaddr[7];
#else /* UIP_IPV6 */
  conn->ripaddr[0] = ripaddr[0];
  conn->ripaddr[1] = ripaddr[1];
#endif /* UIP_IPV6 */
  
  return conn;
}
#endif /* UIP_ACTIVE_OPEN */
/*-----------------------------------------------------------------------------------*/
#if UIP_UDP
struct uip_udp_conn *
uip_udp_new(u16_t *ripaddr, u16_t rport)
{
  struct uip_udp_conn *conn;
  
  /* Find an unused local port. */
 again:
  ++lastport;

  if(lastport >= 32000) {
    lastport = 4096;
  }
  
  for(c = 0; c < UIP_UDP_CONNS; ++c) {
    if(uip_udp_conns[c].lport == lastport)
      goto again;
  }


  conn = 0;
  for(c = 0; c < UIP_UDP_CONNS; ++c) {
    if(uip_udp_conns[c].lport == 0) {
      conn = &uip_udp_conns[c]; 
      break;
    }
  }

  if(conn == 0) {
    return 0;
  }
  
  conn->lport = htons(lastport);
  conn->rport = htons(rport);
#if UIP_IPV6
  conn->ripaddr[0] = ripaddr[0];
  conn->ripaddr[1] = ripaddr[1];
  conn->ripaddr[2] = ripaddr[2];
  conn->ripaddr[3] = ripaddr[3];
  conn->ripaddr[4] = ripaddr[4];
  conn->ripaddr[5] = ripaddr[5];
  conn->ripaddr[6] = ripaddr[6];
  conn->ripaddr[7] = ripaddr[7];
#else /* UIP_IPV6 */
  conn->ripaddr[0] = ripaddr[0];
  conn->ripaddr[1] = ripaddr[1];
#endif /* UIP_IPV6 */
  
  return conn;
}
#endif /* UIP_UDP */
/*-----------------------------------------------------------------------------------*/
void
uip_listen(u16_t port)
{
  for(c = 0; c < UIP_LISTENPORTS; ++c) {
    if(uip_listenports[c] == 0) {
      uip_listenports[c] = htons(port);
      break;
    }
  }
}
/*-----------------------------------------------------------------------------------*/
#if UIP_IPV6
/* IPv4 fragment reassembly is not used in IPv6. */
#define UIP_REASSEMBLY 0
#else /* UIP_IPV6 */
#define UIP_REASSEMBLY 0
#endif /* UIP_IPV6 */

#define UIP_REASS_MAXAGE 10

#if UIP_REASSEMBLY
#define UIP_REASS_BUFSIZE (UIP_BUFSIZE - UIP_LLH_LEN)
static u8_t uip_reassbuf[UIP_REASS_BUFSIZE];
static u8_t uip_reassbitmap[UIP_REASS_BUFSIZE / (8 * 8)];
static const u8_t bitmap_bits[8] = {0xff, 0x7f, 0x3f, 0x1f,
				    0x0f, 0x07, 0x03, 0x01};
static u16_t uip_reasslen;
static u8_t uip_reassflags;
#define UIP_REASS_FLAG_LASTFRAG 0x01
static u8_t uip_reasstmr;

#define IP_HLEN 20
#define IP_MF 0x20

static u8_t
uip_reass(void)
{
  u16_t offset, len;
  u16_t i;
  
  /* If ip_reasstmr is zero, no packet is present in the buffer, so we
     write the IP header of the fragment into the reassembly
     buffer. The timer is updated with the maximum age. */
  if(uip_reasstmr == 0) {
    bcopy(&BUF->vhl, uip_reassbuf, IP_HLEN);
    uip_reasstmr = UIP_REASS_MAXAGE;
    uip_reassflags = 0;
    /* Clear the bitmap. */
    bzero(uip_reassbitmap, sizeof(uip_reassbitmap));
  }

  /* Check if the incoming fragment matches the one currently present
     in the reasembly buffer. If so, we proceed with copying the
     fragment into the buffer. */
  if(BUF->srcipaddr[0] == FBUF->srcipaddr[0] &&
     BUF->destipaddr[1] == FBUF->destipaddr[1] &&
     BUF->srcipaddr[0] == FBUF->srcipaddr[0] &&
     BUF->destipaddr[1] == FBUF->destipaddr[1] &&
     BUF->ipid == FBUF->ipid) {
    
    len = (BUF->len[0] << 8) + BUF->len[1] - (BUF->vhl & 0x0f) * 4;
    offset = (((BUF->ipoffset[0] & 0x3f) << 8) + BUF->ipoffset[1]) * 8;

    /* If the offset or the offset + fragment length overflows the
       reassembly buffer, we discard the entire packet. */
    if(offset > UIP_REASS_BUFSIZE ||
       offset + len > UIP_REASS_BUFSIZE) {
      uip_reasstmr = 0;
      goto nullreturn;
    }

    /* Copy the fragment into the reassembly buffer, at the right
       offset. */
    bcopy(BUF + (BUF->vhl & 0x0f) * 4,
	  &uip_reassbuf[IP_HLEN + offset], len);

    /* Update the bitmap. */
    if(offset / (8 * 8) == (offset + len) / (8 * 8)) {
      /* If the two endpoints are in the same byte, we only update
	 that byte. */
      uip_reassbitmap[offset / (8 * 8)] |=
	bitmap_bits[(offset / 8 ) & 7] &
	~bitmap_bits[((offset + len) / 8 ) & 7];
    } else {
      /* If the two endpoints are in different bytes, we update the
	 bytes in the endpoints and fill the stuff inbetween with
	 0xff. */
      uip_reassbitmap[offset / (8 * 8)] |=
	bitmap_bits[(offset / 8 ) & 7];
      for(i = 1 + offset / (8 * 8); i < (offset + len) / (8 * 8); ++i) {
	uip_reassbitmap[i] = 0xff;
      }      
      uip_reassbitmap[(offset + len) / (8 * 8)] |=
	~bitmap_bits[((offset + len) / 8 ) & 7];
    }
    
    /* If this fragment has the More Fragments flag set to zero, we
       know that this is the last fragment, so we can calculate the
       size of the entire packet. We also set the
       IP_REASS_FLAG_LASTFRAG flag to indicate that we have received
       the final fragment. */

    if((BUF->ipoffset[0] & IP_MF) == 0) {
      uip_reassflags |= UIP_REASS_FLAG_LASTFRAG;
      uip_reasslen = offset + len;
    }
    
    /* Finally, we check if we have a full packet in the buffer. We do
       this by checking if we have the last fragment and if all bits
       in the bitmap are set. */
    if(uip_reassflags & UIP_REASS_FLAG_LASTFRAG) {
      /* Check all bytes up to and including all but the last byte in
	 the bitmap. */
      for(i = 0; i < uip_reasslen / (8 * 8) - 1; ++i) {
	if(uip_reassbitmap[i] != 0xff) {
	  goto nullreturn;
	}
      }
      /* Check the last byte in the bitmap. It should contain just the
	 right amount of bits. */
      if(uip_reassbitmap[uip_reasslen / (8 * 8)] !=
	 (u8_t)~bitmap_bits[uip_reasslen / 8 & 7]) {
	goto nullreturn;
      }

      /* If we have come this far, we have a full packet in the
	 buffer, so we allocate a pbuf and copy the packet into it. We
	 also reset the timer. */
      uip_reasstmr = 0;
      bcopy(FBUF, BUF, uip_reasslen);

      /* Pretend to be a "normal" (i.e., not fragmented) IP packet
	 from now on. */
      BUF->ipoffset[0] = BUF->ipoffset[1] = 0;
      BUF->ipchksum = 0;
      BUF->ipchksum = ~(uip_ipchksum());

      

      return uip_reasslen;
    }
  }

 nullreturn:
  return 0;
}
#endif /* IP_REASSEMBLY */
/*-----------------------------------------------------------------------------------*/
void
uip_process(u8_t flag)
{
  register struct uip_conn *uip_connr = uip_conn;
  /*#define uip_connr uip_conn*/
  
  uip_appdata = &uip_buf[40 + UIP_LLH_LEN];

  
  /* Check if we were invoked because of the perodic timer fireing. */
  if(flag == UIP_TIMER) {
    /* Increase the initial sequence number. */
    if(++iss[3] == 0) {
      if(++iss[2] == 0) {
	if(++iss[1] == 0) {
	  ++iss[0];
	}
      }
    }    
    uip_len = 0;
    if(uip_connr->tcpstateflags == TIME_WAIT ||
       uip_connr->tcpstateflags == FIN_WAIT_2) {
      ++(uip_connr->timer);
      if(uip_connr->timer == UIP_TIME_WAIT_TIMEOUT) {
	uip_connr->tcpstateflags = CLOSED;
      }
    } else if(uip_connr->tcpstateflags != CLOSED) {
      /* If the connection has outstanding data, we increase the
	 connection's timer and see if it has reached the RTO value
	 in which case we retransmit. */
      if(uip_outstanding(uip_connr)) {
	--(uip_connr->timer);
	if(uip_connr->timer == 0) {
	  if(uip_connr->nrtx == UIP_MAXRTX ||
	     ((uip_connr->tcpstateflags == SYN_SENT ||
	       uip_connr->tcpstateflags == SYN_RCVD) &&
	      uip_connr->nrtx == UIP_MAXSYNRTX)) {
	    uip_connr->tcpstateflags = CLOSED;

	    /* We call UIP_APPCALL() with uip_flags set to
	       UIP_TIMEDOUT to inform the application that the
	       connection has timed out. */
	    uip_flags = UIP_TIMEDOUT;
	    UIP_APPCALL();

	    /* We also send a reset packet to the remote host. */
	    BUF->flags = TCP_RST | TCP_ACK;
	    goto tcp_send_nodata;
	  }

	  /* Exponential backoff. */
	  uip_connr->timer = UIP_RTO << (uip_connr->nrtx > 4? 4: uip_connr->nrtx);

	  ++(uip_connr->nrtx);
	  
	  /* Ok, so we need to retransmit. We do this differently
	     depending on which state we are in. In ESTABLISHED, we
	     call upon the application so that it may prepare the
	     data for the retransmit. In SYN_RCVD, we resend the
	     SYNACK that we sent earlier and in LAST_ACK we have to
	     retransmit our FINACK. */
	  UIP_STAT(++uip_stat.tcp.rexmit);
	  switch(uip_connr->tcpstateflags & TS_MASK) {
	  case SYN_RCVD:
	    /* In the SYN_RCVD state, we should retransmit our
               SYNACK. */
	    goto tcp_send_synack;
	    
#if UIP_ACTIVE_OPEN
	  case SYN_SENT:
	    /* In the SYN_SENT state, we retransmit out SYN. */
	    BUF->flags = 0;
	    goto tcp_send_syn;
#endif /* UIP_ACTIVE_OPEN */
	    
	  case ESTABLISHED:
	    /* In the ESTABLISHED state, we call upon the application
               to do the actual retransmit after which we jump into
               the code for sending out the packet (the apprexmit
               label). */
	    uip_len = 0;
	    uip_slen = 0;
	    uip_flags = UIP_REXMIT;
	    UIP_APPCALL();
	    goto apprexmit;
	    
	  case FIN_WAIT_1:
	  case CLOSING:
	  case LAST_ACK:
	    /* In all these states we should retransmit a FINACK. */
	    goto tcp_send_finack;
	    
	  }
	}
      } else if((uip_connr->tcpstateflags & TS_MASK) == ESTABLISHED) {
	/* If there was no need for a retransmission, we poll the
           application for new data. */
	uip_len = 0;
	uip_slen = 0;
	uip_flags = UIP_POLL;
	UIP_APPCALL();
	goto appsend;
      }
    }
    goto drop;
  }
#if UIP_UDP 
  if(flag == UIP_UDP_TIMER) {
    if(uip_udp_conn->lport != 0) {
      uip_appdata = &uip_buf[UIP_LLH_LEN + 28];
      uip_len = uip_slen = 0;
      uip_flags = UIP_POLL;
      UIP_UDP_APPCALL();
      goto udp_send;
    } else {
      goto drop;
    }
  }
#endif

  /* This is where the input processing starts. */
  UIP_STAT(++uip_stat.ip.recv);

#if UIP_IPV6

  /* Start of IPv6 input header processing code. */

  /* Check version number in IP header (should be 6) */
  if((BUF->vtc & 0x60) != 0x60) {
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.vhlerr);
    UIP_LOG("ip: invalid version.");
    goto drop;
  }
  
  /* Check packet length. It must be at less than or equal to
     uip_len. If less, the link layer has added a trailer or padding,
     and we set uip_len accordingly. */
  tmpport = (((u16_t)BUF->len[0] << 8) | BUF->len[1]);
  if(tmpport < uip_len) {
    uip_len = tmpport;
  } else if(tmpport > uip_len) {
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.lblenerr);
    UIP_LOG("ip: packet too short.");                               
    goto drop;
  }

  /* Check if the packet is destined for our IP address. */
  for(c = 0; c < 8; ++c) {
    if(BUF->destipaddr[c] != uip_hostaddr[c]) {
      UIP_STAT(++uip_stat.ip.drop);
      UIP_LOG("ip: packet not for us.");        
      goto drop;
    }
  }

  if(BUF->nxthdr == UIP_PROTO_TCP)  /* Check for TCP packet. If so, jump
					to the tcp_input label. */
    goto tcp_input;

  if(BUF->nxthdr != UIP_PROTO_ICMP) { /* We only allow ICMP packets from
				       here. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.protoerr);
    UIP_LOG("ip: neither tcp nor icmp.");        
    goto drop;
  }

  /* Start of (very simplistic!) ICMPv6 input processing code. */
 icmp_input:  
  UIP_STAT(++uip_stat.icmp.recv);
  
  /* ICMP echo (i.e., ping) processing. This is simple, we only change
     the ICMP type from ECHO to ECHO_REPLY and adjust the ICMP
     checksum before we return the packet. */
  if(ICMPBUF->type != ICMP_ECHO) {
    UIP_STAT(++uip_stat.icmp.drop);
    UIP_STAT(++uip_stat.icmp.typeerr);
    UIP_LOG("icmp: not icmp echo.");
    goto drop;
  }

  ICMPBUF->type = ICMP_ECHO_REPLY;
  
  if(ICMPBUF->icmpchksum >= htons(0xffff - (ICMP_ECHO << 8))) {
    ICMPBUF->icmpchksum += htons(ICMP_ECHO << 8) + 1;
  } else {
    ICMPBUF->icmpchksum += htons(ICMP_ECHO << 8);
  }
  
  /* Swap IP addresses. */
  for(c = 0; c < 8; ++c) {
    tmpport = BUF->destipaddr[c];
    BUF->destipaddr[c] = BUF->srcipaddr[c];
    BUF->srcipaddr[c] = tmpport;
  }

  UIP_STAT(++uip_stat.icmp.sent);
  goto send;


  /* End of IPv6 input header processing code. */
     
#else /* UIP_IPV6 */


  /* Start of IPv4 input header processing code. */
  
  /* Check validity of the IP header. */  
  if(BUF->vhl != 0x45)  { /* IP version and header length. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.vhlerr);
    UIP_LOG("ip: invalid version or header length.");
    goto drop;
  }
  
  /* Check the size of the packet. If the size reported to us in
     uip_len doesn't match the size reported in the IP header, there
     has been a transmission error and we drop the packet. */
  
#if UIP_BUFSIZE > 255
  if(BUF->len[0] != (uip_len >> 8)) { /* IP length, high byte. */
    uip_len = (uip_len & 0xff) | (BUF->len[0] << 8);
  }
  if(BUF->len[1] != (uip_len & 0xff)) { /* IP length, low byte. */
    uip_len = (uip_len & 0xff00) | BUF->len[1];
  }
#else
  if(BUF->len[0] != 0) {        /* IP length, high byte. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.hblenerr);
    UIP_LOG("ip: invalid length, high byte.");
    goto drop;
  }
  if(BUF->len[1] != uip_len) {  /* IP length, low byte. */
    uip_len = BUF->len[1];
  }
#endif /* UIP_BUFSIZE > 255 */  

  if(BUF->ipoffset[0] & 0x3f) { /* We don't allow IP fragments. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.fragerr);
    UIP_LOG("ip: fragment dropped.");    
    goto drop;
  }

  /* If we are configured to use ping IP address configuration and
     hasn't been assigned an IP address yet, we accept all ICMP
     packets. */
#if UIP_PINGADDRCONF
  if((uip_hostaddr[0] | uip_hostaddr[1]) == 0) {
    if(BUF->proto == UIP_PROTO_ICMP) {
      UIP_LOG("ip: possible ping config packet received.");
      goto icmp_input;
    } else {
      UIP_LOG("ip: packet dropped since no address assigned..");
      goto drop;
    }
  }
#endif /* UIP_PINGADDRCONF */
  
  /* Check if the packet is destined for our IP address. */  
  if(BUF->destipaddr[0] != uip_hostaddr[0]) {
    UIP_STAT(++uip_stat.ip.drop);
    UIP_LOG("ip: packet not for us.");        
    goto drop;
  }
  if(BUF->destipaddr[1] != uip_hostaddr[1]) {
    UIP_STAT(++uip_stat.ip.drop);
    UIP_LOG("ip: packet not for us.");        
    goto drop;
  }

  if(uip_ipchksum() != 0xffff) { /* Compute and check the IP header
				    checksum. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.chkerr);
    UIP_LOG("ip: bad checksum.");    
    goto drop;
  }

  if(BUF->proto == UIP_PROTO_TCP)  /* Check for TCP packet. If so, jump
                                     to the tcp_input label. */
    goto tcp_input;

#if UIP_UDP
  if(BUF->proto == UIP_PROTO_UDP)
    goto udp_input;
#endif /* UIP_UDP */

  if(BUF->proto != UIP_PROTO_ICMP) { /* We only allow ICMP packets from
					here. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.protoerr);
    UIP_LOG("ip: neither tcp nor icmp.");        
    goto drop;
  }
  
 icmp_input:
  UIP_STAT(++uip_stat.icmp.recv);
  
  /* ICMP echo (i.e., ping) processing. This is simple, we only change
     the ICMP type from ECHO to ECHO_REPLY and adjust the ICMP
     checksum before we return the packet. */
  if(ICMPBUF->type != ICMP_ECHO) {
    UIP_STAT(++uip_stat.icmp.drop);
    UIP_STAT(++uip_stat.icmp.typeerr);
    UIP_LOG("icmp: not icmp echo.");
    goto drop;
  }

  /* If we are configured to use ping IP address assignment, we use
     the destination IP address of this ping packet and assign it to
     ourself. */
#if UIP_PINGADDRCONF
  if((uip_hostaddr[0] | uip_hostaddr[1]) == 0) {
    uip_hostaddr[0] = BUF->destipaddr[0];
    uip_hostaddr[1] = BUF->destipaddr[1];
  }
#endif /* UIP_PINGADDRCONF */  
  
  ICMPBUF->type = ICMP_ECHO_REPLY;
  
  if(ICMPBUF->icmpchksum >= htons(0xffff - (ICMP_ECHO << 8))) {
    ICMPBUF->icmpchksum += htons(ICMP_ECHO << 8) + 1;
  } else {
    ICMPBUF->icmpchksum += htons(ICMP_ECHO << 8);
  }
  
  /* Swap IP addresses. */
  tmpport = BUF->destipaddr[0];
  BUF->destipaddr[0] = BUF->srcipaddr[0];
  BUF->srcipaddr[0] = tmpport;
  tmpport = BUF->destipaddr[1];
  BUF->destipaddr[1] = BUF->srcipaddr[1];
  BUF->srcipaddr[1] = tmpport;

  UIP_STAT(++uip_stat.icmp.sent);
  goto send;

  /* End of IPv4 input header processing code. */
  
#endif /* UIP_IPV6 */

#if UIP_UDP
  /* UDP input processing. */
 udp_input:
  /* UDP processing is really just a hack. We don't do anything to the
     UDP/IP headers, but let the UDP application do all the hard
     work. If the application sets uip_slen, it has a packet to
     send. */
#if UIP_UDP_CHECKSUMS
  if(uip_udpchksum() != 0xffff) { 
    UIP_STAT(++uip_stat.udp.drop);
    UIP_STAT(++uip_stat.udp.chkerr);
    UIP_LOG("udp: bad checksum.");    
    goto drop;
  }  
#endif /* UIP_UDP_CHECKSUMS */

  /* Demultiplex this UDP packet between the UDP "connections". */
  for(uip_udp_conn = &uip_udp_conns[0];
      uip_udp_conn < &uip_udp_conns[UIP_UDP_CONNS];
      ++uip_udp_conn) {
    if(uip_udp_conn->lport != 0 &&
       UDPBUF->destport == uip_udp_conn->lport &&
       (uip_udp_conn->rport == 0 ||
        UDPBUF->srcport == uip_udp_conn->rport) &&
#if UIP_IPV6
       BUF->srcipaddr[0] == uip_connr->ripaddr[0] &&
       BUF->srcipaddr[1] == uip_connr->ripaddr[1] &&
       BUF->srcipaddr[2] == uip_connr->ripaddr[2] &&
       BUF->srcipaddr[3] == uip_connr->ripaddr[3] &&
       BUF->srcipaddr[4] == uip_connr->ripaddr[4] &&
       BUF->srcipaddr[5] == uip_connr->ripaddr[5] &&
       BUF->srcipaddr[6] == uip_connr->ripaddr[6] &&
       BUF->srcipaddr[7] == uip_connr->ripaddr[7]) {
#else /* UIP_IPV6 */
       BUF->srcipaddr[0] == uip_udp_conn->ripaddr[0] &&
       BUF->srcipaddr[1] == uip_udp_conn->ripaddr[1]) {
#endif /* UIP_IPV6 */       
      goto udp_found; 
    }
  }
  goto drop;
  
 udp_found:
  uip_len = uip_len - 28;
  uip_appdata = &uip_buf[UIP_LLH_LEN + 28];
  uip_flags = UIP_NEWDATA;
  uip_slen = 0;
  UIP_UDP_APPCALL();
 udp_send:
  if(uip_slen == 0) {
    goto drop;      
  }
  uip_len = uip_slen + 28;

#if UIP_BUFSIZE > 255
  BUF->len[0] = (uip_len >> 8);
  BUF->len[1] = (uip_len & 0xff);
#else
  BUF->len[0] = 0;
  BUF->len[1] = uip_len;
#endif /* UIP_BUFSIZE > 255 */  
  BUF->proto = UIP_PROTO_UDP;

  UDPBUF->udplen = htons(uip_slen + 8);
  UDPBUF->udpchksum = 0;
#if UIP_UDP_CHECKSUMS 
  /* Calculate UDP checksum. */
  UDPBUF->udpchksum = ~(uip_udpchksum());
  if(UDPBUF->udpchksum == 0) {
    UDPBUF->udpchksum = 0xffff;
  }
#endif /* UIP_UDP_CHECKSUMS */

  BUF->srcport  = uip_udp_conn->lport;
  BUF->destport = uip_udp_conn->rport;

#if UIP_IPV6
  for(c = 0; c < 8; ++c) {
    BUF->srcipaddr[c] = uip_hostaddr[c];    
    BUF->destipaddr[c] = uip_udp_conn->ripaddr[c];
  }
#else /* UIP_IPV6 */
  BUF->srcipaddr[0] = uip_hostaddr[0];
  BUF->srcipaddr[1] = uip_hostaddr[1];
  BUF->destipaddr[0] = uip_udp_conn->ripaddr[0];
  BUF->destipaddr[1] = uip_udp_conn->ripaddr[1];
 
#endif /* UIP_IPV6 */

  uip_appdata = &uip_buf[UIP_LLH_LEN + 40];
  goto ip_send_nolen;
#endif /* UIP_UDP */
  
  /* TCP input processing. */  
 tcp_input:
  UIP_STAT(++uip_stat.tcp.recv);

  /* Start of TCP input header processing code. */
  
  if(uip_tcpchksum() != 0xffff) {   /* Compute and check the TCP
				       checksum. */
    UIP_STAT(++uip_stat.tcp.drop);
    UIP_STAT(++uip_stat.tcp.chkerr);
    UIP_LOG("tcp: bad checksum.");    
    goto drop;
  }
  
  /* Demultiplex this segment. */
  /* First check any active connections. */
  for(uip_connr = &uip_conns[0]; uip_connr < &uip_conns[UIP_CONNS]; ++uip_connr) {
    if(uip_connr->tcpstateflags != CLOSED &&
       BUF->destport == uip_connr->lport &&
       BUF->srcport == uip_connr->rport &&
#if UIP_IPV6
       BUF->srcipaddr[0] == uip_connr->ripaddr[0] &&
       BUF->srcipaddr[1] == uip_connr->ripaddr[1] &&
       BUF->srcipaddr[2] == uip_connr->ripaddr[2] &&
       BUF->srcipaddr[3] == uip_connr->ripaddr[3] &&
       BUF->srcipaddr[4] == uip_connr->ripaddr[4] &&
       BUF->srcipaddr[5] == uip_connr->ripaddr[5] &&
       BUF->srcipaddr[6] == uip_connr->ripaddr[6] &&
       BUF->srcipaddr[7] == uip_connr->ripaddr[7]) {
#else /* UIP_IPV6 */
       BUF->srcipaddr[0] == uip_connr->ripaddr[0] &&
       BUF->srcipaddr[1] == uip_connr->ripaddr[1]) {
#endif /* UIP_IPV6 */       
      goto found;    
    }
  }

  /* If we didn't find and active connection that expected the packet,
     either this packet is an old duplicate, or this is a SYN packet
     destined for a connection in LISTEN. If the SYN flag isn't set,
     it is an old packet and we send a RST. */
  if((BUF->flags & TCP_CTL) != TCP_SYN)
    goto reset;
  
  tmpport = BUF->destport;
  /* Next, check listening connections. */  
  for(c = 0; c < UIP_LISTENPORTS && uip_listenports[c] != 0; ++c) {
    if(tmpport == uip_listenports[c])
      goto found_listen;
  }
  
  /* No matching connection found, so we send a RST packet. */
  UIP_STAT(++uip_stat.tcp.synrst);
 reset:

  /* We do not send resets in response to resets. */
  if(BUF->flags & TCP_RST) 
    goto drop;

  UIP_STAT(++uip_stat.tcp.rst);
  
  BUF->flags = TCP_RST | TCP_ACK;
  uip_len = 40;
  BUF->tcpoffset = 5 << 4;

  /* Flip the seqno and ackno fields in the TCP header. */
  c = BUF->seqno[3];
  BUF->seqno[3] = BUF->ackno[3];  
  BUF->ackno[3] = c;
  
  c = BUF->seqno[2];
  BUF->seqno[2] = BUF->ackno[2];  
  BUF->ackno[2] = c;
  
  c = BUF->seqno[1];
  BUF->seqno[1] = BUF->ackno[1];
  BUF->ackno[1] = c;
  
  c = BUF->seqno[0];
  BUF->seqno[0] = BUF->ackno[0];  
  BUF->ackno[0] = c;

  /* We also have to increase the sequence number we are
     acknowledging. If the least significant byte overflowed, we need
     to propagate the carry to the other bytes as well. */
  if(++BUF->ackno[3] == 0) {
    if(++BUF->ackno[2] == 0) {
      if(++BUF->ackno[1] == 0) {
	++BUF->ackno[0];
      }
    }
  }
 
  /* Swap port numbers. */
  tmpport = BUF->srcport;
  BUF->srcport = BUF->destport;
  BUF->destport = tmpport;
  
  /* Swap IP addresses. */
#if UIP_IPV6
  for(c = 0; c < 8; ++c) {
    tmpport = BUF->destipaddr[c];
    BUF->destipaddr[c] = BUF->srcipaddr[c];
    BUF->srcipaddr[c] = tmpport;
  }
#else /* UIP_IPV6 */
  tmpport = BUF->destipaddr[0];
  BUF->destipaddr[0] = BUF->srcipaddr[0];
  BUF->srcipaddr[0] = tmpport;
  tmpport = BUF->destipaddr[1];
  BUF->destipaddr[1] = BUF->srcipaddr[1];
  BUF->srcipaddr[1] = tmpport;
#endif /* UIP_IPv6 */
  
  /* And send out the RST packet! */
  goto tcp_send_noconn;

  /* This label will be jumped to if we matched the incoming packet
     with a connection in LISTEN. In that case, we should create a new
     connection and send a SYNACK in return. */
 found_listen:
  /* First we check if there are any connections avaliable. Unused
     connections are kept in the same table as used connections, but
     unused ones have the tcpstate set to CLOSED. Also, connections in
     TIME_WAIT are kept track of and we'll use the oldest one if no
     CLOSED connections are found. Thanks to Eddie C. Dost for a very
     nice algorithm for the TIME_WAIT search. */
  uip_connr = 0;
  for(c = 0; c < UIP_CONNS; ++c) {
    if(uip_conns[c].tcpstateflags == CLOSED) {
      uip_connr = &uip_conns[c];
      break;
    }
    if(uip_conns[c].tcpstateflags == TIME_WAIT) {
      if(uip_connr == 0 ||
	 uip_conns[c].timer > uip_connr->timer) {
	uip_connr = &uip_conns[c];
      }
    }
  }

  if(uip_connr == 0) {
    /* All connections are used already, we drop packet and hope that
       the remote end will retransmit the packet at a time when we
       have more spare connections. */
    UIP_STAT(++uip_stat.tcp.syndrop);
    UIP_LOG("tcp: found no unused connections.");
    goto drop;
  }
  uip_conn = uip_connr;
  
  /* Fill in the necessary fields for the new connection. */
  uip_connr->timer = UIP_RTO;
  uip_connr->nrtx = 0;
  uip_connr->lport = BUF->destport;
  uip_connr->rport = BUF->srcport;
#if UIP_IPV6
  for(c = 0; c < 8; ++c) {
    uip_connr->ripaddr[c] = BUF->srcipaddr[c];
  }
#else /* UIP_IPV6 */
  uip_connr->ripaddr[0] = BUF->srcipaddr[0];
  uip_connr->ripaddr[1] = BUF->srcipaddr[1];
#endif /* UIP_IPV6 */
  uip_connr->tcpstateflags = SYN_RCVD;

  uip_connr->snd_nxt[0] = iss[0];
  uip_connr->snd_nxt[1] = iss[1];
  uip_connr->snd_nxt[2] = iss[2];
  uip_connr->snd_nxt[3] = iss[3];
  uip_connr->len = 1;

  /* rcv_nxt should be the seqno from the incoming packet + 1. */
  uip_connr->rcv_nxt[3] = BUF->seqno[3];
  uip_connr->rcv_nxt[2] = BUF->seqno[2];
  uip_connr->rcv_nxt[1] = BUF->seqno[1];
  uip_connr->rcv_nxt[0] = BUF->seqno[0];
  uip_add_rcv_nxt(1);

  /* Parse the TCP MSS option, if present. */
  if((BUF->tcpoffset & 0xf0) > 0x50) {
    for(c = 0; c < ((BUF->tcpoffset >> 4) - 5) << 2 ;) {
      opt = uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + c];
      if(opt == 0x00) {
	/* End of options. */	
	break;
      } else if(opt == 0x01) {
	++c;
	/* NOP option. */
      } else if(opt == 0x02 &&
		uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == 0x04) {
	/* An MSS option with the right option length. */	
	tmpport = (uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 2 + c] << 8) |
	  uip_buf[40 + UIP_LLH_LEN + 3 + c];
	uip_connr->mss = tmpport > UIP_TCP_MSS? UIP_TCP_MSS: tmpport;
	
	/* And we are done processing options. */
	break;
      } else {
	/* All other options have a length field, so that we easily
	   can skip past them. */
	if(uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == 0) {
	  /* If the length field is zero, the options are malformed
	     and we don't process them further. */
	  break;
	}
	c += uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c];
      }      
    }
  }
  
  /* Our response will be a SYNACK. */
#if UIP_ACTIVE_OPEN
 tcp_send_synack:
  BUF->flags = TCP_ACK;    
  
 tcp_send_syn:
  BUF->flags |= TCP_SYN;    
#else /* UIP_ACTIVE_OPEN */
 tcp_send_synack:
  BUF->flags = TCP_SYN | TCP_ACK;    
#endif /* UIP_ACTIVE_OPEN */
  
  /* We send out the TCP Maximum Segment Size option with our
     SYNACK. */
  BUF->optdata[0] = 2;
  BUF->optdata[1] = 4;
  BUF->optdata[2] = (UIP_TCP_MSS) / 256;
  BUF->optdata[3] = (UIP_TCP_MSS) & 255;
  uip_len = 44;
  BUF->tcpoffset = 6 << 4;
  goto tcp_send;

  /* This label will be jumped to if we found an active connection. */
 found:
  uip_conn = uip_connr;
  uip_flags = 0;

  /* We do a very naive form of TCP reset processing; we just accept
     any RST and kill our connection. We should in fact check if the
     sequence number of this reset is wihtin our advertised window
     before we accept the reset. */
  if(BUF->flags & TCP_RST) {
    uip_connr->tcpstateflags = CLOSED;
    UIP_LOG("tcp: got reset, aborting connection.");
    uip_flags = UIP_ABORT;
    UIP_APPCALL();
    goto drop;
  }
  /* All segments that are come thus far should have the ACK flag set,
     otherwise we drop the packet. */
  if(!(BUF->flags & TCP_ACK)) {
    UIP_STAT(++uip_stat.tcp.drop);
    UIP_STAT(++uip_stat.tcp.ackerr);
    UIP_LOG("tcp: dropped non-ack segment.");
    goto drop;
  }
      
  /* Calculated the length of the data, if the application has sent
     any data to us. */
  c = (BUF->tcpoffset >> 4) << 2;
  /* uip_len will contain the length of the actual TCP data. This is
     calculated by subtracing the length of the TCP header (in
     c) and the length of the IP header (20 bytes). */
  uip_len = uip_len - c - 20;

  /* First, check if the sequence number of the incoming packet is
     what we're expecting next. If not, we send out an ACK with the
     correct numbers in. */
  if(uip_len > 0 &&
     (BUF->seqno[0] != uip_connr->rcv_nxt[0] ||
      BUF->seqno[1] != uip_connr->rcv_nxt[1] ||
      BUF->seqno[2] != uip_connr->rcv_nxt[2] ||
      BUF->seqno[3] != uip_connr->rcv_nxt[3])) {
    goto tcp_send_ack;
  }

  /* Next, check if the incoming segment acknowledges any outstanding
     data. If so, we update the sequence number, reset the length of
     the outstanding data, calculate RTT estimations, and reset the
     retransmission timer. */
  if(uip_outstanding(uip_connr)) {
    uip_add32(uip_connr->snd_nxt, uip_connr->len);
    if(BUF->ackno[0] == uip_acc32[0] &&
       BUF->ackno[1] == uip_acc32[1] &&
       BUF->ackno[2] == uip_acc32[2] &&
       BUF->ackno[3] == uip_acc32[3]) {
      /* Update sequence number. */
      uip_connr->snd_nxt[0] = uip_acc32[0];
      uip_connr->snd_nxt[1] = uip_acc32[1];
      uip_connr->snd_nxt[2] = uip_acc32[2];
      uip_connr->snd_nxt[3] = uip_acc32[3];

      /* Do RTT estimation, unless we have done retransmissions. */
      if(uip_connr->nrtx == 0) {
	signed char m;
	m = (UIP_RTO << (uip_connr->nrtx > 4? 4: uip_connr->nrtx)) - uip_connr->timer;
	/* This is taken directly from VJs original code in his paper */
	m = m - (uip_connr->sa >> 3);
	uip_connr->sa += m;
	if(m < 0) {
	  m = -m;
	}
	m = m - (uip_connr->sv >> 2);
	uip_connr->sv += m;
	uip_connr->rto = (uip_connr->sa >> 3) + uip_connr->sv;

      }

      /* Set the acknowledged flag. */
      uip_flags = UIP_ACKDATA;
      /* Reset the length of the outstanding data. */
      uip_connr->len = 0;
      /* Reset the retransmission timer. */
      uip_connr->timer = UIP_RTO;
    }
  }

  /* Do different things depending on in what state the connection is. */
  switch(uip_connr->tcpstateflags & TS_MASK) {
    /* CLOSED and LISTEN are not handled here. CLOSE_WAIT is not
	implemented, since we force the application to close when the
	peer sends a FIN (hence the application goes directly from
	ESTABLISHED to LAST_ACK). */
  case SYN_RCVD:
    /* In SYN_RCVD we have sent out a SYNACK in response to a SYN, and
       we are waiting for an ACK that acknowledges the data we sent
       out the last time. Therefore, we want to have the UIP_ACKDATA
       flag set. If so, we enter the ESTABLISHED state. */
    if(uip_flags & UIP_ACKDATA) {
      uip_connr->tcpstateflags = ESTABLISHED;
      uip_flags = UIP_CONNECTED;
      if(uip_len > 0) {
        uip_flags |= UIP_NEWDATA;
        uip_add_rcv_nxt(uip_len);
      }
      uip_slen = 0;
      UIP_APPCALL();
      goto appsend;
    }
    goto drop;
#if UIP_ACTIVE_OPEN
  case SYN_SENT:
    /* In SYN_SENT, we wait for a SYNACK that is sent in response to
       our SYN. The rcv_nxt is set to sequence number in the SYNACK
       plus one, and we send an ACK. We move into the ESTABLISHED
       state. */
    if((uip_flags & UIP_ACKDATA) &&
       BUF->flags == (TCP_SYN | TCP_ACK)) {

      /* Parse the TCP MSS option, if present. */
      if((BUF->tcpoffset & 0xf0) > 0x50) {
	for(c = 0; c < ((BUF->tcpoffset >> 4) - 5) << 2 ;) {
	  opt = uip_buf[40 + UIP_LLH_LEN + c];
	  if(opt == 0x00) {
	    /* End of options. */	
	    break;
	  } else if(opt == 0x01) {
	    ++c;
	    /* NOP option. */
	  } else if(opt == 0x02 &&
		    uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == 0x04) {
	    /* An MSS option with the right option length. */
	    tmpport = (uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 2 + c] << 8) |
	      uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 3 + c];
	    uip_connr->mss = tmpport > UIP_TCP_MSS? UIP_TCP_MSS: tmpport;

	    /* And we are done processing options. */
	    break;
	  } else {
	    /* All other options have a length field, so that we easily
	       can skip past them. */
	    if(uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == 0) {
	      /* If the length field is zero, the options are malformed
		 and we don't process them further. */
	      break;
	    }
	    c += uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c];
	  }      
	}
      }
      uip_connr->tcpstateflags = ESTABLISHED;      
      uip_connr->rcv_nxt[0] = BUF->seqno[0];
      uip_connr->rcv_nxt[1] = BUF->seqno[1];
      uip_connr->rcv_nxt[2] = BUF->seqno[2];
      uip_connr->rcv_nxt[3] = BUF->seqno[3];
      uip_add_rcv_nxt(1);
      uip_flags = UIP_CONNECTED | UIP_NEWDATA;
      uip_len = 0;
      uip_slen = 0;
      UIP_APPCALL();
      goto appsend;
    }
    goto reset;
#endif /* UIP_ACTIVE_OPEN */
    
  case ESTABLISHED:
    /* In the ESTABLISHED state, we call upon the application to feed
    data into the uip_buf. If the UIP_ACKDATA flag is set, the
    application should put new data into the buffer, otherwise we are
    retransmitting an old segment, and the application should put that
    data into the buffer.

    If the incoming packet is a FIN, we should close the connection on
    this side as well, and we send out a FIN and enter the LAST_ACK
    state. We require that there is no outstanding data; otherwise the
    sequence numbers will be screwed up. */

    if(BUF->flags & TCP_FIN) {
      if(uip_outstanding(uip_connr)) {
	goto drop;
      }
      uip_add_rcv_nxt(1 + uip_len);      
      uip_flags = UIP_CLOSE;
      if(uip_len > 0) {
	uip_flags |= UIP_NEWDATA;
      }
      UIP_APPCALL();
      uip_connr->len = 1;
      uip_connr->tcpstateflags = LAST_ACK;
      uip_connr->nrtx = 0;
    tcp_send_finack:
      BUF->flags = TCP_FIN | TCP_ACK;      
      goto tcp_send_nodata;
    }

    /* Check the URG flag. If this is set, the segment carries urgent
       data that we must pass to the application. */
    if(BUF->flags & TCP_URG) {
#if UIP_URGDATA > 0
      uip_urglen = (BUF->urgp[0] << 8) | BUF->urgp[1];
      if(uip_urglen > uip_len) {
	/* There is more urgent data in the next segment to come. */
	uip_urglen = uip_len;
      }
      uip_add_rcv_nxt(uip_urglen);
      uip_len -= uip_urglen;
      uip_urgdata = uip_appdata;
      uip_appdata += uip_urglen;
    } else {
      uip_urglen = 0;
#endif /* UIP_URGDATA > 0 */
      uip_appdata += (BUF->urgp[0] << 8) | BUF->urgp[1];
      uip_len -= (BUF->urgp[0] << 8) | BUF->urgp[1];
    }
    
    
    /* If uip_len > 0 we have TCP data in the packet, and we flag this
       by setting the UIP_NEWDATA flag and update the sequence number
       we acknowledge. If the application has stopped the dataflow
       using uip_stop(), we must not accept any data packets from the
       remote host. */
    if(uip_len > 0 && !(uip_connr->tcpstateflags & UIP_STOPPED)) {
      uip_flags |= UIP_NEWDATA;
      uip_add_rcv_nxt(uip_len);
    }
    

    /* If this packet constitutes an ACK for outstanding data (flagged
       by the UIP_ACKDATA flag, we should call the application since it
       might want to send more data. If the incoming packet had data
       from the peer (as flagged by the UIP_NEWDATA flag), the
       application must also be notified.

       When the application is called, the global variable uip_len
       contains the length of the incoming data. The application can
       access the incoming data through the global pointer
       uip_appdata, which usually points 40 bytes into the uip_buf
       array.

       If the application wishes to send any data, this data should be
       put into the uip_appdata and the length of the data should be
       put into uip_len. If the application don't have any data to
       send, uip_len must be set to 0. */
    if(uip_flags & (UIP_NEWDATA | UIP_ACKDATA)) {
      uip_slen = 0;
      UIP_APPCALL();

    appsend:
      if(uip_flags & UIP_ABORT) {
	uip_slen = 0;
	uip_connr->tcpstateflags = CLOSED;
	BUF->flags = TCP_RST | TCP_ACK;
	goto tcp_send_nodata;
      }

      if(uip_flags & UIP_CLOSE) {
	uip_slen = 0;
	uip_connr->len = 1;
	uip_connr->tcpstateflags = FIN_WAIT_1;
	uip_connr->nrtx = 0;
	BUF->flags = TCP_FIN | TCP_ACK;
	goto tcp_send_nodata;	
      }

      /* If uip_slen > 0, the application has data to be sent. We
         cannot send data if the application already has outstanding
         data. */
      if(uip_slen > 0 &&
	 !uip_outstanding(uip_connr)) {
	uip_connr->nrtx = 0;
	uip_connr->len = uip_slen;
      } else {	
	uip_slen = 0;
      }
    apprexmit:
      /* If the application has data to be sent, or if the incoming
         packet had new data in it, we must send out a packet. */
      if(uip_slen > 0 || (uip_flags & UIP_NEWDATA)) {
	/* Add the length of the IP and TCP headers. */
	uip_len = uip_connr->len + UIP_TCPIP_HLEN;
	/* We always set the ACK flag in response packets. */
	BUF->flags = TCP_ACK;
	/* Send the packet. */
	goto tcp_send_noopts;
      }
    }
    goto drop;
  case LAST_ACK:
    /* We can close this connection if the peer has acknowledged our
       FIN. This is indicated by the UIP_ACKDATA flag. */     
    if(uip_flags & UIP_ACKDATA) {
      uip_connr->tcpstateflags = CLOSED;
      uip_flags = UIP_CLOSE;
      UIP_APPCALL();
    }
    break;
    
  case FIN_WAIT_1:
    /* The application has closed the connection, but the remote host
       hasn't closed its end yet. Thus we do nothing but wait for a
       FIN from the other side. */
    if(uip_len > 0) {
      uip_add_rcv_nxt(uip_len);
    }
    if(BUF->flags & TCP_FIN) {
      if(uip_flags & UIP_ACKDATA) {
	uip_connr->tcpstateflags = TIME_WAIT;
	uip_connr->timer = 0;
	uip_connr->len = 0;
      } else {
	uip_connr->tcpstateflags = CLOSING;
      }
      uip_add_rcv_nxt(1);
      uip_flags = UIP_CLOSE;
      UIP_APPCALL();
      goto tcp_send_ack;
    } else if(uip_flags & UIP_ACKDATA) {
      uip_connr->tcpstateflags = FIN_WAIT_2;
      uip_connr->len = 0;
      goto drop;
    }
    if(uip_len > 0) {
      goto tcp_send_ack;
    }
    goto drop;
      
  case FIN_WAIT_2:
    if(uip_len > 0) {
      uip_add_rcv_nxt(uip_len);
    }
    if(BUF->flags & TCP_FIN) {
      uip_connr->tcpstateflags = TIME_WAIT;
      uip_connr->timer = 0;
      uip_add_rcv_nxt(1);
      uip_flags = UIP_CLOSE;
      UIP_APPCALL();
      goto tcp_send_ack;
    }
    if(uip_len > 0) {
      goto tcp_send_ack;
    }
    goto drop;

  case TIME_WAIT:
    goto tcp_send_ack;
    
  case CLOSING:
    if(uip_flags & UIP_ACKDATA) {
      uip_connr->tcpstateflags = TIME_WAIT;
      uip_connr->timer = 0;
    }
  }  
  goto drop;
  

  /* We jump here when we are ready to send the packet, and just want
     to set the appropriate TCP sequence numbers in the TCP header. */
 tcp_send_ack:
  BUF->flags = TCP_ACK;
 tcp_send_nodata:
  uip_len = 40;
 tcp_send_noopts:
  BUF->tcpoffset = 5 << 4;
 tcp_send:
  /* We're done with the input processing. We are now ready to send a
     reply. Our job is to fill in all the fields of the TCP and IP
     headers before calculating the checksum and finally send the
     packet. */
  BUF->ackno[0] = uip_connr->rcv_nxt[0];
  BUF->ackno[1] = uip_connr->rcv_nxt[1];
  BUF->ackno[2] = uip_connr->rcv_nxt[2];
  BUF->ackno[3] = uip_connr->rcv_nxt[3];
  
  BUF->seqno[0] = uip_connr->snd_nxt[0];
  BUF->seqno[1] = uip_connr->snd_nxt[1];
  BUF->seqno[2] = uip_connr->snd_nxt[2];
  BUF->seqno[3] = uip_connr->snd_nxt[3];

  BUF->proto = UIP_PROTO_TCP;
  
  BUF->srcport  = uip_connr->lport;
  BUF->destport = uip_connr->rport;

#if UIP_IPV6
  for(c = 0; c < 8; ++c) {
    BUF->srcipaddr[c] = uip_hostaddr[c];    
    BUF->destipaddr[c] = uip_connr->ripaddr[c];
  }
#else /* UIP_IPV6 */
  BUF->srcipaddr[0] = uip_hostaddr[0];
  BUF->srcipaddr[1] = uip_hostaddr[1];
  BUF->destipaddr[0] = uip_connr->ripaddr[0];
  BUF->destipaddr[1] = uip_connr->ripaddr[1];
 
#endif /* UIP_IPV6 */

  if(uip_connr->tcpstateflags & UIP_STOPPED) {
    /* If the connection has issued uip_stop(), we advertise a zero
       window so that the remote host will stop sending data. */
    BUF->wnd[0] = BUF->wnd[1] = 0;
  } else {
#if (UIP_TCP_MSS) > 255
    BUF->wnd[0] = (uip_connr->mss >> 8);
#else
    BUF->wnd[0] = 0;
#endif /* UIP_MSS */
    BUF->wnd[1] = (uip_connr->mss & 0xff); 
  }

 tcp_send_noconn:

#if UIP_BUFSIZE > 255
  BUF->len[0] = (uip_len >> 8);
  BUF->len[1] = (uip_len & 0xff);
#else
  BUF->len[0] = 0;
  BUF->len[1] = uip_len;
#endif /* UIP_BUFSIZE > 255 */

  /* Calculate TCP checksum. */
  BUF->tcpchksum = 0;
  BUF->tcpchksum = ~(uip_tcpchksum());
  
 ip_send_nolen:

#if UIP_IPV6
  BUF->vtc    = 0x60;
  BUF->tcfl   = 0x00;
  BUF->fl     = 0x0000;
  BUF->hoplim = UIP_TTL;
  BUF->nxthdr = UIP_PROTO_TCP;
#else /* UIP_IPV6 */
  BUF->vhl = 0x45;
  BUF->tos = 0;
  BUF->ipoffset[0] = BUF->ipoffset[1] = 0;
  BUF->ttl  = UIP_TTL;
  ++ipid;
  BUF->ipid[0] = ipid >> 8;
  BUF->ipid[1] = ipid & 0xff;
  
  /* Calculate IP checksum. */
  BUF->ipchksum = 0;
  BUF->ipchksum = ~(uip_ipchksum());
#endif /* UIP_IPV6 */

  UIP_STAT(++uip_stat.tcp.sent);
 send:
  UIP_STAT(++uip_stat.ip.sent);
  /* Return and let the caller do the actual transmission. */
  return;
 drop:
  uip_len = 0;
  return;
}
/*-----------------------------------------------------------------------------------*/