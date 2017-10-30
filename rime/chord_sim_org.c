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
#include "l_bit.h"
//#include <stdlib.h>
/*---------------------------------------------------------------------------*/
PROCESS(daas_process, "DAAS Process");
AUTOSTART_PROCESSES(&daas_process);
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
static void notification_cluster_head(int,int);
static void notification_completed_ring(int);
static void loopback_link_request(int);
static void transmission_hash(int);
static int org_pow(int, int);
static void show_hash_info(void);
// vals
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
static struct etimer et;

// const
#define SINK_NODE_ID 1
#define TOP_NODE_NUM 8
#define TOTAL_NODE 8
#define BROAD_CAST_LIMIT 5
#define DELAY_CLOCK CLOCK_SECOND * 5
#define DEFAULT_RSSI -1000
#define RETRY_BROADCAST 3

// creation val
short int creation_counter = 0;
short int is_initialized_node = 0;
short int is_started_routing = 0;
short int retry_broadcast_counter = 0;

// structs
typedef struct msg {
  uint8_t type;
  int level;
  int status;
  int progress_num; 
  int cluster_head_id;
  int is_from_cluster_head;
  sha1_hash_t range_start;
  sha1_hash_t range_last;
  sha1_hash_t unit;
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
  sha1_hash_t range_start;
  sha1_hash_t range_last;
  sha1_hash_t unit;
  sha1_hash_t range_child_start;
  sha1_hash_t range_child_last;
  sha1_hash_t child_unit;
  sha1_hash_t white_list[128];
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
  node.is_completed = 0;
  node.is_child_ring_completed = 0;
  node.progress_num = 0;
  node.child_level = 0;
  node.level = 255;
  node.link_list[0] = 0;
  node.link_list[1] = 0;
  init_min_hash(&node.range_start);
  init_min_hash(&node.range_last);
  init_min_hash(&node.range_child_last);
  init_min_hash(&node.range_child_start);
  init_min_hash(&node.unit);
  init_min_hash(&node.child_unit);
  is_initialized_node = 1;
  printf("[INFO] success!!\n");
}

void hash_copy(sha1_hash_t *dst, sha1_hash_t *src) {
  int i;
  for(i=0;i<20;i++){
    dst->hash[i] = src->hash[i];
  }
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
        printf("[INFO] duplicated request to %d...\n", from->u8[0]);
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
        loopback_link_request(node.cluster_head_id);
      }
      break;

    case 0x10:
      printf("[INFO] received notification from %d, cluster_head_id : %d\n", from->u8[0], res_m->cluster_head_id);
      node.cluster_head_id = res_m->cluster_head_id;
      break;

    case 0x11:
      node.is_completed = 1;
      node.is_cluster_head = 1;
      if(node.link_list[0] == node.cluster_head_id){
        printf("[DEBUG] next link is cluster head. so not send\n");
      } else {
        printf("[DEBUG] send complete notification to %d\n", node.link_list[0]);
        notification_completed_ring(node.link_list[0]);
      }
      break;

    // search hash
    case 0x30:
      printf("[INFO] received search hash request from %d\n", from->u8[0]);
      

    // loopback request
    case 0x80:
      printf("[INFO] received loop back request from %d level is %d\n", from->u8[0], res_m->level);
      node.is_cluster_head = 1;
      if(res_m->level != node.level) {
        printf("[DEBUG] level is not matched\n");
        node.is_linked[1] = 1;
        node.is_child_ring_completed = 1;
        notification_completed_ring(node.link_list[1]);
      } else {
        printf("[DEBUG] level is matched\n");
        node.is_linked[0] = 1;
        node.is_completed = 1;
        notification_completed_ring(node.link_list[0]);
      }
      break;
    // loopback response
    case 0x88:
      {
        printf("[DEBUG] sha-1 routing message from %d\n", from->u8[0]);
        sha1_hash_t calc_buf;
        sha1_hash_t *child_unit;
        if(res_m->level == node.level) {
          hash_copy(&calc_buf, &res_m->range_last);
          increment(&calc_buf);
          
          hash_copy(&node.range_start, &calc_buf);
          hash_copy(&node.range_child_start, &calc_buf);
          hash_copy(&node.unit, &res_m->unit);
          hash_copy(&node.range_last, &calc_buf);
          hash_copy(&node.range_child_last, &calc_buf);
          
          child_unit = division(&node.unit, (TOP_NODE_NUM/org_pow(2, node.level - 1)));
          hash_copy(&node.child_unit, child_unit);

          add(&node.range_last, &node.unit);
          add(&node.range_child_last, &node.child_unit);
          if(node.cluster_head_id != node.link_list[0])transmission_hash(node.link_list[0]);
          transmission_hash(node.link_list[1]);
        } else if(level_node_num <= 2) {
          hash_copy(&calc_buf, &res_m->range_last);
          increment(&calc_buf);

          hash_copy(&node.range_start, &calc_buf);
          hash_copy(&node.range_last, &calc_buf);
          hash_copy(&node.unit, &res_m->unit);

          add(&node.range_last, &node.unit);
        } else {
        }
        break;
      }
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
      if(node.is_linked[1]) break;
      m->type = 0x1;
      packetbuf_copyfrom(m, 64);
      unicast_send(&uc, from); 
      printf("[SUCCESS] send child ACK creation to %d\n", from->u8[0]);
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
    if(node.neighbor_id != 0) {
      notification_cluster_head(node.id, node.neighbor_id); 
      clock_wait(DELAY_CLOCK);
      unicast_link_request(node.neighbor_id);
    }
    } else {
    m.type = 0x1;
    m.level = node.level;
    packetbuf_copyfrom(&m, 64);
    broadcast_send(&broadcast);
    printf("[SUCCESS] init creation message sent\n");

    clock_wait(DELAY_CLOCK);
    if(node.neighbor_id != 0) {
      notification_cluster_head(node.cluster_head_id, node.neighbor_id); 
      clock_wait(DELAY_CLOCK);
      unicast_link_request(node.neighbor_id);
    }
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
  printf("[SUCCESS] send completed notification to %d\n", id);
}

static void node_add_to_link(int id) {
  if(node.link_list[0] == 0) {
    printf("[DEBUG] add next %d\n", id);
    node.link_list[0] = id;
    node.has_link = 1;
    return;
  } else if(node.link_list[1] == 0) {
    printf("[DEBUG] add child %d\n", id);
    node.link_list[1] = id;
    node.has_child = 1;
  } else {
    return;
  }
}

static void loopback_link_request(int id) {
  linkaddr_t to_addr;
  to_addr.u8[0] = id;
  to_addr.u8[1] = 0;
  message_t m;
  m.type = 0x80;
  m.level = node.level;
  packetbuf_copyfrom(&m, 64);
  unicast_send(&uc, &to_addr);
  printf("[SUCCESS] send loop back request to %d\n", id);
  node_add_to_link(id);
  return;
}

static void transmission_hash(int id) {
  linkaddr_t to_addr;
  to_addr.u8[0] = id;
  to_addr.u8[1] = 0;
  message_t m;
  m.type = 0x88;
  m.level = node.level;
  hash_copy(&m.range_start, &node.range_start);
  hash_copy(&m.range_last, &node.range_last);
  hash_copy(&m.unit, &node.unit);
  packetbuf_copyfrom(&m, 80);
  unicast_send(&uc, &to_addr);

  printf("[SUCCESS] send hash info to %d\n", id);
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

static void show_hash_info(void) {
  debug_print(&node.range_start);
  debug_print(&node.range_last);
  debug_print(&node.range_child_start);
  debug_print(&node.range_child_last);
  debug_print(&node.unit);
  debug_print(&node.child_unit);
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(daas_process, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  if(is_initialized_node == 0) node_init();

  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks);
  broadcast_open(&broadcast, 129, &broadcast_call);


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
  int level_node_num = TOP_NODE_NUM / org_pow(2, node.level-1);
  

  while(1) {
    if(node.is_completed && node.is_cluster_head && level_node_num > 2 && !node.is_child_ring_completed){
      if(retry_broadcast_counter >= RETRY_BROADCAST) break;
      struct_ring();
      retry_broadcast_counter += 1;
    }
    if(level_node_num <= 2){
      printf("[INFO] end level\n");
      break;
    }
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

  // sha-1 routing
  if(node.id == SINK_NODE_ID && !is_started_routing) {
    etimer_set(&et, CLOCK_SECOND * 15 + random_rand() % (CLOCK_SECOND * 15));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    printf("[INFO] start transmission hash id\n");
    sha1_hash_t buf;
    init_max_hash(&buf);
    sha1_hash_t *unit = division(&buf, TOP_NODE_NUM);
    hash_copy(&node.unit, unit);
    sha1_hash_t *child_unit = division(unit, (TOP_NODE_NUM / org_pow(2, 1)));
    hash_copy(&node.child_unit, child_unit);
    add(&node.range_last, unit);
    add(&node.range_child_last, child_unit);

    transmission_hash(node.link_list[0]); 
    transmission_hash(node.link_list[1]); 
    
    is_started_routing = 1;
  }

  etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
 
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
