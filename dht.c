#include "contiki.h"
#include "contiki-lib.h"
#include "net/rime/rime.h"
#include "sys/node-id.h"

#include <stdio.h>
#include <string.h>
#include "daas_node.h"
#include "csn.h"
#include "dht.h"
#include "config.h"
#include "l_bit.h"
#include "dev/cc2420/cc2420.h"

static struct unicast_conn dhtUC;
static struct unicast_callbacks dhtUCCallBacks = {DHTUCReceiver};
DHT dht;
// static struct etimer et;
void DHTInit(void) {
  unicast_open(&dhtUC, DHT_UC_PORT, &dhtUCCallBacks);
  dht.MaxID = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  dht.MinID = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  dht.ChildMaxID = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  dht.ChildMinID = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  dht.Unit = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  dht.ChildUnit = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  dht.M = (DHTMessage *)malloc(sizeof(DHTMessage));
  dht.InsertDHTMessage = InsertDHTMessage;
  dht.DHTSendUCPacket = DHTSendUCPacket;
  dht.SelfAllocate = SelfAllocate;
  dht.AllocateHashToSuccessor = AllocateHashToSuccessor;
  dht.AllocateHashByPrev = AllocateHashByPrev;
}
/*---------------------------------------------------------------------------*/
PROCESS(dhtProcess, "hash id allocate process");
PROCESS_THREAD(dhtProcess, ev, data) {
  PROCESS_BEGIN();
  printf("[DHT:DEBUG] start hash allocate process\n");
  // ID self allocate logic
  dht.SelfAllocate(&dht);
  dht.AllocateHashToSuccessor(&dht);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

void StartHashAllocation(void) {
  process_start(&dhtProcess, (void *)1);
}
void DHTUCReceiver(struct unicast_conn *c, const linkaddr_t *from) {
  DHTMessage *m = (DHTMessage *)packetbuf_dataptr();

  switch(m->Type) {
    case IncrementProgress:
      dht.InsertDHTMessage(dht.M, IncrementProgress, csn.Level, m->Publisher, m->Progress + 1);
      dht.DHTSendUCPacket(dht.M, csn.Successor);
      break;
    case AllocateHash:
      printf("[DHT:DEBUG] Received allocate hash order from %d\n", from->u8[0]);
      dht.AllocateHashByPrev(&dht, &m->Unit, &m->PrevID);
      break;
    default:
      break;
  }
}

void InsertDHTMessage(DHTMessage *m, int type, int level, int publisher, int progress) {
  m->Type = type;
  m->Level = level;
  m->Publisher = publisher;
  m->Progress = progress;
}
void DHTSendUCPacket(DHTMessage *m, int id) {
  linkaddr_t toAddr;
  toAddr.u8[0] = id;
  toAddr.u8[1] = 0;
  packetbuf_copyfrom(m, 80);

  int delay = 0;
  if (csn.ID <= 10) delay = csn.ID;
  if (csn.ID >= 10 && csn.ID < 100) delay = csn.ID % 10;
  if (csn.ID >= 100) delay = csn.ID % 100;
  clock_wait(DELAY_CLOCK + random_rand() % delay);
  if (csn.IsRingTail && id == csn.Successor) {
    multihop_send(dht.Multihop, &toAddr);
  } else {
    unicast_send(&dhtUC, &toAddr);
  }
}

void CheckRingNum(CSN *csn) {
  dht.InsertDHTMessage(dht.M, IncrementProgress, csn->Level, csn->ID, 1);
  dht.DHTSendUCPacket(dht.M, csn->Successor);
  return;
}
void CheckChildRingNum(CSN *csn) {
  dht.InsertDHTMessage(dht.M, IncrementProgress, csn->Level, csn->ID, 1);
  dht.DHTSendUCPacket(dht.M, csn->ChildSuccessor);
  return;
}

// This function is executed by wsn header
void SelfAllocate(DHT *dht) {
  init_max_hash(dht->MaxID);
  sha1_hash_t *buf = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  sha1_hash_t *mod = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  DhtCopy(dht->MaxID, buf);
  ConvertHash(csn.RingNodeNum, mod);
  sha1Div(buf, mod, dht->Unit);
  free(buf);
  free(mod);
  PrintHash(dht->Unit);
  return;
}
void AllocateHashToSuccessor(DHT *dht) {
  init_min_hash(&dht->M->Unit);
  init_min_hash(&dht->M->PrevID); 
  DhtCopy(dht->Unit, &dht->M->Unit);
  DhtCopy(dht->MaxID, &dht->M->PrevID);
  dht->InsertDHTMessage(dht->M, AllocateHash, csn.Level, csn.ID, 1);
  dht->DHTSendUCPacket(dht->M, csn.Successor);
  return;
}
void AllocateHashByPrev(DHT *dht, sha1_hash_t *unit, sha1_hash_t *prev) {
  sha1_hash_t *buf1 = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  sha1_hash_t *buf2 = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  int MaxRingNode = MAX_NODE / orgPow(2, csn.Level - 1);

  // increment
  incrementHash(prev);
  // copy min id
  DhtCopy(prev, dht->MinID);
  DhtCopy(prev, buf1);
  // unit copy
  DhtCopy(unit, dht->Unit);
  DhtCopy(unit, buf2);
  // max id = min id + unit
  sha1Add(buf1, buf2, dht->MaxID);

  free(buf1);
  free(buf2);
  if (!csn.IsBot) {
    AllocateChildHash(dht);
  }
  PrintDHT(dht);
  // send hash allocate order to successor
  if (!csn.IsBot) {
    DhtCopy(dht->Unit, &dht->M->Unit);
    DhtCopy(dht->MaxID, &dht->M->PrevID);
    dht->InsertDHTMessage(dht->M, AllocateHash, csn.Level, csn.ID, 1);
    dht->DHTSendUCPacket(dht->M, csn.Successor);

    // send hash allocate order to child successor
    DhtCopy(dht->ChildUnit, &dht->M->Unit);
    DhtCopy(dht->ChildMaxID, &dht->M->PrevID);
    dht->InsertDHTMessage(dht->M, AllocateHash, csn.Level, csn.ID, 1);
    dht->DHTSendUCPacket(dht->M, csn.ChildSuccessor);
    return;
  }

  // couldn't find child node. but this node is not original bot node.
  // then, send hash allocate order to successor.
  // This time, this doesn't send it to child successor.
  if (csn.IsBot && (MaxRingNode > 2)) {
    DhtCopy(dht->Unit, &dht->M->Unit);
    DhtCopy(dht->MaxID, &dht->M->PrevID);
    dht->InsertDHTMessage(dht->M, AllocateHash, csn.Level, csn.ID, 1);
    dht->DHTSendUCPacket(dht->M, csn.Successor);
    return;
  }

  return;

}
void AllocateChildHash(DHT *dht) {
  sha1_hash_t *buf1 = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  sha1_hash_t *buf2 = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  sha1_hash_t *nodeNum = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));

  // copy buf = unit
  DhtCopy(dht->Unit, buf1);
  // child unit = buf1 / csn.child node num
  ConvertHash(csn.ChildRingNodeNum, nodeNum);
  sha1Div(buf1, nodeNum, dht->ChildUnit);
  // copy child min id = min id
  DhtCopy(dht->MinID, dht->ChildMinID);
  // copy buf2 = child min id
  DhtCopy(dht->ChildMinID, buf2);
  DhtCopy(dht->ChildUnit, buf1);
  // child max id = buf2 + child unit
  sha1Add(buf1, buf2, dht->ChildMaxID);

  free(buf1);
  free(buf2);
  free(nodeNum);  
  return;
}
void DhtCopy(sha1_hash_t *src, sha1_hash_t *dst) {
  int i;
  for (i=0;i<DEFAULT_HASH_SIZE;i++) {
    dst->hash[i] = src->hash[i];
  }
  return;
}
void ConvertHash(int n, sha1_hash_t *buf) {
  init_min_hash(buf);
  buf->hash[0] = (uint8_t)n;
  return;
}
void PrintHash(sha1_hash_t *h) {
  int i;
  printf("[DHT:DEBUG] hash: ");
  for (i=DEFAULT_HASH_SIZE-1;i>=0;i--) {
    printf("%02x ", h->hash[i]);
  }
  printf("\n");
}
void stdHash(sha1_hash_t *h) {
  int i;
  h->hash[0] = 0x01;
  for (i=1;i<DEFAULT_HASH_SIZE;i++) {
    h->hash[i] = 0x00;
  }
}
void incrementHash(sha1_hash_t *h) {
  sha1_hash_t *vec = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  sha1_hash_t *result = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  stdHash(vec);
  sha1Add(h, vec, result);

  DhtCopy(result, h);
  free(vec);
  free(result);
  return;
}

void PrintDHT(DHT *dht) {
  printf("[DHT:DEBUG] ===== hash info max, min, unit, child unit, child max, child min =====\n");
  PrintHash(dht->MaxID);
  PrintHash(dht->MinID);
  PrintHash(dht->Unit);
  PrintHash(dht->ChildUnit);
  PrintHash(dht->ChildMaxID);
  PrintHash(dht->ChildMinID);
  printf("[DHT:DEBUG] ===== ========================================================== =====\n");
  return;
}