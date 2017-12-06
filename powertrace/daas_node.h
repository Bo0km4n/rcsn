#ifndef _DAAS_NODE_H_
#define _DAAS_NODE_H_
#include "./lib/l_bit.h"
typedef struct DaasDHT{
  sha1_hash_t idHash[DEFAULT_HASH_SIZE]; 
} DaasDHT;
typedef struct DaasContext {
  DaasDHT DHT;
} DaasContext;
extern int isEndStructRing;
#endif
