#include "contiki.h"
#include "contiki-lib.h"
#include "net/rime/rime.h"
#include "sys/node-id.h"

#include <stdio.h>
#include <string.h>
#include "daas_node.h"
#include "csn.h"
#include "config.h"
#include "dev/cc2420/cc2420.h"

int successorRSSI = -1000;
int successorCandidate = 0;
int neighborID = 0;
int progress = 0;
int ucLock = 0;
static struct unicast_conn ringUC;
static struct broadcast_conn ringBC;
static struct unicast_callbacks ringUCCallBacks = {CsnUCReceiver};
static struct broadcast_callbacks ringBCCallBacks = {CsnBCReceiver};
static struct etimer et;
CSN csn;

void init(void) {
  csn.ID = node_id;
  csn.Successor = 0;
  csn.Previous = 0;
  csn.Level = 0;
  csn.M = (CSNMessage *)malloc(sizeof(CSNMessage));
  csn.ClusterHeadID = 0;
  csn.SendCreationMessage = SendCreationMessage;
  csn.SendUCPacket = SendUCPacket;
  csn.InsertCSNMessage = InsertCSNMessage;
}
/*---------------------------------------------------------------------------*/
PROCESS(csnProcess, "csn struct ring process");
PROCESS_THREAD(csnProcess, ev, data)
{
  init();
  PROCESS_BEGIN();

  printf("[CSN:DEBUG] hello\n");
  unicast_open(&ringUC, CSN_UC_PORT, &ringUCCallBacks);
  broadcast_open(&ringBC, CSN_BC_PORT, &ringBCCallBacks);
  
  if((int)data == 1) {
    etimer_set(&et, CLOCK_SECOND * 10 + random_rand() % (CLOCK_SECOND * 3));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    csn.Level = 1;
    csn.ClusterHeadID = DAAS_CLUSTER_HEAD_ID;
    csn.InsertCSNMessage(csn.M , CreationType, csn.Level, csn.ClusterHeadID, progress);
    csn.SendCreationMessage(csn.M);
  } else {
    csn.InsertCSNMessage(csn.M , CreationType, csn.Level, csn.ClusterHeadID, progress);
    csn.SendCreationMessage(csn.M);
  }
  
  etimer_set(&et, CLOCK_SECOND * 10 + random_rand() % (CLOCK_SECOND * 10));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  InsertCSNMessage(csn.M, LinkRequestType, 0, 0, 0);
  csn.SendUCPacket(csn.M, neighborID);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void StartStructCsn(void) {
  process_start(&csnProcess, (void *)1);
}
void CsnUCReceiver(struct unicast_conn *c, const linkaddr_t *from) {
  CSNMessage *m = (CSNMessage *)packetbuf_dataptr();
  printf("[CSN:INFO] uc receive packet m.type: %d\n", m->type);
  
  static signed int rss;
  static signed int rss_val;
  static signed int rss_offset;
  rss_val = cc2420_last_rssi;
  rss_offset = -45;
  rss = rss_val + rss_offset;

  switch(m->type) {
    case CreationType:
      if (rss > successorRSSI && (csn.Previous != from->u8[0])) {
        successorRSSI = rss;
        neighborID = from->u8[0];
        printf("[CSN:DEBUG] RSSI of of last received packet = %d from %d\n", rss, from->u8[0]);
      }
      break;
    case LinkRequestType:
      ucLock = 1;
      csn.InsertCSNMessage(csn.M, LinkRequestACKType, 0, 0, 0);
      csn.SendUCPacket(csn.M, from->u8[0]);
      break;
    case LinkRequestACKType:
      csn.InsertCSNMessage(csn.M, LinkType, csn.Level, csn.ClusterHeadID, progress+1); 
      csn.SendUCPacket(csn.M, from->u8[0]);
      csn.Successor = from->u8[0];
      break;
    case LinkType:
      ucLock = 0;
      csn.Previous = from->u8[0];
      csn.Level = m->level;
      csn.ClusterHeadID = m->clusterHead;
      progress = m->progress;
      printf("[CSN:INFO] Linked from %d\n", from->u8[0]);
      break;
    default:
      break;
  }
}
void CsnBCReceiver(struct broadcast_conn *c, const linkaddr_t *from) {
  CSNMessage *m = (CSNMessage *)packetbuf_dataptr();
  printf("[CSN:DEBUG] bc receive packet m.type: %d\n", m->type);

  switch(m->type) {
    case CreationType:
      if (csn.Previous != 0 || ucLock != 0) break;
      csn.InsertCSNMessage(m, CreationType, 0, 0, 0);
      csn.SendUCPacket(m, from->u8[0]);
      break;
    default:
      break;
  }
}
void CsnInit(void) {
  printf("[CSN:DEBUG] initilized ring conn\n");
  init();
  unicast_open(&ringUC, CSN_UC_PORT, &ringUCCallBacks);
  broadcast_open(&ringBC, CSN_BC_PORT, &ringBCCallBacks);
}
void SendCreationMessage(CSNMessage *m) {
  packetbuf_copyfrom(m, 100);
  broadcast_send(&ringBC);
}
void SendUCPacket(CSNMessage *m, int id) {
  linkaddr_t toAddr;
  toAddr.u8[0] = id;
  toAddr.u8[1] = 0;
  packetbuf_copyfrom(m, 100);
  unicast_send(&ringUC, &toAddr);
}
void InsertCSNMessage(CSNMessage *m, int type, int nodeLevel, int clusterHead, int progress) {
  m->type = type;
  m->level = nodeLevel;
  m->clusterHead = clusterHead;
  m->progress = progress;
}
