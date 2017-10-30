/*
 * リング作成起動用プログラム
 *
 * About:
 *  ノードID: 1のノードに起動メッセージを送信
 */
#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "launcher.h"
#include "daas_node.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
PROCESS(daas_launcher_process, "DAAS Process");
AUTOSTART_PROCESSES(&daas_launcher_process);
/*---------------------------------------------------------------------------*/
static struct unicast_conn launcherUC;
static const struct unicast_callbacks launcherCallBacks = {LauncherReceiver};

void LauncherReceiver(struct unicast_conn *c, const linkaddr_t *from) {
  printf("Receive\n");
}

void LaunchStart() {
  linkaddr_t to_addr;
  to_addr.u8[0] = DAAS_CLUSTER_HEAD_ID;
  to_addr.u8[1] = 0;
  printf("launch start\n");
  unicast_open(&launcherUC, LAUNCHER_PORT, &launcherCallBacks);
  packetbuf_copyfrom(LAUNCH_MESSAGE, 32);
  unicast_send(&launcherUC, &to_addr);
}

PROCESS_THREAD(daas_launcher_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&launcherUC);)
  PROCESS_BEGIN();
  LaunchStart();
  PROCESS_END();
}
