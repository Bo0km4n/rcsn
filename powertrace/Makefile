CONTIKI_PROJECT = example-powertrace
APPS+=powertrace
all: $(CONTIKI_PROJECT)

CONTIKI = ../..
CONTIKI_WITH_RIME = 1
TARGET_LIBFILES += ./lib/libsha1.a ./lib/libcalcbit.a
PROJECT_SOURCEFILES += csn.c dht.c
include $(CONTIKI)/Makefile.include
