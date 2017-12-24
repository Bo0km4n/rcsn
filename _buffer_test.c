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

typedef struct Message {
    sha1_hash_t key;
    short int refferer[32];
    short int reffererIndex;
} Message;
void UCReceiver(struct unicast_conn *c, const linkaddr_t *from) {
    int i=0;
    Message *m = (Message *)packetbuf_dataptr();
    PrintHash(&m->key);
    for (i=0;i<32;i++) {
        printf("%d: %d\n", i, m->refferer[i]);
    }
}

int p=0;
static struct unicast_conn uc;
static struct unicast_callbacks ucCallBacks = {UCReceiver};

PROCESS(main_process, "daas main process");
AUTOSTART_PROCESSES(&main_process);
PROCESS_THREAD(main_process, ev, data)
{
    PROCESS_BEGIN();
    linkaddr_t to;
    to.u8[0] = 2;
    to.u8[1] = 0;
    unicast_open(&uc, 100, &ucCallBacks);
    Message message;
    for (p=0;p<DEFAULT_HASH_SIZE;p++) {
        message.key.hash[p] = random_rand() % 255;
    }
    for (p=0;p<32;p++) {
        message.refferer[p] = random_rand() % 10;
    }
    packetbuf_copyfrom(&message, 100);
    unicast_send(&uc, &to);
    PROCESS_END();
}