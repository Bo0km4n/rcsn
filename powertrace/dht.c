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
#include "./lib/l_bit.h"
#include "dev/cc2420/cc2420.h"

static struct unicast_conn dhtUC;
static struct unicast_callbacks dhtUCCallBacks = {DHTUCReceiver};
DHT dht;
void DHTInit(void) {
  unicast_open(&dhtUC, DHT_UC_PORT, &dhtUCCallBacks);
  dht.RingNodeNum = 0;
  dht.ChildRingNodeNum = 0;
  dht.ID = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  dht.ChildID = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
  dht.M = (DHTMessage *)malloc(sizeof(DHTMessage));
  dht.InsertDHTMessage = InsertDHTMessage;
  dht.DHTSendUCPacket = DHTSendUCPacket;
}

void DHTUCReceiver(struct unicast_conn *c, const linkaddr_t *from) {
  DHTMessage *m = (DHTMessage *)packetbuf_dataptr();

  switch(m->Type) {
    case StartSearchRingNum:
      dht.InsertDHTMessage(dht.M, IncrementProgress, csn.ID, 1); 
      dht.DHTSendUCPacket(dht.M, csn.Successor);
      break; 
    case IncrementProgress:
      dht.InsertDHTMessage(dht.M, IncrementProgress, m->Publisher, m->Progress + 1);
      dht.DHTSendUCPacket(dht.M, csn.Successor);
      break;
    default:
      break;
  }
}

void InsertDHTMessage(DHTMessage *m, int type, int publisher, int progress) {
  m->Type = type;
  m->Publisher = publisher;
  m->Progress = progress;
}
void DHTSendUCPacket(DHTMessage *m, int id) {
  linkaddr_t toAddr;
  toAddr.u8[0] = id;
  toAddr.u8[1] = 0;
  packetbuf_copyfrom(m, 100);
  unicast_send(&dhtUC, &toAddr);
}
