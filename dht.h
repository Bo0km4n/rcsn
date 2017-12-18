#ifndef _DHT_H_
#define _DHT_H_

#include "l_bit.h"
#include "contiki.h"
#include "contiki-lib.h"
#include "net/rime/rime.h"
void DHTInit(void);
void DHTUCReceiver(struct unicast_conn *, const linkaddr_t *);

typedef struct DHTMessage {
  int Type;
  int Level;
  int Publisher;
  int Progress;
  sha1_hash_t Unit;
  sha1_hash_t PrevID;
} DHTMessage;
typedef struct DHT{
  int RingNodeNum;
  int ChildRingNodeNum;
  DHTMessage *M;
  sha1_hash_t *MaxID;
  sha1_hash_t *MinID;
  sha1_hash_t *ChildMaxID;
  sha1_hash_t *ChildMinID;
  sha1_hash_t *Unit;
  sha1_hash_t *ChildUnit;
  struct multihop_conn *Multihop;
  void (*InsertDHTMessage) (DHTMessage *m, int type, int level, int publisher, int progress);
  void (*DHTSendUCPacket) (DHTMessage *m, int id);
  void (*SelfAllocate) (struct DHT *dht);
  void (*AllocateHashToSuccessor) (struct DHT *dht);
  void (*AllocateHashByPrev) (struct DHT *dht, sha1_hash_t *unit, sha1_hash_t *prev);
} DHT;

void InsertDHTMessage(DHTMessage *, int, int, int, int);
void StartHashAllocation(void);
void DHTSendUCPacket(DHTMessage *, int);
void AllocateHashToSuccessor(DHT *dht);
void AllocateHashByPrev(DHT *, sha1_hash_t *, sha1_hash_t *);
void AllocateChildHash(DHT *);
enum DHTMessageTypes {
  StartSearchRingNum,
  IncrementProgress,
  AllocateHash
};
void CheckRingNum(CSN *);
void CheckChildRingNum(CSN *);
void SelfAllocate(DHT *);
void ConvertHash(int, sha1_hash_t *);
void DhtCopy(sha1_hash_t *, sha1_hash_t *);
void PrintHash(sha1_hash_t *);
void stdHash(sha1_hash_t *);
void incrementHash(sha1_hash_t *);
void PrintDHT(DHT *);
extern DHT dht;
#endif
