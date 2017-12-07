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

int successorRSSI = RSSI;
int successorCandidate = 0;
int neighborID = 0;
int progress = 0;
int ucLock = 0;
int currentLevelMaxNode = 0;
int isInitialized = 0;
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
  csn.ChildSuccessor = 0;
  csn.ChildPrevious = 0;
  csn.ChildLevel = 0;
  csn.M = (CSNMessage *)malloc(sizeof(CSNMessage));
  csn.ClusterHeadID = 0;
  csn.SendCreationMessage = SendCreationMessage;
  csn.SendUCPacket = SendUCPacket;
  csn.InsertCSNMessage = InsertCSNMessage;
  printf("[CSN:INFO] csn info initilized\n");
  isInitialized = 1;
}
/*---------------------------------------------------------------------------*/
/* For struct ring process  */
PROCESS(csnProcess, "csn struct ring process");
PROCESS_THREAD(csnProcess, ev, data)
{
  if(!isInitialized) {
    init();
  }
  PROCESS_BEGIN();

  unicast_open(&ringUC, CSN_UC_PORT, &ringUCCallBacks);
  broadcast_open(&ringBC, CSN_BC_PORT, &ringBCCallBacks);
  
  if((int)data == 1) {
    etimer_set(&et, CLOCK_SECOND * 10 + random_rand() % (CLOCK_SECOND * 3));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    progress = 1;
    csn.Level = 1;
    currentLevelMaxNode = MAX_NODE / orgPow(2, csn.Level-1);
    csn.ClusterHeadID = ALL_HEAD_ID;
    csn.InsertCSNMessage(csn.M , CreationType, csn.Level, csn.ClusterHeadID, progress);
    csn.SendCreationMessage(csn.M);
  } else {
    csn.InsertCSNMessage(csn.M , CreationType, csn.Level, csn.ClusterHeadID, progress);
    csn.SendCreationMessage(csn.M);
  }
  
  etimer_set(&et, CLOCK_SECOND * 10 + random_rand() % (CLOCK_SECOND * 10));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  if (successorRSSI == RSSI) {
    printf("[CSN:INFO] nothing neighbor node\n");
    csn.InsertCSNMessage(csn.M, FinishNotifyType, 0, 0, 0);
    csn.SendUCPacket(csn.M, csn.ClusterHeadID);
  } else {
    csn.InsertCSNMessage(csn.M, LinkRequestType, 0, 0, 0);
    csn.SendUCPacket(csn.M, neighborID);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* For struct child ring process  */
PROCESS(csnChildProcess, "csn struct child ring process");
PROCESS_THREAD(csnChildProcess, ev, data)
{
  PROCESS_BEGIN();
  printf("[CSN:INFO] start child process\n");
  progress = 1;
  successorRSSI = RSSI;
  csn.ChildLevel = csn.Level + 1;
  csn.ChildClusterHeadID = csn.ID;
  csn.InsertCSNMessage(csn.M, CreationType, csn.ChildLevel, csn.ChildClusterHeadID, progress);
  csn.SendCreationMessage(csn.M);
  
  etimer_set(&et, CLOCK_SECOND * 10 + random_rand() % (CLOCK_SECOND * 10));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  if (successorRSSI == RSSI) {
    printf("[CSN:INFO] nothing child node\n");
  } else {
    csn.InsertCSNMessage(csn.M, ChildLinkRequestType, 0, 0, 0);
    csn.SendUCPacket(csn.M, neighborID);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void StartStructCsn(void) {
  process_start(&csnProcess, (void *)1);
}
void StartStructChildCsn(int p) {
  process_start(&csnChildProcess, (void *)p);
}
void CsnUCReceiver(struct unicast_conn *c, const linkaddr_t *from) {
  CSNMessage *m = (CSNMessage *)packetbuf_dataptr();
  
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
        printf("[CSN:INFO] RSSI of of last received packet = %d from %d\n", rss, from->u8[0]);
      }
      break;
    case LinkRequestType:
      if (ucLock) {
        ResponseReject(&csn, from->u8[0]);
      }
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
      currentLevelMaxNode = MAX_NODE / orgPow(2, csn.Level - 1);
      printf("[CSN:INFO] Linked from %d level: %d\n", from->u8[0], csn.Level);
      
      if (progress < currentLevelMaxNode) {
        process_start(&csnProcess, (void *)0);
      } else {
        csn.Successor = csn.ClusterHeadID;
        csn.InsertCSNMessage(csn.M, FinishNotifyType, csn.Level, csn.ClusterHeadID, 0);
        csn.SendUCPacket(csn.M, csn.ClusterHeadID);
      }
      break;
    case FinishNotifyType:
      printf("[CSN:INFO] received finish notice from %d level: %d\n", from->u8[0], m->level);
      if (csn.Level < m->level) {
        csn.ChildPrevious = from->u8[0];
        csn.InsertCSNMessage(csn.M, StartChildRingType, 0, 0, 0);
        csn.SendUCPacket(csn.M, csn.ChildSuccessor);
        csn.SendUCPacket(csn.M, csn.Successor);
      } else {
        csn.Previous = from->u8[0];
        csn.InsertCSNMessage(csn.M, StartChildRingType, 0, 0, 0);
        csn.SendUCPacket(csn.M, csn.Successor);
      }
      break;
    case StartChildRingType:
      printf("[CSN:INFO] received start child ring notice from %d\n", from->u8[0]);
      StartStructChildCsn(0);
      break;
    case ChildLinkRequestType:
      if (ucLock) {
        ResponseReject(&csn, from->u8[0]);
      }
      ucLock = 1;
      csn.InsertCSNMessage(csn.M, ChildLinkRequestACKType, 0, 0, 0);
      csn.SendUCPacket(csn.M, from->u8[0]);
      break;
    case ChildLinkRequestACKType:
      csn.InsertCSNMessage(csn.M, LinkType, csn.ChildLevel, csn.ChildClusterHeadID, progress+1); 
      csn.SendUCPacket(csn.M, from->u8[0]);
      csn.ChildSuccessor = from->u8[0];
      break;
    case RequestRejectType:
      printf("[CSN:INFO] Rejected request from %d\n", from->u8[0]);
      break;
    default:
      break;
  }
}
void CsnBCReceiver(struct broadcast_conn *c, const linkaddr_t *from) {
  CSNMessage *m = (CSNMessage *)packetbuf_dataptr();

  switch(m->type) {
    case CreationType:
      if (csn.Level != 0 || ucLock) break;
      csn.InsertCSNMessage(m, CreationType, 0, 0, 0);
      clock_wait(DELAY_CLOCK + random_rand() % (CLOCK_SECOND * 2));
      csn.SendUCPacket(m, from->u8[0]);
      break;
    default:
      break;
  }
}
void CsnInit(void) {
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
void ResponseReject(CSN *csn, int id) {
  csn->InsertCSNMessage(csn->M, RequestRejectType, 0, 0, 0);
  csn->SendUCPacket(csn->M, id);
}

int orgPow(int base, int exponent) {
  int i = exponent;
  int answer = 1;
  while(i!=0) {
    answer = answer * base;
    i--;
  }

  return answer;
}
