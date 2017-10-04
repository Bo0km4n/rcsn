/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Testing the broadcast layer in Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "cfs/cfs.h" 

#include "dev/button-sensor.h"

#include "dev/leds.h"
#include "dev/cc2420/cc2420.h"
#include "sys/node-id.h"
#include "sha1.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "sys/timer.h"
#include "sys/clock.h"
//#include <stdlib.h>
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/

// ====================
// function definition
// ====================
static char * convert_hash_to_char(uint8_t);
static void node_init();
static void recv_uc(struct unicast_conn *, const linkaddr_t *);
static void broadcast_recv(struct broadcast_conn *, const linkaddr_t *);
static void struct_ring(void);
static void unicast_link_request(int);
static void unicast_link(int);
static void node_add_to_link(int);
static void debug_unicast(void);
static bool validate_node_num(int, int);
static void completed_notification(int);
static void notification_cluster_head(int,int);
static void notification_completed_ring(int);
static void loopback_link_request(int);
static void loopback_link_response(int);
static void loopback_link(int);
static int org_pow(int, int);
// vals
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
static struct etimer et;

// const
#define SINK_NODE_ID 1
#define TOP_NODE_NUM 4
#define BROAD_CAST_LIMIT 5
#define DELAY_CLOCK 1000
#define DEFAULT_RSSI -1000

// creation val
int creation_counter = 0;
int is_initialized_node = 0;

// structs
typedef struct msg {
  uint8_t type;
  int level;
  int status;
  int progress_num; 
  int cluster_head_id;
  int is_from_cluster_head;
} message_t;

typedef struct node_t {
  int id;
  int cluster_head_id;
  int neighbor_id;
  int neighbor_rssi;
  int previous_id;
  int link_list[2];
  int has_link;
  int has_child;
  int is_linked[2];
  int is_cluster_head;
  int is_completed;
  int is_child_ring_completed;
  int level;
  int child_level;
  int progress_num;
} node_t;

node_t node;
static void node_init(void) {
  printf("[INFO] initializing node info...\n");
  node.id = node_id;
  node.cluster_head_id = 0;
  node.neighbor_id = 0;
  node.neighbor_rssi = DEFAULT_RSSI;
  node.previous_id = 0;
  node.has_link = 0;
  node.has_child = 0;
  node.is_linked[0] = 0;
  node.is_linked[1] = 0;
  node.is_cluster_head = 0;
  node.progress_num = 0;
  node.child_level = 0;
  node.level = 255;
  node.link_list[0] = 0;
  node.link_list[1] = 0;

  is_initialized_node = 1;
  printf("[INFO] success!!\n");
}

static void recv_uc(struct unicast_conn *c, const linkaddr_t *from) {
  message_t *res_m;
  message_t m;
  linkaddr_t to_addr;

  static signed int rss;
  static signed int rss_val;
  static signed int rss_offset;
  rss_val = cc2420_last_rssi;
  rss_offset = -45;
  rss = rss_val + rss_offset;
  res_m = (message_t *)packetbuf_dataptr();
  int level_node_num = TOP_NODE_NUM / org_pow(2, res_m->level-1);

  switch (res_m->type) {
    // TOP level creation message
    case 0x1:
      if(rss > node.neighbor_rssi && (node.previous_id != from->u8[0])) {
        node.neighbor_rssi = rss;
        node.neighbor_id = from->u8[0];
        printf("RSSI of last received packet = %d from %d\n", rss, from->u8[0]);
      }
      break;
    case 0x2:
      printf("[DEBUG] received link request from %d\n", from->u8[0]);
      if(node.is_linked[0]){
        m.status = 400;
      } else {
        m.status = 200;
      }
      m.type = 0x3;
      packetbuf_copyfrom(&m, 64);
      unicast_send(&uc, from);
    case 0x3:
      printf("[DEBUG] received link response from %d\n", from->u8[0]);
      if(res_m->status == 200) {
        unicast_link(from->u8[0]);
        node.has_link = 1;
        break;
      } else {
        break;
      }
    case 0x4:
      printf("[INFO] received link completed from %d, now progress : %d, level_node_num: %d level: %d, cluster_head_id : %d\n", from->u8[0], res_m->progress_num + 1, level_node_num, res_m->level, node.cluster_head_id);
      node.is_linked[0] = 1;
      if(res_m->is_from_cluster_head){
        node.level = res_m->level + 1;
      } else {
        node.level = res_m->level;
      }
      node.previous_id = from->u8[0];
      node.progress_num = res_m->progress_num;
      if(res_m->progress_num + 1 >= level_node_num) {
        //TODO
        //loopback用のメソッドに変更
        loopback_link_request(node.cluster_head_id);
        notification_completed_ring(node.cluster_head_id);
      }
      break;
    case 0x10:
      printf("[INFO] received notification from %d, cluster_head_id : %d\n", from->u8[0], res_m->cluster_head_id);
      node.cluster_head_id = res_m->cluster_head_id;
      break;
    case 0x11:
      if(node.is_completed == 1){
        break;
      }
      printf("[INFO] notificated ring creation is completed. FROM %d\n", from->u8[0]);
      if((res_m->level - node.level) == 0) {
        node.is_completed = 1;
        node.is_cluster_head = 1;
        notification_completed_ring(node.link_list[0]);
        printf("[SUCCESS] send message that completed creation ring TO %d\n", node.link_list[0]);
      } else {
        node.is_child_ring_completed = 1;
        notification_completed_ring(node.link_list[1]);
        printf("[SUCCESS] send message that completed creation child ring TO %d\n", node.link_list[1]);
      }
      break; 
    // loopback request
    case 0x80:
      break;
    // loopback response
    case 0x81:
      break;
    // loopback connect
    case 0x82:
      break;
    case 0x99:
      printf("[DEBUG] received from id: %d || level is %d\n", from->u8[0], node.level);
      break;
    default:
      printf("[ERROR] invalid message type\n");
      break;
  }

  return;
}

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
  message_t *res_m;
  message_t *m = (message_t *)malloc(sizeof(message_t));
  res_m = (message_t *)packetbuf_dataptr();


  switch (res_m->type) {
    case 0x1: 
      if(node.is_completed || node.is_linked[0] || (res_m->level >= node.level)) break;
      printf("[INFO] creation message recieved from :%d: \n", from->u8[0]);
      m->type = 0x1;
      packetbuf_copyfrom(m, 64);
      unicast_send(&uc, from); 
      printf("[SUCCESS] send ACK creation\n");
      break;
      // top level creation message received
    case 0x2:
      if(node.level < res_m->level) break;
      m->type = 0x1;
      packetbuf_copyfrom(m, 64);
      unicast_send(&uc, from); 
      printf("[SUCCESS] send child ACK creation\n");
      break;
    case 0x10:
      node.cluster_head_id = res_m->cluster_head_id;
      break;
    default:
      break;
  }

  free(m);
  return;
}

static void notification_cluster_head(int cluster_head_id, int to) {
  linkaddr_t to_addr;
  to_addr.u8[0] = to;
  to_addr.u8[1] = 0;
  message_t m;
  m.type = 0x10;
  m.cluster_head_id = cluster_head_id;
  packetbuf_copyfrom(&m, 64);
  unicast_send(&uc, &to_addr);
  printf("[SUCCESS] send notification request to %d\n", to);

}

static void struct_ring(void) {
  message_t m;
  if(node.is_completed && !node.is_child_ring_completed) {
    m.type = 0x2;
    m.level = node.level + 1;

    packetbuf_copyfrom(&m, 64);
    broadcast_send(&broadcast);
    printf("[SUCCESS] send child ring creation message\n");

    clock_wait(DELAY_CLOCK);
    notification_cluster_head(node.id, node.neighbor_id); 
    clock_wait(DELAY_CLOCK);
    unicast_link_request(node.neighbor_id);
  } else {
    m.type = 0x1;
    m.level = node.level;
    packetbuf_copyfrom(&m, 64);
    broadcast_send(&broadcast);
    printf("[SUCCESS] init creation message sent\n");

    clock_wait(DELAY_CLOCK);
    notification_cluster_head(node.cluster_head_id, node.neighbor_id); 
    clock_wait(DELAY_CLOCK);
    unicast_link_request(node.neighbor_id);
  }

}

static void unicast_link_request(int id) {
  linkaddr_t to_addr;
  to_addr.u8[0] = id;
  to_addr.u8[1] = 0;
  message_t m;
  m.type = 0x2;
  packetbuf_copyfrom(&m, 64);
  unicast_send(&uc, &to_addr);
  printf("[SUCCESS] send link request to %d\n", id);
}

static void unicast_link(int id) {
  linkaddr_t to_addr;
  to_addr.u8[0] = id;
  to_addr.u8[1] = 0;
  message_t m;
  m.type = 0x4;
  m.progress_num = node.progress_num + 1;
  m.is_from_cluster_head = node.is_cluster_head;
  if(node.is_cluster_head) {
    m.level = node.level + 1;
  } else {
    m.level = node.level;
  }
  packetbuf_copyfrom(&m, 64);
  unicast_send(&uc, &to_addr);
  printf("[SUCCESS] send link request to %d\n", id);
  node_add_to_link(id);
}

static void notification_completed_ring(int id) {
  linkaddr_t to_addr;
  to_addr.u8[0] = id;
  to_addr.u8[1] = 0;
  message_t m;
  m.type = 0x11;
  m.level = node.level;
  packetbuf_copyfrom(&m, 64);
  unicast_send(&uc, &to_addr);
  printf("[SUCCESS] send link request to %d\n", id);
  node_add_to_link(id);
}

static void node_add_to_link(int id) {
  if(node.link_list[0] == 0) {
    node.link_list[0] = id;
    node.has_link = 1;
    return;
  } else if(node.link_list[1] == 0) {
    node.link_list[1] = id;
    node.has_child = 1;
  } else {
    return;
  }
}

static void loopback_link_request(int id) {
  printf("debug\n");
  return;
}

static void debug_unicast(void) {
  linkaddr_t to_addr;
  to_addr.u8[0] = node.neighbor_id;
  to_addr.u8[1] = 0;
  message_t m;
  m.type = 0x99;
  packetbuf_copyfrom(&m, 64);
  unicast_send(&uc, &to_addr);
}

static bool validate_node_num(int level, int progress_num) {
  int node_num = TOP_NODE_NUM/(org_pow(2, level-1));
  printf("[DEBUG] node_num :%d, current_progress_num: %d, level: %d, bias: %d\n", node_num, progress_num+1, level, org_pow(2, level-1));
  if(node_num == (progress_num + 1)) return true;
  else return false;
}

static int org_pow(int base, int exponent) {
  int i = exponent;
  int answer = 1;
  while(i!=0) {
    answer = answer * base;
    i--;
  }

  return answer;
}

static void completed_notification(int to_id) {
  linkaddr_t to_addr;
  to_addr.u8[0] = to_id;
  to_addr.u8[1] = 0;
  message_t m;
  m.type = 0x4;
  m.level = node.level;

  packetbuf_copyfrom(&m, 64);
  unicast_send(&uc, &to_addr);
  printf("[SUCCESS] send completed notification\n");
}

static char * convert_hash_to_char(uint8_t node_id) {
  SHA1Context sha;
  uint8_t message[20];
  char hash_node_id[40];
  char *buf;
  char node_id_str[5];
  buf = malloc(2);
  int i;

  SHA1Reset(&sha);
  sprintf(node_id_str, "%d", node_id);
  SHA1Input(&sha, (const unsigned char *)node_id_str, strlen(node_id_str));
  SHA1Result(&sha, message);

   for (i=0; i<20; i++) {
    buf[0] = '\0';
    sprintf(buf, "%02x", message[i]);
    strcat(hash_node_id, buf);
  }

  free(buf);
  return hash_node_id;
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  
  if(is_initialized_node == 0) node_init();

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks);
  broadcast_open(&broadcast, 129, &broadcast_call);
  int level_node_num = TOP_NODE_NUM / org_pow(2, node.level-1);

  // current level
  while(1) {
    if (node_id == SINK_NODE_ID && node.link_list[0] == 0) {
      node.level = 1;
      node.cluster_head_id = SINK_NODE_ID;
      struct_ring();
    }
    if(node.is_linked[0] && !node.link_list[0]){
      struct_ring();
    }
    if(node.is_completed && node.is_linked[0] && node.link_list[0]) break;
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

  // info reset
  printf("[INFO] reset information\n");
  node.neighbor_id = 0;
  node.neighbor_rssi = DEFAULT_RSSI;
  node.progress_num = 0;
  

  while(1) {
    if(node.is_completed && node.is_cluster_head && level_node_num > 1 && !node.is_child_ring_completed){
      struct_ring();
    }
    if(level_node_num <= 2) break;
    if(node.link_list[0] && node.link_list[1] && node.is_child_ring_completed){
      printf("[DEBUG] neighbor: %d, child: %d\n", node.link_list[0], node.link_list[1]);
      break;
    }
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

  printf("Struct ring is finished\n");
  etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
