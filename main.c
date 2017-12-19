#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "daas_node.h"
#include "csn.h"
#include "dht.h"
#include "config.h"
#include <stdio.h>
#include "sys/node-id.h"
#include "white_list_mote.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "dev/button-sensor.h"

int isEndStructRing = 0;
int i = 0;
static struct etimer et;
#define NEIGHBOR_TIMEOUT 60 * CLOCK_SECOND
#define MAX_NEIGHBORS 16
LIST(neighbor_table);
MEMB(neighbor_mem, struct Neighbor, MAX_NEIGHBORS);

static void receivedAnnouncement(struct announcement *a, const linkaddr_t *from, uint16_t id, uint16_t value) {
  struct Neighbor *e;

    // printf("Got announcement from %d.%d, id %d, value %d\n",
      //from->u8[0], from->u8[1], id, value);

  for(e = list_head(neighbor_table); e != NULL; e = e->next) {
    if(linkaddr_cmp(from, &e->addr)) {
      return;
    }
  }

  e = memb_alloc(&neighbor_mem);
  if(e != NULL) {
    linkaddr_copy(&e->addr, from);
    list_add(neighbor_table, e);
    //ctimer_set(&e->ctimer, NEIGHBOR_TIMEOUT, remove_neighbor, e);
  }
}

static linkaddr_t * mhForward(struct multihop_conn *c, const linkaddr_t *originator, const linkaddr_t *dest, const linkaddr_t *prevhop, uint8_t hops) {
  int num, i;
  struct Neighbor *n;
  printf("[MULTI_HOP:DEBUG] Received forward packet from %d\n", prevhop->u8[0]);
  if(list_length(neighbor_table) > 0) {
    for (n = list_head(neighbor_table); n != NULL; n = n->next) {
      if (n->addr.u8[0] == dest->u8[0] && n->addr.u8[1] == dest->u8[1]) {
        printf("[MULTI_HOP:DEBUG] find dest %d.%d\n", dest->u8[0], dest->u8[1]);
        return &n->addr;
      }
    }
    num = random_rand() % list_length(neighbor_table);
    i = 0;
    for(n = list_head(neighbor_table); n != NULL && i != num; n = n->next) {
      ++i;
    }
    if(n != NULL) {
      printf("[MULTI_HOP:DEBUG] %d.%d: Forwarding packet to %d.%d (%d in list), hops %d\n",
	     linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
	     n->addr.u8[0], n->addr.u8[1], num,
	     packetbuf_attr(PACKETBUF_ATTR_HOPS));
      // returnしたアドレスに次のパケットを送信してる
      return &n->addr;
    }
  }
  printf("[MULTI_HOP:DEBUG] %d.%d: did not find a neighbor to foward to\n",
	 linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
  return NULL;
}

static linkaddr_t * resultMhForward(struct multihop_conn *c, const linkaddr_t *originator, const linkaddr_t *dest, const linkaddr_t *prevhop, uint8_t hops) {
  int num, i;
  struct Neighbor *n;
  printf("[WL:DEBUG] Received forward packet from %d\n", prevhop->u8[0]);
  if(list_length(neighbor_table) > 0) {
    for (n = list_head(neighbor_table); n != NULL; n = n->next) {
      if (n->addr.u8[0] == dest->u8[0] && n->addr.u8[1] == dest->u8[1]) {
        printf("[WL:DEBUG] find dest %d.%d\n", dest->u8[0], dest->u8[1]);
        return &n->addr;
      }
    }
    num = random_rand() % list_length(neighbor_table);
    i = 0;
    for(n = list_head(neighbor_table); n != NULL && i != num; n = n->next) {
      ++i;
    }
    if(n != NULL) {
      printf("[WL:DEBUG] %d.%d: Forwarding packet to %d.%d (%d in list), hops %d\n",
	     linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
	     n->addr.u8[0], n->addr.u8[1], num,
	     packetbuf_attr(PACKETBUF_ATTR_HOPS));
      // returnしたアドレスに次のパケットを送信してる
      return &n->addr;
    }
  }
  printf("[MULTI_HOP:DEBUG] %d.%d: did not find a neighbor to foward to\n",
	 linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
  return NULL;
}

static void mhRecv(struct multihop_conn *c, const linkaddr_t *sender, const linkaddr_t *prevhop, uint8_t hops) {
  CSNMessage *m = (CSNMessage *)packetbuf_dataptr();

  switch (m->type) {
    case FinishNotifyType:
      printf("[CSN:DEBUG] Received multi hop finish notice from %d\n", sender->u8[0]);
      if (csn.Level < m->level) {
        csn.ChildRingNodeNum = m->progress;
        csn.ChildPrevious = sender->u8[0];
        csn.InsertCSNMessage(csn.M, StartChildRingType, 0, 0, 0);
        csn.SendUCPacket(csn.M, csn.ChildSuccessor);
        if (csn.Successor == csn.ClusterHeadID) {
          break;
        } else {
          csn.SendUCPacket(csn.M, csn.Successor);
        }
      } else {
        csn.RingNodeNum = m->progress;
        csn.Previous = sender->u8[0];
        StartStructChildCsn(1);
      }
      break;
    default:
      break;
  }
}
static void dhtMhRecv(struct multihop_conn *c, const linkaddr_t *sender, const linkaddr_t *prevhop, uint8_t hops) {
  DHTMessage *m = (DHTMessage *)packetbuf_dataptr();
  switch (m->Type) {
    case AllocateHash:
    {
      sha1_hash_t *buf = (sha1_hash_t *)malloc(sizeof(sha1_hash_t));
      printf("[DHT:DEBUG] Received allocate hash order from ring tail: %d\n", sender->u8[0]);
      // exec only all cluster head
      if (csn.ID == ALL_HEAD_ID && csn.Level == m->Level) {
        DhtCopy(&m->PrevID, buf);
        incrementHash(buf);
        DhtCopy(buf, dht.MinID);
        AllocateChildHash(&dht);
        // send hash allocate order to child successor
        if (csn.IsBot) break;
        DhtCopy(dht.ChildUnit, &dht.M->Unit);
        DhtCopy(dht.ChildMaxID, &dht.M->PrevID);
        dht.InsertDHTMessage(dht.M, AllocateHash, csn.Level, csn.ID, 1);
        dht.DHTSendUCPacket(dht.M, csn.ChildSuccessor);
        PrintDHT(&dht);
      }
      free(buf);
      break;
    }
    default:
      break;
  }
}

static void wlMhRecv(struct multihop_conn *c, const linkaddr_t *sender, const linkaddr_t *prevhop, uint8_t hops) {
  WhiteListMessage *m = (WhiteListMessage *)packetbuf_dataptr();
  switch (m->Type) {
    case Publish:
    printf("[WL:DEBUG] received multihop white list message\n");
    break;
    default:
    break;
  }
}

static void queryMhRecv(struct multihop_conn *c, const linkaddr_t *sender, const linkaddr_t *prevhop, uint8_t hops) {
  Query *q = (Query *)packetbuf_dataptr();
    if (CheckRange(&q->Body)) {
        if (CheckChildRange(&q->Body)) {
            whiteListMote.R->IsExist = ScanWhiteList(&q->Body);
            DhtCopy(&q->Body, &whiteListMote.R->Body);
            whiteListMote.R->Dest = q->Publisher;
            ResultSendMHPacket(whiteListMote.R);
        } else {
            printf("[WL:DEBUG] send to child \n");
            QuerySendUCPacket(q, csn.ChildSuccessor);
        }
    } else {
        QuerySendUCPacket(q, csn.Successor);
    }
}

static void resultMhRecv(struct multihop_conn *c, const linkaddr_t *sender, const linkaddr_t *prevhop, uint8_t hops) {
  Result *r = (Result *)packetbuf_dataptr();
  printf("[WL:DEBUG] received query result %d\n", r->IsExist);
}

static struct announcement announcement;
static const struct multihop_callbacks multihopCall = {mhRecv, mhForward};
static const struct multihop_callbacks dhtMultihopCall = {dhtMhRecv, mhForward};
static const struct multihop_callbacks wlMultihopCall = {wlMhRecv, mhForward};
static const struct multihop_callbacks qMultihopCall = {queryMhRecv, mhForward};
static const struct multihop_callbacks rMultihopCall = {resultMhRecv, resultMhForward};

struct multihop_conn multihop;
struct multihop_conn dhtMultihop;
struct multihop_conn wlMultihop;
struct multihop_conn qMultihop;
struct multihop_conn rMultihop;


PROCESS(main_process, "daas main process");
AUTOSTART_PROCESSES(&main_process);
PROCESS_THREAD(main_process, ev, data)
{
  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);

  memb_init(&neighbor_mem);

  /* Initialize the list used for the neighbor table. */
  list_init(neighbor_table);

  /* Open a multihop connection on Rime channel CHANNEL. */
  multihop_open(&multihop, MULTIHOP_PORT, &multihopCall);
  multihop_open(&dhtMultihop, MULTIHOP_DHT_PORT, &dhtMultihopCall);
  multihop_open(&qMultihop, MULTIHOP_QUERY_PORT, &qMultihopCall);
  multihop_open(&rMultihop, MULTIHOP_RESULT_PORT, &rMultihopCall);

  /* Register an announcement with the same announcement ID as the
     Rime channel we use to open the multihop connection above. */
  announcement_register(&announcement,
		MULTIHOP_PORT,
		receivedAnnouncement);
  
  /* Set a dummy value to start sending out announcments. */
  announcement_set_value(&announcement, 0);

  struct Neighbor *n;  
  csn.Multihop = &multihop;
  dht.Multihop = &dhtMultihop;
  whiteListMote.Multihop = &wlMultihop;
  whiteListMote.QMultihop = &qMultihop;
  whiteListMote.RMultihop = &rMultihop;
  for (i=0;i<5;i++) {
    etimer_set(&et, CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 5));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    for(n = list_head(neighbor_table); n != NULL; n = n->next) {
      if(n != NULL) {
        printf("[Table]: %d.%d\n", n->addr.u8[0], n->addr.u8[1]);
      }
    }
  }
  
  if(node_id == ALL_HEAD_ID) {
    StartStructCsn();
    DHTInit();
    WhiteListMoteInit();
  } else {
    CsnInit();
    DHTInit();
    WhiteListMoteInit();
  }
  
  PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event &&
			    data == &button_sensor);
  StartHashAllocation();

  /*
  * Your application logic
  */

   PROCESS_END();
}
