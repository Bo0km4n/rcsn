/* \author
 *        Katsuya Kawabe <al14031@shibaura-it.ac.jp>
 * Last Update
 *        2017/10/2 Monday
 */
#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "cfs/cfs.h" 

#include "dev/button-sensor.h"

#include "dev/leds.h"
#include "dev/cc2420/cc2420.h"
#include "sha1.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "sys/timer.h"
#include "sys/clock.h"

PROCESS(daas_sim_process, "daas_sim");
AUTOSTART_PROCESSES(&daas_sim_process);

/*
 * const
 */
#define MAX_NODE_NUM 256
#define UDP_PORT 1234
#define SEND_INTERVAL		(20 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

static struct simple_udp_connection broadcast_connection;
short int is_initialized = 0;

/*
 * structs
 */
typedef struct node_t {
  short int id;
  short int connect_able_lists[MAX_NODE_NUM];
} node_t;
node_t node;

static void node_init(void) {
  printf("[INFO] initializing node info...\n");
  node.id = node_id;
  is_initialized = 1;
  printf("[INFO] success!!\n");
}

static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  printf("Data received on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);
}


typedef struct bc_msg {
  uint8_t type;
} bc_msg;

typedef struct uc_msg {
  uint8_t type;
} uc_msg;

/* 
 * function define
 */
static void node_init();

/*
 * static vals
 */
static struct etimer et;

/*
 * function implementations
 */


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(daas_sim_process, ev, data)
{
  
  if(is_initialized == 0) node_init();
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t addr;


  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
    printf("Sending broadcast\n");
    uip_create_linklocal_allnodes_mcast(&addr);
    simple_udp_sendto(&broadcast_connection, "Test", 4, &addr);
  }


    
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
