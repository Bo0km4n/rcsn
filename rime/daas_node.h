#ifndef _DAAS_NODE_H_
#define _DAAS_NODE_H_


#define DAAS_CLUSTER_HEAD_ID 1
#define LAUNCHER_PORT 145
#define CSN_PORT 146
#define CBT_PORT 147
#define DHT_PORT 148

typedef struct CSNNode{
  int ID;
  int RingClusterHeadID;
} CSNNode;
typedef struct CBTNode{
  int ID;
  int ClusterHeadID;
} CBTNode;
typedef struct DaasDHT{

} DaasDHT;
typedef struct DaasContext {
  CSNNode CSNNode;
  CBTNode CBTNode;
  DaasDHT DHT;
} DaasContext;
void LauncherReceiver(struct unicast_conn *, const linkaddr_t *);


#endif
