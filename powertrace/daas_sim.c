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
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "simple-udp.h"



PROCESS(daas_sim_process, "daas_sim");
AUTOSTART_PROCESSES(&daas_sim_process);

/*
 * const
 */
#define MAX_NODE_NUM 32
#define UDP_BC_PORT 1234
#define UDP_UC_PORT 1235
#define SEND_INTERVAL		(20 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

static struct simple_udp_connection broadcast_connection;
static struct simple_udp_connection unicast_connection;
short int is_initialized = 0;

/*
 * structs
 */
typedef struct node_t {
  const uip_ipaddr_t *connect_able_lists[MAX_NODE_NUM];
  short int connect_able_rssi[MAX_NODE_NUM];
  short int connect_able_idx;
} node_t;
node_t node;

typedef struct bc_msg {
  uint8_t type;
} bc_msg_t;

typedef struct uc_msg {
  uint8_t type;
} uc_msg_t;

/* 
 * function define
 */
static void init(void);

/*
 * static vals
 */
static struct etimer et;

/*
 * function implementations
 */
static void init(void) {
  node.connect_able_idx = 0;
}

static void
bc_receiver(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr, uint16_t sender_port, const uip_ipaddr_t *receiver_addr, uint16_t receiver_port, const uint8_t *data, uint16_t datalen) {
  bc_msg_t *m = (bc_msg_t *)data;
  uc_msg_t res_m;
  switch(m->type) {
    case 0x1:
      res_m.type = 0x1;
      uip_debug_ipaddr_print(sender_addr);
      simple_udp_sendto(&unicast_connection, &res_m, 64, sender_addr);
      break;
  }
}

static void uc_receiver(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr, uint16_t sender_port, const uip_ipaddr_t *receiver_addr, uint16_t receiver_port, const uint8_t *data, uint16_t datalen) {
  static signed int rss;
  static signed int rss_val;
  static signed int rss_offset;
  rss_val = cc2420_last_rssi;
  rss_offset = -45;
  rss = rss_val + rss_offset;
  printf("[DEBUG] unicast receive sender_port: %d, receive_port:%d rssi: %d\n", sender_port, receiver_port, rss);
  node.connect_able_lists[node.connect_able_idx] = sender_addr;
  node.connect_able_rssi[node.connect_able_rssi] = rss;
  node.connect_able_idx += 1;
} 

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(daas_sim_process, ev, data)
{
  
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t addr;
  bc_msg_t m;
  m.type = 0x1;

  PROCESS_BEGIN();

  simple_udp_register(&broadcast_connection, UDP_BC_PORT, NULL, UDP_BC_PORT, bc_receiver);
  simple_udp_register(&unicast_connection, UDP_UC_PORT, NULL, UDP_UC_PORT, uc_receiver);

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
    printf("Sending broadcast\n");
    uip_create_linklocal_allnodes_mcast(&addr);
    simple_udp_sendto(&broadcast_connection, &m, 64, &addr);
  }


    
  PROCESS_END();
}

