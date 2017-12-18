#ifndef _WL_MOTE_H_
#define _WL_MOTE_H_
#include "l_bit.h"
#include "contiki.h"
#include "contiki-lib.h"
#include "net/rime/rime.h"
#include "white_list.h"

#define KEY_LIST_LEN 16
typedef struct WhiteListMessage {
    int Type;
    sha1_hash_t Body;
} WhiteListMessage;

typedef struct WhiteList {
    int Cursor;
    sha1_hash_t Keys[KEY_LIST_LEN];
    WhiteListMessage *M;
    struct multihop_conn *Multihop;
} WhiteListMote;

enum WLMessageTypes {
  Publish,
  Update,
  Delete
};

void WhiteListMoteInit(void);
void WhiteListUCRecv(struct unicast_conn *, const linkaddr_t *);
void InsertWLMessage(WhiteListMessage *, sha1_hash_t *);
void WLSendUCPacket(WhiteListMessage *, int);
void PublishHandle(sha1_hash_t *);
void StoreKey(sha1_hash_t *);
void PublishKey(sha1_hash_t *, int);
extern WhiteListMote whiteListMote;
#endif