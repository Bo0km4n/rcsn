CONTIKI_PROJECT = example-powertrace
APPS+=powertrace
all: $(CONTIKI_PROJECT)

CONTIKI = ../..
CONTIKI_WITH_RIME = 1
PROJECT_SOURCEFILES += csn.c dht.c sha1.c l_bit.c white_list_mote.c
include $(CONTIKI)/Makefile.include
