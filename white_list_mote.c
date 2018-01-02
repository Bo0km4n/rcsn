#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "daas_node.h"
#include "csn.h"
#include "dht.h"
#include "white_list_mote.h"
#include "config.h"
#include <stdio.h>
#include "sys/node-id.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "dev/button-sensor.h"

static struct unicast_conn whiteListUC;
static struct unicast_conn searchUC;
static struct unicast_callbacks whiteListUCCallBacks = {WhiteListUCRecv};
static struct unicast_callbacks searchUCCallBacks = {SearchUCRecv};
static struct broadcast_conn whiteListBC;
static struct broadcast_callbacks whiteListBCCallBacks = {WhiteListBCRecv};
static struct etimer et;
WhiteListMote whiteListMote;

/*---------------------------------------------------------------------------*/
/* random search hash id  */
PROCESS(randomHashSearchProcess, "random search hash id process");
PROCESS_THREAD(randomHashSearchProcess, ev, data)
{
  PROCESS_BEGIN();
  printf("[WL:DEBUG] start random search hash process\n");
  while(whiteListMote.Switch) {
      if (csn.ID != ALL_HEAD_ID) break; // for debug
      etimer_set(&et, CLOCK_SECOND * (10 + (random_rand() % 60)));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      HashRandomization(&whiteListMote.Q->Body);
      PrintHash(&whiteListMote.Q->Body);
      QueryPublish(whiteListMote.Q);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

void WhiteListMoteInit() {
    unicast_open(&whiteListUC, WL_UC_PORT, &whiteListUCCallBacks);
    unicast_open(&searchUC, WL_SEARCH_PORT, &searchUCCallBacks);
    broadcast_open(&whiteListBC, WL_BC_PORT, &whiteListBCCallBacks);
    whiteListMote.Cursor = 0;
    whiteListMote.Switch = 0;
    whiteListMote.M = (WhiteListMessage *)malloc(sizeof(WhiteListMessage));
    whiteListMote.Q = (Query *)malloc(sizeof(Query));
    whiteListMote.R = (Result *)malloc(sizeof(Result));
    whiteListMote.ResultQueue = (ResultQueue *)malloc(sizeof(ResultQueue));
    whiteListMote.ResultQueue->Enqueue = Enqueue;
    whiteListMote.ResultQueue->Dequeue = Dequeue;
    whiteListMote.SwitchOn = switchOn;
    whiteListMote.SwitchOff = switchOff;
    printf("[WL:DEBUG] white list initilized\n");
    return;
}
void WhiteListUCRecv(struct unicast_conn *uc, const linkaddr_t *from) {
    WhiteListMessage *m = (WhiteListMessage *)packetbuf_dataptr();
    switch (m->Type) {
        case Publish:
        printf("[WL:DEBUG] received publish from %d\n", from->u8[0]);
        PrintHash(&m->Body);
        if (csn.ID == ALL_HEAD_ID) {
            InsertWLMessage(whiteListMote.M, &m->Body);
            WLSendUCPacket(whiteListMote.M, csn.Successor);
        } else {
            PublishHandle(&m->Body);
        }
        break;
        default:
        break;
    }
}
void WhiteListBCRecv(struct broadcast_conn *bc, const linkaddr_t *from) {
    printf("[WL:DEBUG] received switch on message\n");
    if (!whiteListMote.Switch) {
        whiteListMote.SwitchOn();
        broadcast_send(bc);
        StartRandomSearch();
    }
}
void SearchUCRecv(struct unicast_conn *uc, const linkaddr_t *from) {
    Query *q = (Query *)packetbuf_dataptr();
    if (q->Next != csn.ID) {
        QuerySendUCPacket(q, csn.Previous);
        return;
    }
    if (csn.Level == 1) {
        int index = ScanCache(&q->Body);
        if (index >= 0) {
            whiteListMote.R->IsExist = whiteListMote.ResultQueue->Data[index].IsExist;
            DhtCopy(&whiteListMote.ResultQueue->Data[index].Body, &whiteListMote.R->Body);
            whiteListMote.R->Dest = q->Publisher;
            ResultSendMHPacket(whiteListMote.R);
            return;
        }
    }

    if (csn.IsBot) {
        if (CheckRange(&q->Body)) {
            whiteListMote.R->IsExist = ScanWhiteList(&q->Body);
            DhtCopy(&q->Body, &whiteListMote.R->Body);
            whiteListMote.R->Dest = q->Publisher;
            ResultSendMHPacket(whiteListMote.R);
        } else if (csn.MaxRingNode > 2) {
            QuerySendUCPacket(q, csn.Successor);
        } else {
            printf("[WL:DEBUG] query error\n");
        }
    } else {
        if (CheckRange(&q->Body)) {
            if (CheckChildRange(&q->Body)) {
                whiteListMote.R->IsExist = ScanWhiteList(&q->Body);
                DhtCopy(&q->Body, &whiteListMote.R->Body);
                whiteListMote.R->Dest = q->Publisher;
                ResultSendMHPacket(whiteListMote.R);
                return;
            } else {
                QuerySendUCPacket(q, csn.ChildSuccessor);
                return;
            }
        } else {
            QuerySendUCPacket(q, csn.Successor);
            return;
        }
    }
}
void WLSendUCPacket(WhiteListMessage *m, int id) {
    linkaddr_t to;
    to.u8[0] = id;
    to.u8[1] = 0;
    packetbuf_copyfrom(m, 64);
    int delay = 0;
    if (csn.ID <= 10) delay = csn.ID;
    if (csn.ID >= 10 && csn.ID < 100) delay = csn.ID % 10;
    if (csn.ID >= 100) delay = csn.ID % 100;
    clock_wait(DELAY_CLOCK + random_rand() % delay);
    if (csn.IsRingTail && id == csn.Successor) {
        multihop_send(whiteListMote.Multihop, &to);
    } else {
        unicast_send(&whiteListUC, &to);
    }
}
void QuerySendUCPacket(Query *q, int id) {
    linkaddr_t to;
    q->Next = id;
    packetbuf_copyfrom(q, 100);
    int delay = 0;
    if (csn.ID <= 10) delay = csn.ID;
    if (csn.ID >= 10 && csn.ID < 100) delay = csn.ID % 10;
    if (csn.ID >= 100) delay = csn.ID % 100;
    clock_wait(DELAY_CLOCK + random_rand() % delay);
    if (csn.IsRingTail && id == csn.Successor) {
        //multihop_send(whiteListMote.QMultihop, &to);
        to.u8[0] = csn.Previous;
    } else {
        to.u8[0] = id;
    }
    to.u8[1] = 0;
    unicast_send(&searchUC, &to);
}

void ResultSendMHPacket(Result *r) {
    linkaddr_t to;
    to.u8[0] = r->Dest;
    to.u8[1] = 0;
    packetbuf_copyfrom(r, 64);
    multihop_send(whiteListMote.RMultihop, &to);
}

void InsertWLMessage(WhiteListMessage *m, sha1_hash_t *body) {
    DhtCopy(body, &m->Body);
}

void PublishHandle(sha1_hash_t *body) {
    if (csn.IsBot) {
        if (CheckRange(body)) {
            StoreKey(body);
        } else if (csn.MaxRingNode > 2) {
            PublishKey(body, csn.Successor);
        } else {
            printf("[WL:DEBUG] publish query error\n");
        }
    } else {
        if (CheckRange(body)) {
            if (CheckChildRange(body)) {
                StoreKey(body);
            } else {
                PublishKey(body, csn.ChildSuccessor);
            }
        } else {
            PublishKey(body, csn.Successor);
        }
    }
}

void StoreKey(sha1_hash_t *key) {
    int i = 0;
    if (whiteListMote.Cursor >= KEY_LIST_LEN) return;
    DhtCopy(key, &whiteListMote.Keys[whiteListMote.Cursor]);
    whiteListMote.Cursor += 1;
    printf("[WL:DEBUG] stored key || ");
    for (i=DEFAULT_HASH_SIZE-1;i>=0;i--) {
        printf("%02x ", key->hash[i]);
    }
    printf("\n");
    return;
}

void PublishKey(sha1_hash_t *key, int id) {
    InsertWLMessage(whiteListMote.M, key);
    WLSendUCPacket(whiteListMote.M, id);
}

void StartRandomSearch() {
    process_start(&randomHashSearchProcess, (void *)0);
}

void switchOn() {
    whiteListMote.Switch = 1;
}
void switchOff() {
    whiteListMote.Switch = 0;
}
void HashRandomization(sha1_hash_t *h) {
    int i=0;
    for(i=0;i<DEFAULT_HASH_SIZE;i++) {
        h->hash[i] = (uint8_t)(random_rand() % 255);
    }
}
void QueryPublish(Query *q) {
    q->Publisher = csn.ID;
    if (csn.IsBot) {
        if (CheckRange(&q->Body)) {
            whiteListMote.R->IsExist = ScanWhiteList(&q->Body);
            printf("[WL:DEBUG] result : %d", whiteListMote.R->IsExist);
        } else {
            QuerySendUCPacket(q, csn.Successor);
        }
    } else {
        if (CheckRange(&q->Body)) {
            if (CheckChildRange(&q->Body)) {
                whiteListMote.R->IsExist = ScanWhiteList(&q->Body);
            } else {
                //StoreRefferer(q, csn.ID);
                QuerySendUCPacket(q, csn.ChildSuccessor);
            }
        } else {
            //StoreRefferer(q, csn.ID);
            QuerySendUCPacket(q, csn.Successor);
        }
    }
}
int CheckRange(sha1_hash_t *key) {
    // 自分のリングの範囲内でない return 0
    // ! MinID <= key <= MaxID
    if (!sha1Comp(dht.MaxID, key) && !sha1Comp(key, dht.MinID)) return 1;
    return 0;
}
int CheckChildRange(sha1_hash_t *key) {
    // 子供のリングの範囲内でない場合 return 0
    // ! ChildMinID <= key <= ChildMaxID
    if (!sha1Comp(dht.ChildMaxID, key) && !sha1Comp(key, dht.ChildMinID)) return 1;
    return 0;
}
int ScanWhiteList(sha1_hash_t *key) {
    int i=0;
    for (i=0;i<KEY_LIST_LEN;i++) {
        if (EqualHash(key, &whiteListMote.Keys[i])) {
            printf("[WL:DEBUG] key hit!!\n");
            return 1;
        }
    }
    printf("[WL:DEBUG] key not hit...\n");
    return 0;
}
int ScanCache(sha1_hash_t *key) {
    int i=0;
    for (i=0;i<16;i++) {
        if (EqualHash(key, &whiteListMote.ResultQueue->Data[i].Body)) {
            printf("[WL:DEBUG] cache hit!!\n");
            return i;
        }
    }
    printf("[WL:DEBUG] cache not hit...\n");
    return -1;
}
int EqualHash(sha1_hash_t *a, sha1_hash_t *b) {
    int i;
    for (i=0;i<DEFAULT_HASH_SIZE;i++) {
        if (a->hash[i] != b->hash[i]) return 0;
    }
    return 1;
}
int next(int a) {
    return (a+1) % QUEUE_SIZE;
} 
void Enqueue(ResultQueue *rQueue, Result *r) {
    int i = 0;

    if (next(rQueue->Cursor) == QUEUE_SIZE-1) {
        Dequeue(rQueue);
        rQueue->Data[rQueue->Cursor].IsExist = r->IsExist;
        DhtCopy(&r->Body, &rQueue->Data[rQueue->Cursor].Body);
        printf("[WL:DEBUG] cached ");
        for (i=DEFAULT_HASH_SIZE-1;i>=0;i--) {
            printf("%02x ", rQueue->Data[rQueue->Cursor].Body.hash[i]);
        }
        printf("\n");
    } else {
        rQueue->Data[rQueue->Cursor].IsExist = r->IsExist;
        DhtCopy(&r->Body, &rQueue->Data[rQueue->Cursor].Body);
        rQueue->Cursor += 1;
        printf("[WL:DEBUG] cached ");
        for (i=DEFAULT_HASH_SIZE-1;i>=0;i--) {
            printf("%02x ", rQueue->Data[rQueue->Cursor-1].Body.hash[i]);
        }
        printf("\n");
    }
}
Result Dequeue(ResultQueue *rQueue) {
    Result r, v;
    int i = 0;
    r = rQueue->Data[0];
    // Shift
    for (i=1;i<QUEUE_SIZE;i++) {
        rQueue->Data[i-1] = rQueue->Data[i];
    }
    rQueue->Data[QUEUE_SIZE-1] = v;
    return r;
}
void StoreRefferer(Query *q, short int p) {
    if (q->ReffererIndex >= ReffererLength - 1) return;
    q->Refferer[q->ReffererIndex] = p;
    q->ReffererIndex += 1;
}