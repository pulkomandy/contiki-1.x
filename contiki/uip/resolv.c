/*
 * Copyright (c) 2002, Adam Dunkels.
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
 * $Id: resolv.c,v 1.1 2003/03/19 14:16:06 adamdunkels Exp $
 *
 */

#include "resolv.h"
#include "dispatcher.h"

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

#define MAX_RETRIES 8

struct dns_hdr {
  u16_t id;
  u8_t flags1, flags2;
#define DNS_FLAG1_RESPONSE        0x80
#define DNS_FLAG1_OPCODE_STATUS   0x10
#define DNS_FLAG1_OPCODE_INVERSE  0x08
#define DNS_FLAG1_OPCODE_STANDARD 0x00
#define DNS_FLAG1_AUTHORATIVE     0x04
#define DNS_FLAG1_TRUNC           0x02
#define DNS_FLAG1_RD              0x01
#define DNS_FLAG2_RA              0x80
#define DNS_FLAG2_ERR_MASK        0x0f
#define DNS_FLAG2_ERR_NONE        0x00
#define DNS_FLAG2_ERR_NAME        0x03
  u16_t numquestions;
  u16_t numanswers;
  u16_t numauthrr;
  u16_t numextrarr;
};

struct dns_answer {
  /* DNS answer record starts with either a domain name or a pointer
     to a name already present somewhere in the packet. */
  u16_t type;
  u16_t class;
  u16_t ttl[2];
  u16_t len;
  u16_t ipaddr[2];
};


#define STATE_UNUSED 0
#define STATE_NEW    1
#define STATE_ASKING 2
#define STATE_DONE   3
#define STATE_ERROR  4
struct namemap {
  u8_t state;
  u8_t tmr;
  u8_t retries;
  u8_t seqno;
  u8_t err;
  char name[64];
  u16_t ipaddr[2];
};

#define RESOLV_ENTRIES 8

static struct namemap names[RESOLV_ENTRIES];

static u8_t seqno;

static struct uip_udp_conn *resolv_conn = NULL;

ek_signal_t resolv_signal_found = EK_SIGNAL_NONE;

/*-----------------------------------------------------------------------------------*/
/* parse_name(name):
 *
 * Returns the end of the name.
 */
static unsigned char *
parse_name(unsigned char *query)
{
  unsigned char n;

  do {
    n = *query++;
    
    while(n > 0) {
      /*      printf("%c", *query);*/
      ++query;
      --n;
    };
    /*    printf(".");*/
  } while(*query != 0);
  /*  printf("\n");*/
  return query + 1;
}
/*-----------------------------------------------------------------------------------*/
/* check_entries(void):
 *
 * Runs through the list of names to see if there are any that have
 * not been queried yet. If so, a query is sent out.
 */
static void
check_entries(void)
{
  struct dns_hdr *hdr;
  char *query, *nptr, *nameptr;
  u8_t i;
  u8_t n;

  for(i = 0; i < RESOLV_ENTRIES; ++i) {

    if(names[i].state == STATE_NEW ||
       names[i].state == STATE_ASKING) {
      if(names[i].state == STATE_ASKING) {
	--names[i].tmr;
	if(names[i].tmr == 0) {
	  ++names[i].retries;
	  if(names[i].retries == MAX_RETRIES) {
	    names[i].state = STATE_ERROR;
	    resolv_found(names[i].name, NULL);
	    continue;
	  }
	  names[i].tmr = names[i].retries;	  
	} else {
	  /*	  printf("Timer %d\n", names[i].tmr);*/
	  /* Its timer has not run out, so we move on to next
	     entry. */
	  continue;
	}
      } else {
	names[i].state = STATE_ASKING;
	names[i].tmr = 1;
	names[i].retries = 0;
      }
      hdr = (struct dns_hdr *)uip_appdata;
      hdr->id = htons(i);
      hdr->flags1 = DNS_FLAG1_RD;
      hdr->flags2 = 0;
      hdr->numquestions = htons(1);
      hdr->numanswers = hdr->numauthrr = hdr->numextrarr = 0;
      query = (char *)uip_appdata + 12;
      nameptr = names[i].name;
      --nameptr;
      /* Convert hostname into suitable query format. */
      do {
	++nameptr;
	nptr = query;
	++query;
	for(n = 0; *nameptr != '.' && *nameptr != 0; ++nameptr) {
	  *query = *nameptr;
	  ++query;
	  ++n;
	}
	*nptr = n;
      } while(*nameptr != 0);
      nptr = query;
      *nptr = 0; /* End of query name. */
      ++nptr;
      *nptr = 0; /* High byte of query type. */
      ++nptr;
      *nptr = 1; /* Low byte of query type. 1 == IP address query. */
      ++nptr;
      *nptr = 0; /* High byte of query class. */
      ++nptr;
      *nptr = 1; /* Low byte of query class. */
      ++nptr;
      uip_udp_send((unsigned char)(nptr - (char *)uip_appdata));
      break;
    }
  }
}
/*-----------------------------------------------------------------------------------*/
static void
newdata(void)
{
  char *nameptr;
  struct dns_answer *ans;
  struct dns_hdr *hdr;
  u8_t nquestions, nanswers;
  u8_t i;
  
  hdr = (struct dns_hdr *)uip_appdata;
  /*  printf("ID %d\n", htons(hdr->id));
  printf("Query %d\n", hdr->flags1 & DNS_FLAG1_RESPONSE);
  printf("Error %d\n", hdr->flags2 & DNS_FLAG2_ERR_MASK);
  printf("Num questions %d, answers %d, authrr %d, extrarr %d\n",
	 htons(hdr->numquestions),
	 htons(hdr->numanswers),
	 htons(hdr->numauthrr),
	 htons(hdr->numextrarr));
  */

  /* The ID in the DNS header should be our entry into the name
     table. */
  i = htons(hdr->id); 
  if(i < RESOLV_ENTRIES &&
     names[i].state == STATE_ASKING) {

    /* This entry is now finished. */
    names[i].state = STATE_DONE;
    names[i].err = hdr->flags2 & DNS_FLAG2_ERR_MASK;

    /* Check for error. If so, call callback to inform. */
    if(names[i].err != 0) {
      names[i].state = STATE_ERROR;
      resolv_found(names[i].name, NULL);
      return;
    }

    /* We only care about the question(s) and the answers. The authrr
       and the extrarr are simply discarded. */
    nquestions = htons(hdr->numquestions);
    nanswers = htons(hdr->numanswers);

    /* Skip the name in the question. XXX: This should really be
       checked agains the name in the question, to be sure that they
       match. */
    nameptr = parse_name((char *)uip_appdata + 12) + 4;

    while(nanswers > 0) {
      /* The first byte in the answer resource record determines if it
	 is a compressed record or a normal one. */
      if(*nameptr & 0xc0) {       
	/* Compressed name. */
	nameptr +=2;
	/*	printf("Compressed anwser\n");*/
      } else {
	/* Not compressed name. */
	nameptr = parse_name((char *)nameptr);
      }

      ans = (struct dns_answer *)nameptr;
      /*      printf("Answer: type %x, class %x, ttl %x, length %x\n",
	     htons(ans->type), htons(ans->class),
	     (htons(ans->ttl[0]) << 16) | htons(ans->ttl[1]),
	     htons(ans->len));*/

      /* Check for IP address type and Internet class. Others are
	 discarded. */
      if(ans->type == htons(1) &&
	 ans->class == htons(1) &&
	 ans->len == htons(4)) {
	/*	printf("IP address %d.%d.%d.%d\n",
	       htons(ans->ipaddr[0]) >> 8,
	       htons(ans->ipaddr[0]) & 0xff,
	       htons(ans->ipaddr[1]) >> 8,
	       htons(ans->ipaddr[1]) & 0xff);*/
	/* XXX: we should really check that this IP address is the one
	   we want. */
	names[i].ipaddr[0] = ans->ipaddr[0];
	names[i].ipaddr[1] = ans->ipaddr[1];
	resolv_found(names[i].name, names[i].ipaddr);
	return;
      } else {
	nameptr = nameptr + 10 + htons(ans->len);
      }
      --nanswers;
    }
  }

}
/*-----------------------------------------------------------------------------------*/
/* udp_appcall():
 *
 * The main UDP function.
 */
void
udp_appcall(void)
{
  if(uip_udp_conn->rport == htons(53)) {
    if(uip_poll()) {
      check_entries();
    }
    if(uip_newdata()) {
      newdata();
    }       
  }
}
/*-----------------------------------------------------------------------------------*/
/* resolv_query(name):
 *
 * Queues a name so that a question for the name will be sent out the
 * next time the udp_appcall is polled.
 */
void
resolv_query(char *name)
{
  u8_t i;
  u8_t lseq, lseqi;

  lseq = lseqi = 0;
  
  for(i = 0; i < RESOLV_ENTRIES; ++i) {
    if(names[i].state == STATE_UNUSED) {
      break;
    }
    if(seqno - names[i].seqno > lseq) {
      lseq = seqno - names[i].seqno;
      lseqi = i;
    }
  }

  if(i == RESOLV_ENTRIES) {
    i = lseqi;
  }

  /*  printf("Using entry %d\n", i);*/

  strcpy(names[i].name, name);
  names[i].state = STATE_NEW;
  names[i].seqno = seqno;
  ++seqno;

}
/*-----------------------------------------------------------------------------------*/
u16_t *
resolv_lookup(char *name)
{
  u8_t i;

  /* Walk through the list to see if the name is in there. If it is
     not, we return NULL. */
  for(i = 0; i < RESOLV_ENTRIES; ++i) {
    if(names[i].state == STATE_DONE &&
       strcmp(name, names[i].name) == 0) {
      return names[i].ipaddr;
    }
  }
  return NULL;
}  
/*-----------------------------------------------------------------------------------*/
void
resolv_conf(u16_t *dnsserver)
{
  if(resolv_conn != NULL) {
    uip_udp_remove(resolv_conn);
  }
  
  resolv_conn = uip_udp_new(dnsserver, 53);

}
/*-----------------------------------------------------------------------------------*/
void
resolv_init(void)
{
  u8_t i;

  for(i = 0; i < RESOLV_ENTRIES; ++i) {
    names[i].state = STATE_DONE;
  }

  resolv_signal_found = dispatcher_sigalloc();
}
/*-----------------------------------------------------------------------------------*/
void
resolv_found(char *name, u16_t *ipaddr)
{
  dispatcher_emit(resolv_signal_found, name, DISPATCHER_BROADCAST);
}