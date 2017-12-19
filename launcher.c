#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "daas_node.h"
#include "csn.h"
#include "config.h"
#include "dht.h"
#include <stdio.h>
#include "sys/node-id.h"
static struct unicast_conn dhtUC;
static struct unicast_callbacks dhtUCCallBacks = {};

PROCESS(launcherProcess, "launcher process");
AUTOSTART_PROCESSES(&launcherProcess);
PROCESS_THREAD(launcherProcess, ev, data)
{
  PROCESS_BEGIN();
  linkaddr_t toAddr;
  toAddr.u8[0] = ALL_HEAD_ID;
  toAddr.u8[1] = 0;
  DHTMessage m;
  m.Type = StartSearchRingNum;
  packetbuf_copyfrom(&m, 100);
  unicast_open(&dhtUC, DHT_UC_PORT, &dhtUCCallBacks);
  unicast_send(&dhtUC, &toAddr);
  PROCESS_END();
}
