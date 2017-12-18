#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "daas_node.h"
#include "csn.h"
#include "dht.h"
#include "white_list_mote.h"
#include "config.h"
#include <stdio.h>
#include "sys/node-id.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "dev/button-sensor.h"

static struct unicast_conn whiteListUC;
static struct unicast_callbacks whiteListUCCallBacks = {WhiteListUCRecv};
WhiteListMote whiteListMote;
void WhiteListMoteInit() {
    unicast_open(&whiteListUC, WL_UC_PORT, &whiteListUCCallBacks);
    whiteListMote.Cursor = 0;
    whiteListMote.M = (WhiteListMessage *)malloc(sizeof(WhiteListMessage));
    printf("[WL:DEBUG] white list initilized\n");
    return;
}
void WhiteListUCRecv(struct unicast_conn *uc, const linkaddr_t *from) {
    WhiteListMessage *m = (WhiteListMessage *)packetbuf_dataptr();
    switch (m->Type) {
        case Publish:
        printf("[WL:DEBUG] received publish from %d\n", from->u8[0]);
        PrintHash(&m->Body);
        if (csn.ID == ALL_HEAD_ID) {
            InsertWLMessage(whiteListMote.M, &m->Body);
            WLSendUCPacket(whiteListMote.M, csn.Successor);
        } else {
            PublishHandle(&m->Body);
        }
        break;
        default:
        break;
    }
}
void WLSendUCPacket(WhiteListMessage *m, int id) {
    linkaddr_t to;
    to.u8[0] = id;
    to.u8[1] = 0;
    packetbuf_copyfrom(m, 64);
    int delay = 0;
    if (csn.ID <= 10) delay = csn.ID;
    if (csn.ID >= 10 && csn.ID < 100) delay = csn.ID % 10;
    if (csn.ID >= 100) delay = csn.ID % 100;
    clock_wait(DELAY_CLOCK + random_rand() % delay);
    if (csn.IsRingTail && id == csn.Successor) {
        multihop_send(whiteListMote.Multihop, &to);
    } else {
        unicast_send(&whiteListUC, &to);
    }
}

void InsertWLMessage(WhiteListMessage *m, sha1_hash_t *body) {
    DhtCopy(body, &m->Body);
}

void PublishHandle(sha1_hash_t *body) {
    if (csn.IsBot) {
        // 自リングの範囲のみチェック
    } else {
        // 自リングの範囲チェック
        // 子リングの範囲チェック
    }
}