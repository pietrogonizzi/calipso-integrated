CONTIKI_PROJECT=http-server
all: $(CONTIKI_PROJECT) 

CONTIKI=../../..

UIP_CONF_IPV6=1
CFLAGS+= -DUIP_CONF_IPV6_RPL

#linker optimizations
SMALL=1

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += httpd-simple.c

include $(CONTIKI)/Makefile.include
