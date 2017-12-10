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
  dht.RingNodeNum = 0;
  dht.ChildRingNodeNum = 0;
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
}
/*---------------------------------------------------------------------------*/
PROCESS(dhtProcess, "hash id allocate process");
PROCESS_THREAD(dhtProcess, ev, data) {
  PROCESS_BEGIN();
  printf("[DHT:INFO] start hash allocate process\n");
  if(csn.ID == ALL_HEAD_ID) CheckRingNum(&csn);
  CheckChildRingNum(&csn);
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
  packetbuf_copyfrom(m, 64);

  clock_wait(DELAY_CLOCK + random_rand() % (CLOCK_SECOND * RAND_MARGIN));
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
  ConvertHash(dht->RingNodeNum, mod);
  sha1Div(buf, mod, dht->Unit);
  free(buf);
  free(mod);
  PrintHash(dht->Unit);
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