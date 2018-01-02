#ifndef _WL_MOTE_H_
#define _WL_MOTE_H_
#include "l_bit.h"
#include "contiki.h"
#include "contiki-lib.h"
#include "net/rime/rime.h"
#include "white_list.h"

#define KEY_LIST_LEN 16
#define QUEUE_SIZE 32
#define ReffererLength 32
typedef struct WhiteListMessage {
    int Type;
    sha1_hash_t Body;
} WhiteListMessage;
typedef struct Query {
    short int Publisher;
    short int Refferer[ReffererLength];
    short int ReffererIndex;
    short int Next;
    sha1_hash_t Body;
} Query;
typedef struct Result {
    short int Dest;
    short int IsExist;
    short int Refferer[ReffererLength];
    short int ReffererIndex;
    short int Next;
    sha1_hash_t Body;
} Result;
typedef struct ResultQueue {
    Result Data[QUEUE_SIZE];
    void (*Enqueue) (struct ResultQueue *, Result *);
    Result (*Dequeue) (struct ResultQueue *);
    int Cursor;
} ResultQueue;

typedef struct WhiteList {
    int Cursor;
    sha1_hash_t Keys[KEY_LIST_LEN];
    int Switch;
    WhiteListMessage *M;
    Query *Q;
    Result *R;
    ResultQueue *ResultQueue;
    struct multihop_conn *Multihop;
    struct multihop_conn *RMultihop;
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
void ResultUCRecv(struct unicast_conn *,const linkaddr_t *);
void resultForward(Result *, int);
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
int ScanWhiteList(sha1_hash_t *);
int ScanCache(sha1_hash_t *);
int EqualHash(sha1_hash_t *, sha1_hash_t *);
void ResultSendMHPacket(Result *);
extern WhiteListMote whiteListMote;
void Enqueue(ResultQueue *, Result *);
Result Dequeue(ResultQueue *);
int Empty(ResultQueue *);
void StoreRefferer(Query *, short int);
void ReplyResult(Query *, Result *);
void ResultSendUCPacket(Result *, int);
#endif