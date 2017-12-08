#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "daas_node.h"
#include "csn.h"
#include "dht.h"
#include "config.h"
#include <stdio.h>
#include "sys/node-id.h"
#include "lib/list.h"
#include "lib/memb.h"

int isEndStructRing = 0;
static struct etimer et;
#define NEIGHBOR_TIMEOUT 60 * CLOCK_SECOND
#define MAX_NEIGHBORS 16
LIST(neighbor_table);
MEMB(neighbor_mem, struct Neighbor, MAX_NEIGHBORS);

static void receivedAnnouncement(struct announcement *a, const linkaddr_t *from, uint16_t id, uint16_t value) {
  struct Neighbor *e;

    //printf("Got announcement from %d.%d, id %d, value %d\n",
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
static struct announcement announcement;

static void mhRecv(struct multihop_conn *c, const linkaddr_t *sender, const linkaddr_t *prevhop, uint8_t hops) {
  printf("multihop message received '%s'\n", (char *)packetbuf_dataptr());
}
static linkaddr_t * mhForward(struct multihop_conn *c, const linkaddr_t *originator, const linkaddr_t *dest, const linkaddr_t *prevhop, uint8_t hops) {
  int num, i;
  struct Neighbor *n;

  if(list_length(neighbor_table) > 0) {
    for (n = list_head(neighbor_table); n != NULL; n = n->next) {
      if (n->addr.u8[0] == dest->u8[0] && n->addr.u8[1] == dest->u8[1]) return &n->addr;
    }
    num = random_rand() % list_length(neighbor_table);
    i = 0;
    for(n = list_head(neighbor_table); n != NULL && i != num; n = n->next) {
      ++i;
    }
    if(n != NULL) {
      printf("[MULTI_HOP:INFO] %d.%d: Forwarding packet to %d.%d (%d in list), hops %d\n",
	     linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
	     n->addr.u8[0], n->addr.u8[1], num,
	     packetbuf_attr(PACKETBUF_ATTR_HOPS));
      // returnしたアドレスに次のパケットを送信してる
      return &n->addr;
    }
  }
  printf("[MULTI_HOP:INFO] %d.%d: did not find a neighbor to foward to\n",
	 linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
  return NULL;
}

static const struct multihop_callbacks multihopCall = {mhRecv, mhForward};
struct multihop_conn multihop;


PROCESS(main_process, "daas main process");
AUTOSTART_PROCESSES(&main_process);
PROCESS_THREAD(main_process, ev, data)
{
  PROCESS_BEGIN();

  memb_init(&neighbor_mem);

  /* Initialize the list used for the neighbor table. */
  list_init(neighbor_table);

  /* Open a multihop connection on Rime channel CHANNEL. */
  multihop_open(&multihop, MULTIHOP_PORT, &multihopCall);

  /* Register an announcement with the same announcement ID as the
     Rime channel we use to open the multihop connection above. */
  announcement_register(&announcement,
		MULTIHOP_PORT,
		receivedAnnouncement);
  
  /* Set a dummy value to start sending out announcments. */
  announcement_set_value(&announcement, 0);


  if(node_id == ALL_HEAD_ID) {
    etimer_set(&et, CLOCK_SECOND * 10 + random_rand() % (CLOCK_SECOND * 10));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    StartStructCsn();
    DHTInit();
  } else {
    CsnInit();
    DHTInit();
  }
 
  PROCESS_END();
}
