#ifndef _DHT_H_
#define _DHT_H_

void DHTInit(void);
void DHTUCReceiver(struct unicast_conn *, const linkaddr_t *);
typedef struct DHTMessage {
  int Type;
  int Publisher;
  int Progress;
} DHTMessage;
typedef struct DHT{
  int RingNodeNum;
  int ChildRingNodeNum;
  DHTMessage *M;
  sha1_hash_t *ID;
  sha1_hash_t *ChildID;
  multihop_conn *Multihop;
  void (*InsertDHTMessage) (DHTMessage *m, int type, int publisher, int progress);
  void (*DHTSendUCPacket) (DHTMessage *m, int id);
} DHT;

void InsertDHTMessage(DHTMessage *, int, int, int);
void StartHashAllocation(void);
void DHTSendUCPacket(DHTMessage *, int);
enum DHTMessageTypes {
  StartSearchRingNum,
  IncrementProgress
};
extern DHT dht;
#endif
