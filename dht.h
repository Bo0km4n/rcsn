#ifndef _DHT_H_
#define _DHT_H_

void DHTInit(void);
void DHTUCReceiver(struct unicast_conn *, const linkaddr_t *);

typedef struct DHTMessage {
  int Type;
  int Level;
  int Publisher;
  int Progress;
  sha1_hash_t *Unit;
  sha1_hash_t *PrevID;
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
} DHT;

void InsertDHTMessage(DHTMessage *, int, int, int, int);
void StartHashAllocation(void);
void DHTSendUCPacket(DHTMessage *, int);
enum DHTMessageTypes {
  StartSearchRingNum,
  IncrementProgress
};
void CheckRingNum(CSN *);
void CheckChildRingNum(CSN *);
void SelfAllocate(DHT *);
void ConvertHash(int, sha1_hash_t *);
void DhtCopy(sha1_hash_t *, sha1_hash_t *);
void PrintHash(sha1_hash_t *);
extern DHT dht;
#endif
