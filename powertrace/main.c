#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "daas_node.h"
#include "csn.h"
#include "dht.h"
#include "config.h"
#include <stdio.h>
#include "sys/node-id.h"

int isEndStructRing = 0;
static struct etimer et;

PROCESS(main_process, "daas main process");
AUTOSTART_PROCESSES(&main_process);
PROCESS_THREAD(main_process, ev, data)
{
  PROCESS_BEGIN();
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
