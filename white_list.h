#ifndef _WL_H_
#define _WL_H_
#include "l_bit.h"
#include "contiki.h"
#include "contiki-lib.h"
#include "net/rime/rime.h"
#include "white_list_mote.h"

#define WHITE_LIST_LEN 5
void Pub(sha1_hash_t *, int);
void WhiteListInit(sha1_hash_t *);
void debugServerWL(sha1_hash_t *);
void WhiteListServerRecv(struct unicast_conn *, const linkaddr_t *);
void WhiteListServerBCRecv(struct broadcast_conn *, const linkaddr_t *);
#endif