This project provides a simple http server to demonstrate RPL neighbourhood.

You will need to run a RPL border router somewhere in your RPL network
in order to access the http server.  Use e.g. ../rpl-border-router.

The http server reuses the code of httpd-simple as found in rpl-border-router,
and shows a single page with available uip6 stack information.

The http server node behaves as a plain simple ipv6 node and simply reacts
to events propagated from the tcpip stack.

The default config of Contiki, plus the following settings in Makefile should
make the ipv6 stack use 6loWPAN header compression.
```
UIP_CONF_IPV6=1
CFLAGS+= -DUIP_CONF_IPV6_RPL
```
