#ifndef _CSN_H_
#define _CSN_H_

#include "contiki.h"
#include "contiki-lib.h"
#include "net/rime/rime.h"
typedef struct CSNMessage {
  int type;
  int level;
  int clusterHead; 
  int progress;
  int Dest;
  int Pub;
} CSNMessage;
typedef struct CSN{
  CSNMessage *M;
  int ID;
  int ClusterHeadID;
  int Successor;
  int Previous;
  int Level;
  int ChildSuccessor;
  int ChildClusterHeadID;
  int ChildPrevious;
  int ChildLevel;
  int RetryCounter;
  int IsRingTail;
  int IsBot;
  int MaxRingNode;
  int RingNodeNum;
  int ChildRingNodeNum;
  struct multihop_conn *Multihop;
  void (*SendCreationMessage) (CSNMessage *m); 
  void (*SendUCPacket) (CSNMessage *m, int id);
  void (*InsertCSNMessage) (CSNMessage *m, int type, int nodeLevel, int clusterHead, int progress);
} CSN;

extern CSN csn;
void InsertCSNMessage(CSNMessage *, int, int, int, int);
void StartStructCsn(void);
void StartStructChildCsn(int);
void SendCreationMessage(CSNMessage *);
void SendUCPacket(CSNMessage *, int);
void SendLinkRequest(CSNMessage *, int);
void CsnUCReceiver(struct unicast_conn *, const linkaddr_t *);
void CsnBCReceiver(struct broadcast_conn *, const linkaddr_t *);
void CsnInit(void);
void RetrySearchBC(CSN *, int);
void ResponseReject(CSN *, int, int);
int orgPow(int, int);


enum CSNMessageTypes {
  CreationType,
  LinkRequestType,
  LinkRequestACKType,
  LinkType,
  FinishNotifyType,
  StartChildRingType,
  ChildLinkRequestType,
  ChildLinkRequestACKType,
  RequestRejectType,
  ChildRequestRejectType,
  ToHead
};
#endif
