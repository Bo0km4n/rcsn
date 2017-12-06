#ifndef _CSN_H_
#define _CSN_H_

typedef struct CSNMessage {
  int type;
  int level;
  int clusterHead; 
  int progress;
} CSNMessage;
typedef struct CSN{
  CSNMessage *M;
  int ID;
  int ClusterHeadID;
  int Successor;
  int Previous;
  int Level;

  void (*SendCreationMessage) (CSNMessage *m); 
  void (*SendUCPacket) (CSNMessage *m, int id);
  void (*InsertCSNMessage) (CSNMessage *m, int type, int nodeLevel, int clusterHead, int progress);
} CSN;

void InsertCSNMessage(CSNMessage *, int, int, int, int);
void StartStructCsn(void);
void SendCreationMessage(CSNMessage *);
void SendUCPacket(CSNMessage *, int);
void SendLinkRequest(CSNMessage *, int);
void CsnUCReceiver(struct unicast_conn *, const linkaddr_t *);
void CsnBCReceiver(struct broadcast_conn *, const linkaddr_t *);
void CsnInit(void);

enum MessageTypes {
  CreationType,
  LinkRequestType,
  LinkRequestACKType,
  LinkType
};
#endif
