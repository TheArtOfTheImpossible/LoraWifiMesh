// Copyright © 2020 by antónio montez . All rights reserved.
// Licensed under the MIT license.

#ifndef _LORA_WIFI_MESH_H_
#define _LORA_WIFI_MESH_H_
#include "ArduinoUniqueID.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
extern "C" {
#include <espnow.h>
#include <user_interface.h>
#include "Wire.h"
}
#include <SoftwareSerial.h>


#elif defined(ESP32) || defined ( _HELTEC_)
#include "Wire.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#endif

#if defined(ARDUINO_ARCH_AVR)
#elif defined(ARDUINO_ARCH_ESP32)
#define _HELTEC_ true
#endif

#if defined ( _HELTEC_)
#include "heltec.h"

#else
#include <LoRa.h>
#endif

#include <string.h>
#include <stdlib.h>
#include "Arduino.h"

 
#define VT200  true
#define WIFI_MAX_MSG_SIZE 200

#define MSG_TYPE byte
#define DELIVER_STATUS byte
#define STSCODE byte
#define QUEUE_STATUS byte
#define ROUTE_TYPE byte

#define LORA_MESH_NODE_TYPE_MASTER 1
#define LORA_MESH_NODE_TYPE_GENERIC 2
#define LORA_MESH_NODE_TYPE_END_NODE 4
#define LORA_MESH_NODE_TYPE_NO_BROADCAST 8
#define LORA_MESH_NODE_TYPE_NO_ROUTING 16

#define LORA_MESH_MSG_REGISTRATION 1
#define LORA_MESH_MSG_USER 2
#define LORA_MESH_MSG_RREQ  4
#define LORA_MESH_MSG_RREP  8
#define LORA_MESH_MSG_RERR  16
#define LORA_MESH_MSG_SENDTO  32
#define LORA_MESH_MSG_ACK 64

#define LORA_MESH_MAX_MSG_SIZE 32

#define MESH_PROTOCOL_LORA 1
#define MESH_PROTOCOL_WIFI 2

#define LORA_MESH_BROADCAST_ADDRESS 0xFF
//---
#if defined(ARDUINO_ARCH_AVR)
      #define LORA_MESH_MAX_DROPNODES_TABLE_SIZE 4
      #define LORA_MESH_MAX_ROUTING_PATH_SIZE 8
      #define LORA_MESH_MAX_ROUTING_TABLE_SIZE 4
      #define LORA_MESH_MSG_QUEUE_SIZE 1
      #define LORA_MESH_RREQ_QUEUE_SIZE 1
      #define LORA_MESH_RECEIVED_QUEUE_SIZE 1
      #define LORA_MESH_MAX_NETWORK_SIZE 1      
#elif defined(ESP8266) || defined(ESP32)
      #define LORA_MESH_MAX_DROPNODES_TABLE_SIZE 32
      #define LORA_MESH_MAX_ROUTING_PATH_SIZE 8
      #define LORA_MESH_MAX_ROUTING_TABLE_SIZE 32
      #define LORA_MESH_MSG_QUEUE_SIZE 8
      #define LORA_MESH_RREQ_QUEUE_SIZE 8
      #define LORA_MESH_RECEIVED_QUEUE_SIZE 8
      #define LORA_MESH_MAX_NETWORK_SIZE 32
#endif 

#define    LORA_MESH_NODE_UNKOWN 1
#define    LORA_MESH_NODE_REGISTERED 2
#define    LORA_MESH_NODE_ALIVE 4

#define    LORA_MESH_QUEUE_FREE  1
#define    LORA_MESH_QUEUE_USED  2
#define    STS_ROUTE_WAITING  32
#define    STS_ROUTE_MISSING  64
#define    STS_ROUTE_UPDATED  128

#define LORA_MESH_QUEUE_TYPE_ANY 1
#define LORA_MESH_QUEUE_TYPE_MSG 2
#define LORA_MESH_QUEUE_TYPE_REQ 4

#define LORA_MESH_RREQ_QUEUE_TIMEOUT 6000
#define LORA_MESH_MSG_QUEUE_TIMEOUT 10000
#define LORA_MESH_KEEP_ALIVE_INTERVAL 30000
#define LORA_MESH_QUEUE_INTERVAL_RESET 120000

#define ERR_MSG_NOT_FOR_ME -1
#define ERR_DROP_DUE_TO_RULES -2
#define ERR_DUP_RREQ -3
#define ERR_DROP_ROUTING -4
#define ERR_RREQ_CRC_ERR -5

#define LORA_MESH_MAX_BLOCK_NODES 8

#define BLUE 34
#define GREEN 32
#define RED 31
#define YELLOW 33
#define MAGENTA 35
#define _WHITE 37

/* success codes */
      
#define      STS_UNKNOWN  0
#define      STS_OK  1
#define      STS_DELIVERED  2
#define      STS_RECEIVED  4
#define      STS_TIMEOUT  8
#define      STS_CRC_ERR   16
#define      STS_ROUTE_RETURNED  32
      
#define      STS_MSG_REACH_DESTINATION  40
#define      STS_MSG_ACK_RECEIVED  41

#define      STS_MSG_ACK_REGISTRATION_DONE 42

      /* error codes */      
#define      CRC_ERR  -1
#define      ROUTING_QUEUE_FULL  -50
#define      DROPNODES_QUEUE_FULL  -51
#define      RREQ_QUEUE_FULL  -52
#define      MSG_QUEUE_FULL  -53
#define      NETWORK_QUEUE_FULL -54

#define      ERR_NO_MSG  -70
#define      ERR_MSG_NOT_FOR_ME  -71
#define      ERR_DROP_DUE_TO_RULES  -72
#define      ERR_DUP_RREQ  -73
#define      ERR_DROP_ROUTING  -74
#define      ERR_RREQ_CRC_ERR  -75

#define      DROP_MSG_DUE_TO_FILTER_RULES  -90

#define      ERR_CANNOT_SEND_TO_SELF  -100
#define      ERR_CANNOT_ROUTE_TO_SELF  -101

#define ROUTE_DYNAMIC  1
#define ROUTE_STATIC  2

#define STS_RR_TO_BE_SENT 1
#define STS_RR_SENT 2
#define STS_NONE 0

#define LORA_MESH_SEND_MSG_RETRY_COUNT 8

typedef struct NODES {
        uint8_t nodeId ; //= 0x00;
        uint8_t macAddress[6] ; //= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        long lastKeepAlive;
        long RSSI;
        STSCODE sts ; //= 0x00;
        char path[LORA_MESH_MAX_ROUTING_PATH_SIZE];
};


typedef struct RECEIVED_MSG {
        uint8_t msgId;
        
        STSCODE sts;
        char msg[LORA_MESH_MAX_MSG_SIZE];
};

typedef union RECEIVED_Packet{
        struct RECEIVED_MSG _pkt;
        char _bmsg[sizeof(RECEIVED_MSG)];
};

typedef struct RECEIVED_TABLE {
      QUEUE_STATUS sts = LORA_MESH_QUEUE_FREE;
      RECEIVED_Packet _pkt;
      };
      
typedef struct HDR_MSG {
      MSG_TYPE hdrType;
      uint8_t len;
      uint8_t sourceNode;   
      uint8_t destinationNode; 
      uint8_t msgId;
        byte _crc;
        
      };

typedef struct RREQ_MSG {
      uint8_t sourceNode;   
      uint8_t destinationNode; 
      uint8_t uniqueId;   
      MSG_TYPE type;
      char path[LORA_MESH_MAX_ROUTING_PATH_SIZE];
      char msg[LORA_MESH_MAX_MSG_SIZE];
      };

typedef struct RREP_MSG {
      uint8_t sourceNode;   
      uint8_t destinationNode; 
      uint8_t uniqueId;   
      MSG_TYPE type;
      char path[LORA_MESH_MAX_ROUTING_PATH_SIZE];
      char msg[LORA_MESH_MAX_MSG_SIZE];
    };

typedef struct HI_MSG {
      uint8_t destNode;   
      uint8_t nodeID; 
      uint8_t type;
      uint8_t msgCount;
      };
      
typedef struct SENDTO_MSG {
      uint8_t sourceNode;   
      uint8_t destinationNode; 
      uint8_t uniqueId;   
      MSG_TYPE type;
      char path[LORA_MESH_MAX_ROUTING_PATH_SIZE];   
      char msg[LORA_MESH_MAX_MSG_SIZE];
      };
      

 typedef struct RREQ_DATAGRAM {
      HDR_MSG _hdr;
      RREQ_MSG _rreq; 
    };

 typedef struct RREP_DATAGRAM {
      HDR_MSG _hdr;
      RREP_MSG _rrep;
    };

 typedef struct SEND_DATAGRAM {
      HDR_MSG _hdr;
      SENDTO_MSG _send;
    };

typedef union SEND_Packet{
      SEND_DATAGRAM _msg;
      char _bmsg[sizeof(SEND_DATAGRAM)];
     };
     
typedef struct QUEUE_MSG {
      SEND_Packet _pkt;
      uint8_t retryCount;
      long timeStamp;
      QUEUE_STATUS sts = LORA_MESH_QUEUE_FREE;
      };
    
typedef union RREQ_Packet{
      RREQ_DATAGRAM _msg;
      char  _bmsg[sizeof(RREQ_DATAGRAM)];
      };

typedef union RREP_Packet{
      RREP_DATAGRAM _msg;
      char _bmsg[sizeof(RREQ_DATAGRAM)];
      };

typedef union RR_Packet{
      RREQ_DATAGRAM _rreq;
      RREP_DATAGRAM _rrep;
      char _bmsg[sizeof(SEND_DATAGRAM)];
     };
     
typedef struct RREQ_TABLE {
      RR_Packet _rr;
      uint8_t uniqueId;
      uint8_t retryCount;
      long timeStamp;
      DELIVER_STATUS _deliveredStatus = STS_NONE;
      QUEUE_STATUS sts = LORA_MESH_QUEUE_FREE;
      } ;

typedef struct ROUTING_TABLE {
      uint8_t destNode;   
      long timeStamp; 
      ROUTE_TYPE type;
      QUEUE_STATUS sts = LORA_MESH_QUEUE_FREE;
      char path[LORA_MESH_MAX_ROUTING_PATH_SIZE];
      } ;
typedef union Global_Packet{
      SEND_DATAGRAM _send;
      RREQ_DATAGRAM _rreq;
      RREP_DATAGRAM _rrep;
      char _bmsg[sizeof(SEND_DATAGRAM)];
     };
typedef struct  NODE_FILTER {
    uint8_t _sourceAddr;   
    uint8_t _destAddr;   
    QUEUE_STATUS _sts;
    };
    
typedef struct NODE_REGISTRATION {
        uint8_t userMsgType;
        uint8_t nodeId;
        uint8_t macAddress[6];
        long lastKeepAlive;
        byte RSSI;
        STSCODE sts;
        char path[LORA_MESH_MAX_ROUTING_PATH_SIZE];
};


typedef struct NODE_USER_MSG {
        uint8_t userMsgType;
};


typedef union USER_PACKET {
      NODE_REGISTRATION _reg;
      NODE_USER_MSG _user;
      char _b[sizeof(NODE_REGISTRATION)];
     };

typedef struct NODE_CONFIGURATION {
        uint8_t   nodeId;
        uint8_t   masterNode          = 0X44;
        uint8_t   macAddress[6]       = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t   nodeType            = LORA_MESH_NODE_TYPE_GENERIC;
        uint8_t   protocol            = MESH_PROTOCOL_WIFI;
        long      band                = 433E6;
        uint8_t   maxMsgRetry         = LORA_MESH_SEND_MSG_RETRY_COUNT;
        long      retryInterval       = LORA_MESH_MSG_QUEUE_TIMEOUT;
        bool      keepAlive           = true;
        long      keepAliveInterval   = LORA_MESH_KEEP_ALIVE_INTERVAL;
        uint8_t   debugLevel         = 0;
        char      pathToMaster[LORA_MESH_MAX_ROUTING_PATH_SIZE];
        uint8_t   blockNodes[LORA_MESH_MAX_BLOCK_NODES]  = {0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00};
        uint8_t   blockBroadcast[LORA_MESH_MAX_BLOCK_NODES] =  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};     
};

 typedef struct netInfo {
          uint8_t nodeId;
          uint8_t sts;
          char path[4];
   };

typedef union NET {
  netInfo meshNetwork[LORA_MESH_MAX_NETWORK_SIZE];
  char _bmsg[sizeof(netInfo)*LORA_MESH_MAX_NETWORK_SIZE];
};



class LoraWifiMesh {
  public:  
    int totalRetry = 0;
    int totalCRC = 0;
    bool netUpdate = false;
    byte Protocol = MESH_PROTOCOL_LORA;
    long Band = 433E6;
    uint8_t LocalAddress = 0;
    uint8_t dataToSend2[WIFI_MAX_MSG_SIZE];
    uint8_t dataReceived[WIFI_MAX_MSG_SIZE];
    uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    long KeepAliveInterval = LORA_MESH_KEEP_ALIVE_INTERVAL;
    NODES meshNetwork[LORA_MESH_MAX_NETWORK_SIZE];
   

    LoraWifiMesh();
    ~LoraWifiMesh();

    
    byte getRREQ(uint8_t destinationAddress);
    bool hasMsg( RECEIVED_Packet *rec, int packetSize = 0);
    byte sendMsg(uint8_t destAddr, char *, char *path = "\0", byte uni = 0xFF, byte ret = 0xFF);

    void setupNode(byte protocol, long band = 0);
    void stringSts(uint8_t sts);
    void dumpHDR(HDR_MSG msg);
    void dumpRREQ(RREQ_MSG msg);
    void dumpRREP(RREP_MSG msg);
    void dumpRTable();
    void dumpRREQTable();
    void dumpMSGTable();
    void dumpNetwork();
    bool findRoute(uint8_t destNode,char *path);
    bool setMac(char *nodeMac);


    STSCODE setConfig(NODE_CONFIGURATION nc);
    STSCODE registerNode(USER_PACKET up);
    STSCODE doMsg(RREP_Packet *ack);    
    STSCODE init(byte protocol);
    STSCODE setProtocol(byte protocol);
    STSCODE initAddress(uint8_t locAdd);
    STSCODE yield();  
    STSCODE processMsg(int packetSize, uint8_t *msg = 0x00);
    STSCODE addStaticRoute (uint8_t destAddr, char * path );
    STSCODE dropBroadcastNode (uint8_t sourceAddr, uint8_t destAddr = 0x00);
    STSCODE dropSourceNode (uint8_t sourceAddr);
    STSCODE setDebugLevel(byte DebugLevel = 0);
    STSCODE addNodeToNetwork(uint8_t nodeId, char *mac,byte protocol);
  
  private:
    byte DebugLevel = 0;
    bool KeepAlive = true;
    long RetryInterval = LORA_MESH_MSG_QUEUE_TIMEOUT;
    long _lastKeepAlive;
    long _lastReset;
    long ResetInterval = LORA_MESH_QUEUE_INTERVAL_RESET;
    uint8_t NodeType = LORA_MESH_NODE_TYPE_GENERIC;
    uint8_t MasterNode;
    uint8_t MaxMsgRetry = LORA_MESH_SEND_MSG_RETRY_COUNT;

    uint8_t _uniqRReqId = 0x00;
    uint8_t _uniqMsgId = 0x00;
    uint8_t Mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t BlockNodes[LORA_MESH_MAX_BLOCK_NODES]  = {0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00};
    uint8_t BlockBroadcast[LORA_MESH_MAX_BLOCK_NODES] =  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};   
        
    NODE_FILTER  dropNodes[LORA_MESH_MAX_DROPNODES_TABLE_SIZE];
    ROUTING_TABLE routingTable[LORA_MESH_MAX_ROUTING_TABLE_SIZE];
    QUEUE_MSG sentQueue[LORA_MESH_MSG_QUEUE_SIZE];
    RREQ_TABLE sentRREQ[LORA_MESH_RREQ_QUEUE_SIZE];
    RECEIVED_TABLE receivedQueue[LORA_MESH_RECEIVED_QUEUE_SIZE];
   
    STSCODE addRREQToQueue(uint8_t uniqueId);
    STSCODE addMSGToQueue(SEND_Packet msg);
    STSCODE removeMSGfromQueue(uint8_t uniqueId, char *_msg);
    STSCODE addRoute(uint8_t destination,char *path);
    STSCODE cleanQueues( byte queueType = LORA_MESH_QUEUE_TYPE_ANY );
    STSCODE addRRToQueue(RR_Packet rr, uint8_t uniqueId);
    STSCODE _send(char *bmsg, byte len);
    void dumpSendTo(SEND_Packet pkt);
    bool findRREQ(byte uniqueId);
    byte CRC(const char *data, byte len);
    bool checkCRC( char*,byte len,char *msg = "");
    byte getCRC( char*,byte len);

};

extern LoraWifiMesh LWMesh;

#endif
