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
#include "powertrace.h"

static struct unicast_conn whiteListUC;
static struct unicast_conn resultUC;
static struct unicast_conn searchUC;
static struct unicast_conn notifyUC;
static struct unicast_callbacks whiteListUCCallBacks = {WhiteListUCRecv};
static struct unicast_callbacks searchUCCallBacks = {SearchUCRecv};
static struct unicast_callbacks resultUCCallBacks = {ResultUCRecv};
static struct unicast_callbacks notifyUCCallBacks = {NotifyUCRecv};
static struct broadcast_conn whiteListBC;
static struct broadcast_callbacks whiteListBCCallBacks = {WhiteListBCRecv};
static struct etimer et;
WhiteListMote whiteListMote;
int queryCounter = 0;
/*---------------------------------------------------------------------------*/
/* random search hash id  */
PROCESS(randomHashSearchProcess, "random search hash id process");
PROCESS_THREAD(randomHashSearchProcess, ev, data)
{
  PROCESS_BEGIN();
  printf("[WL:DEBUG] start random search hash process\n");
  powertrace_start(CLOCK_SECOND * 10);
  while(whiteListMote.Switch) {
      //if (csn.ID != 12) break; // for debug
      queryCounter += 1;
      etimer_set(&et, CLOCK_SECOND * (10 + (random_rand() % 20)));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      HashRandomization(&whiteListMote.Q->Body);
      QueryPublish(whiteListMote.Q);
      printf("[EVAL:DEBUG] query counter %d\n", queryCounter);
      if (queryCounter == 10) {
            etimer_set(&et, CLOCK_SECOND * 20);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
            printf("[EVAL:DEBUG] stop powertrace\n");
            powertrace_stop();
            process_exit(&randomHashSearchProcess);
      }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

void WhiteListMoteInit() {
    unicast_open(&whiteListUC, WL_UC_PORT, &whiteListUCCallBacks);
    unicast_open(&searchUC, WL_SEARCH_PORT, &searchUCCallBacks);
    unicast_open(&resultUC, WL_RESULT_PORT, &resultUCCallBacks);
    unicast_open(&notifyUC, WL_RANDOM_QUERY_PORT, &notifyUCCallBacks);
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
    if (!whiteListMote.Switch) {
        printf("[WL:DEBUG] received switch on message\n");
        whiteListMote.SwitchOn();
        clock_wait(random_rand() % 10);
        broadcast_send(bc);
        StartRandomSearch();
    }
}
void SearchUCRecv(struct unicast_conn *uc, const linkaddr_t *from) {
    Query *q = (Query *)packetbuf_dataptr();
    int i;
    //printf("[WL:DEBUG] debug received search uc from %d next %d\n", from->u8[0], q->Next);
    if (q->Next != csn.ID) {
        QuerySendMHPacket(q, q->Next, csn.Previous);
        return;
    }
    if (csn.Level == 1) {
        // int index = ScanCache(&q->Body);
        // if (index >= 0) {
        //     for (i=0;i<q->ReffererIndex;i++) {
        //         whiteListMote.R->Refferer[i] = q->Refferer[i];
        //     }
        //     whiteListMote.R->ReffererIndex = q->ReffererIndex;
        //     whiteListMote.R->IsExist = whiteListMote.ResultQueue->Data[index].IsExist;
        //     DhtCopy(&whiteListMote.ResultQueue->Data[index].Body, &whiteListMote.R->Body);
        //     whiteListMote.R->Dest = q->Publisher;
        //     ResultSendMHPacket(whiteListMote.R);
        //     return;
        // }
    }
    if (csn.IsBot) {
        if (CheckRange(&q->Body)) {
            ReplyResult(q, whiteListMote.R);
        } else if (csn.MaxRingNode > 2) {
            StoreRefferer(q, csn.ID);
            QuerySendUCPacket(q, csn.Successor);
        } else {
            printf("[WL:DEBUG] query error\n");
        }
    } else {
        if (CheckRange(&q->Body)) {
            if (CheckChildRange(&q->Body)) {
                ReplyResult(q, whiteListMote.R);
                return;
            } else {
                StoreRefferer(q, csn.ID);
                QuerySendUCPacket(q, csn.ChildSuccessor);
                return;
            }
        } else {
            StoreRefferer(q, csn.ID);
            QuerySendUCPacket(q, csn.Successor);
            return;
        }
    }
}
void ResultUCRecv(struct unicast_conn *uc, const linkaddr_t *from) {
    Result *r = (Result *)packetbuf_dataptr();
    //printf("[WL:DEBUG] received query result from %d dest %d\n", from->u8[0], r->Dest);
    if (csn.ID == r->Dest) {
        printf("[WL:DEBUG] received query result from %d\n", from->u8[0]);
    } else if (r->Next != csn.ID) {
        ResultSendUCPacket(r, csn.Previous);
    } else {
        r->ReffererIndex -= 1;
        ResultSendUCPacket(r, r->Refferer[r->ReffererIndex]);
    }
}
void NotifyUCRecv(struct unicast_conn *uc, const linkaddr_t *from) {
    whiteListMote.SwitchOn();
    StartRandomSearch();
    printf("[WL:DEBUG] received start random query notification\n");
    linkaddr_t successor;
    linkaddr_t childSuccessor;
    successor.u8[0] = csn.Successor;
    successor.u8[1] = 0;
    childSuccessor.u8[0] = csn.ChildSuccessor;
    childSuccessor.u8[1] = 0;
    packetbuf_copyfrom("", 4);
    if (!csn.IsRingTail) {
        unicast_send(&notifyUC, &successor);
    }
    if (!csn.IsBot) {
        unicast_send(&notifyUC, &childSuccessor);
    }
}

void resultForward(Result *r, int id) {
    return;
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
    if (csn.IsRingTail && id == csn.ClusterHeadID) {
        to.u8[0] = csn.Previous;
    } else {
        to.u8[0] = id;
    }
    to.u8[1] = 0;
    unicast_send(&searchUC, &to);
}
void QuerySendMHPacket(Query *q, int toID, int next) {
    linkaddr_t to;
    to.u8[0] = next;
    to.u8[1] = 0;    
    q->Next = toID;
    StoreRefferer(q, csn.ID);
    packetbuf_copyfrom(q, 100);
    int delay = 0;
    if (csn.ID <= 10) delay = csn.ID;
    if (csn.ID >= 10 && csn.ID < 100) delay = csn.ID % 10;
    if (csn.ID >= 100) delay = csn.ID % 100;
    clock_wait(DELAY_CLOCK + random_rand() % delay);
    unicast_send(&searchUC, &to);
}
void ResultSendUCPacket(Result *r, int id) {
    linkaddr_t to;
    r->Next = id;
    packetbuf_copyfrom(r, 100);
      int delay = 0;
    if (csn.ID <= 10) delay = csn.ID;
    if (csn.ID >= 10 && csn.ID < 100) delay = csn.ID % 10;
    if (csn.ID >= 100) delay = csn.ID % 100;
    clock_wait(DELAY_CLOCK + random_rand() % delay);
    if (csn.IsRingTail && id == csn.ClusterHeadID) {
        //multihop_send(whiteListMote.QMultihop, &to);
        to.u8[0] = csn.Previous;
    } else {
        to.u8[0] = id;
    }
    to.u8[1] = 0;
    unicast_send(&resultUC, &to);
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
    CleanRefferer(q);    
    StoreRefferer(q, csn.ID);
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
    printf("[WL:DEBUG] stored refferer %d\n", p);
}
void ReplyResult(Query *q, Result *r) {
    int i;
    for (i=0;i<ReffererLength;i++) {
        r->Refferer[i] = q->Refferer[i];
    }
    r->ReffererIndex = q->ReffererIndex-1;
    r->IsExist = ScanWhiteList(&q->Body);
    DhtCopy(&q->Body, &r->Body);
    r->Dest = q->Publisher;
    PrintRefferer(q);
    ResultSendUCPacket(r, r->Refferer[r->ReffererIndex]);
}
void StartRandomQuery() {
    whiteListMote.SwitchOn();
    linkaddr_t successor;
    linkaddr_t childSuccessor;
    successor.u8[0] = csn.Successor;
    successor.u8[1] = 0;
    childSuccessor.u8[0] = csn.ChildSuccessor;
    childSuccessor.u8[1] = 0;
    packetbuf_copyfrom("", 4);
    if (!csn.IsRingTail) {
        unicast_send(&notifyUC, &successor);
    }
    if (!csn.IsBot) {
        unicast_send(&notifyUC, &childSuccessor);
    }
}
void PrintRefferer(Query *q) {
    int i;
    printf("[WL:DEBUG] Refferer ");
    for (i=0;i<ReffererLength;i++) {
        printf("%d ", q->Refferer[i]);
    }
    printf("\n");
}
void CleanRefferer(Query *q) {
    int i;
    for (i=0;i<ReffererLength;i++) {
        q->Refferer[i] = 0;
    }
    q->ReffererIndex = 0;
}