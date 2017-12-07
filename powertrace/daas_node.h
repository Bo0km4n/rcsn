#ifndef _DAAS_NODE_H_
#define _DAAS_NODE_H_
#include "./lib/l_bit.h"
#define MAX_NODE 4
#define ALL_HEAD_ID 1
#define DELAY_CLOCK 2
#define RAND_MARGIN 10
#define RSSI -9999
#define RETRY_LIMIT 3
typedef struct DaasDHT{
  sha1_hash_t idHash[DEFAULT_HASH_SIZE]; 
} DaasDHT;
typedef struct DaasContext {
  DaasDHT DHT;
} DaasContext;
extern int isEndStructRing;
#endif
