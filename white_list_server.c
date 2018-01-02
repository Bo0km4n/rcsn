#include "csn.h"
#include "config.h"
#include "dht.h"
#include <stdio.h>
#include "white_list.h"
#include "white_list_mote.h"
#include "daas_node.h"
#include "l_bit.h"
#include "dev/button-sensor.h"
#include "sys/node-id.h"
#include "contiki.h"
#include "contiki-lib.h"
#include "net/rime/rime.h"

sha1_hash_t whiteList[WHITE_LIST_LEN];
static struct unicast_conn whiteListServerUC;
static struct broadcast_conn whiteListServerBC;
static struct unicast_callbacks whiteListServerCallBacks = {WhiteListServerRecv};
static struct broadcast_callbacks whiteListServerBCCallBacks = {WhiteListServerBCRecv};
WhiteListMessage *wlm;
PROCESS(whiteListProcess, "white list server process");
AUTOSTART_PROCESSES(&whiteListProcess);
PROCESS_THREAD(whiteListProcess, ev, data)
{
  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);
  unicast_open(&whiteListServerUC, WL_UC_PORT, &whiteListServerCallBacks);
  broadcast_open(&whiteListServerBC, WL_BC_PORT, &whiteListServerBCCallBacks);
  WhiteListInit(whiteList);
  PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
  printf("[WL:DEBUG] starting white list server...\n");
  broadcast_send(&whiteListServerBC);
  PROCESS_END();
}

// Publish ホワイトリストをネットワークに送信
void Pub(sha1_hash_t *h, int a) {
    linkaddr_t to;
    to.u8[0] = a;
    to.u8[1] = 0;

    wlm->Type = Publish;
    DhtCopy(h, &wlm->Body);
    PrintHash(&wlm->Body);
    packetbuf_copyfrom(wlm, 64);
    unicast_send(&whiteListServerUC, &to);
    return;
}

// WhiteListInit ホワイトリストにランダムなハッシュ値を挿入
void WhiteListInit(sha1_hash_t *list) {
    int i;
    int j;
    wlm = (WhiteListMessage *)malloc(sizeof(WhiteListMessage));
    for(i=0;i<WHITE_LIST_LEN;i++) {
        for(j=0;j<DEFAULT_HASH_SIZE;j++) {
            list[i].hash[j] = (uint8_t)(random_rand() % 255);
        }
    }
    return;
}

void debugServerWL(sha1_hash_t *list) {
    int i;
    int j;
    for(i=0;i<WHITE_LIST_LEN;i++) {
        printf("[WL:DEBUG] list %03d ||  ", i);
        for(j=DEFAULT_HASH_SIZE-1;j>=0;j--) {
            printf("%02x ", list[i].hash[j]);
        }
        printf("\n");
    }
    return;
}

// WhiteListServerRecv 受信時ハンドラ
void WhiteListServerRecv(struct unicast_conn *uc, const linkaddr_t *from) {
    printf("[WL:DEBUG] received wl message from %d\n", from->u8[0]);
    return;
}

void WhiteListServerBCRecv(struct broadcast_conn *bc, const linkaddr_t *from) {
    return;
}