#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/rpl/rpl.h"

#include "net/netstack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "net/uip-debug.h"

/* Use simple webserver with only two pages for minimum footprint.
 * Multiple connections can result in interleaved tcp segments since
 * a single static buffer is used for all segments.
 */
#include "httpd-simple.h"
/* The internal webserver can provide additional information if
 * enough program flash is available.
 */
#define WEBSERVER_CONF_LOADTIME 0
#define WEBSERVER_CONF_FILESTATS 0
#define WEBSERVER_CONF_NEIGHBOR_STATUS 0

static const char *TOP = "<html><head><title>ContikiRPL</title></head><body>\n";
static const char *SCRIPT = "<script src=\"script.js\"></script>\n";
static const char *BOTTOM = "</body></html>\n";
static char buf[512];
static int blen;
#define ADD(...) do {                                                   \
    blen += snprintf(&buf[blen], sizeof(buf) - blen, __VA_ARGS__);      \
  } while(0)

/*---------------------------------------------------------------------------*/
static void
add_ipaddr(const uip_ipaddr_t *addr)
{
  uint16_t a;
  int i, f;
  ADD("<a>");
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) ADD("::");
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
        ADD(":");
      }
      ADD("%x", a);
    }
  }
  ADD("</a>");
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(generate_script(struct httpd_state *s))
{
  PSOCK_BEGIN(&s->sout);
  SEND_STRING(&s->sout, "\
  onload=function() {\
	p=location.host.replace(/::.*/,'::').substr(1);\
	a=document.getElementsByTagName('a');\
	for(i=0;i<a.length;i++) {\
		txt=a[i].innerHTML.replace(/^FE80::/,p);\
		a[i].href='http://['+txt+']';\
	}\
  }");
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(generate_page(struct httpd_state *s))
{
  static uip_ds6_route_t *r;
  static uip_ds6_nbr_t *nbr;
#if WEBSERVER_CONF_LOADTIME
  static clock_time_t numticks;
  numticks = clock_time();
#endif
  static uip_ipaddr_t *preferred_parent_ip;
  { /* assume we have only one instance */
  rpl_dag_t *dag = rpl_get_any_dag();
  preferred_parent_ip = rpl_get_parent_ipaddr(dag->preferred_parent);
  }

  PSOCK_BEGIN(&s->sout);

  SEND_STRING(&s->sout, TOP);
  SEND_STRING(&s->sout, SCRIPT);
  ADD("Neighbors<pre>\n");

  for(nbr = nbr_table_head(ds6_neighbors);
      nbr != NULL;
      nbr = nbr_table_next(ds6_neighbors, nbr)) {

      add_ipaddr(&nbr->ipaddr);

#if WEBSERVER_CONF_NEIGHBOR_STATUS
{uint8_t j=blen+25;
      while (blen < j) ADD(" ");
      switch (nbr->state) {
      case NBR_INCOMPLETE: ADD(" INCOMPLETE");break;
      case NBR_REACHABLE: ADD(" REACHABLE");break;
      case NBR_STALE: ADD(" STALE");break;      
      case NBR_DELAY: ADD(" DELAY");break;
      case NBR_PROBE: ADD(" NBR_PROBE");break;
      }
}
#endif

      if (uip_ipaddr_cmp(&nbr->ipaddr, preferred_parent_ip))
        ADD(" PREFERRED");
      ADD("\n");
      if(blen > sizeof(buf) - 45) {
        SEND_STRING(&s->sout, buf);
        blen = 0;
      }
  }
{
  ADD("</pre>\nDefault Route<pre>\n");
  SEND_STRING(&s->sout, buf);
  blen = 0;
  add_ipaddr(uip_ds6_defrt_choose());
  ADD("\n");
}
  ADD("</pre>\nRoutes<pre>\n");
  SEND_STRING(&s->sout, buf);
  blen = 0;

  for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {

    add_ipaddr(&r->ipaddr);
    ADD("/%u (via ", r->length);
    add_ipaddr(uip_ds6_route_nexthop(r));
    if(1 || (r->state.lifetime < 600)) {
      ADD(") %us\n", (unsigned int)r->state.lifetime);
    } else {
      ADD(")\n");
    }
    SEND_STRING(&s->sout, buf);
    blen = 0;
  }
  ADD("</pre>\n");

#if WEBSERVER_CONF_FILESTATS
  static uint16_t numtimes;
  ADD("<br><i>This page sent %u times</i>",++numtimes);
#endif

#if WEBSERVER_CONF_LOADTIME
  numticks = clock_time() - numticks + 1;
  ADD(" <i>(%u.%02u sec)</i>",numticks/CLOCK_SECOND,(100*(numticks%CLOCK_SECOND))/CLOCK_SECOND);
#endif

  SEND_STRING(&s->sout, buf);
  SEND_STRING(&s->sout, BOTTOM);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
httpd_simple_script_t
httpd_simple_get_script(const char *name)
{
  if (!strcmp("script.js", name))
    return generate_script;
  else
    return generate_page;
}

/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  printf("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      printf(" ");
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }
}

PROCESS(webserver_process, "Web server");
PROCESS_THREAD(webserver_process, ev, data)
{
  PROCESS_BEGIN();

  print_local_addresses();
  httpd_init();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    httpd_appcall(data);
  }
  
  PROCESS_END();
}
AUTOSTART_PROCESSES(&webserver_process);

