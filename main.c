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
PROCESS(main_process, "daas main process");
AUTOSTART_PROCESSES(&main_process);
PROCESS_THREAD(main_process, ev, data)
{
  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);
  
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
