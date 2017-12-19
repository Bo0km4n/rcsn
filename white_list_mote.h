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
typedef struct Query {
    int Publisher;
    sha1_hash_t Body;
} Query;
typedef struct Result {
    int Dest;
    int IsExist;
    sha1_hash_t Body;
} Result;

typedef struct WhiteList {
    int Cursor;
    sha1_hash_t Keys[KEY_LIST_LEN];
    int Switch;
    WhiteListMessage *M;
    Query *Q;
    Result *R;
    struct multihop_conn *Multihop;
    struct multihop_conn *QMultihop;
    void (*SwitchOn) (void);
    void (*SwitchOff) (void);
} WhiteListMote;

enum WLMessageTypes {
  Publish,
  Search,
  Update,
  Delete
};

void WhiteListMoteInit(void);
void WhiteListUCRecv(struct unicast_conn *, const linkaddr_t *);
void WhiteListBCRecv(struct broadcast_conn *, const linkaddr_t *);
void SearchUCRecv(struct unicast_conn *,const linkaddr_t *);
void InsertWLMessage(WhiteListMessage *, sha1_hash_t *);
void WLSendUCPacket(WhiteListMessage *, int);
void QuerySendUCPacket(Query *, int);
void QueryPublish(Query *);
void PublishHandle(sha1_hash_t *);
void StoreKey(sha1_hash_t *);
void PublishKey(sha1_hash_t *, int);
void StartRandomSearch();
void switchOn();
void switchOff();
void HashRandomization(sha1_hash_t *);
int CheckRange(sha1_hash_t *);
int CheckChildRange(sha1_hash_t *);
extern WhiteListMote whiteListMote;
#endif