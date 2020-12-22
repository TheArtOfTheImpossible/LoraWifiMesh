// Copyright © 2020 by antónio montez . All rights reserved.


/*!
 * @file LoraWifiMesh.cpp
 *
 * @mainpage Arduino library to implement a Mesh Library using LoRa or WiFi (ESP_NOW) protocols
 *
 * @section intro_sec Introduction
 *
 *      This a Mesh Library to connect nodes using LoRa and WiFi modens. (Does not use LoRaWan protocol/algorithms )
 *      It implement an automatic route algorithm along with manual staic routing
 *      It implement a deliver confirmation of messages, every message deliver will return ACK.
 *      The retry interval and retry count is configurable.
 *      Sending and receiving is asynchronous
 *      Nodes can be Master & Slave at the same time. can act as gateways or just as passive nodes.
 *      More than one MASTER can exist in the Network.
 *      Node can be configure by means of a OTA like message.
 *      As a technique it uses static memory allocation, preventing in this way any memory lickage .
 *      the AVR compilation result is:
 *          Sketch uses 16476 bytes (53%) of program storage space. Maximum is 30720 bytes.
 *          Global variables use 1184 bytes (57%) of dynamic memory, leaving 864 bytes for local variables. Maximum is 2048 bytes.
 *      It uses a CRC code to validate msg contents
 *      When using the WiFi, it uses the specific ESP_NOW protocol that allows for peer to peer communication,
 *      It doesn't use the normal TCP/IP on top of WiFi, there's doesn't need a home router and internet connection.
 * 
 * @section dependencies Dependencies
 *
 *      This library depends on several libraries, depending which protocl you are using LoRa or WiFi (ESP_NOW) protocols
 *      It was been tested on AVR , ESP8266 and ESP32 (Heltec LoRa Boards)
 *
 * @section author Author
 *
 *      Written by antónio montez . All rights reserved. Copyright © 2020
 *
 * @section license License
 *
 *      BSD license, all text above, and the splash screen included below,
 *      must be included in any redistribution.
 *
 * 
 * @section Disclaimer
 * 
 * This SOFTWARE PRODUCT is provided by THE PROVIDER "as is" and "with all faults." THE PROVIDER makes no representations or warranties of any kind concerning the safety,
 * suitability, lack of viruses, inaccuracies, typographical errors, or other harmful components of this SOFTWARE PRODUCT. There are inherent dangers in the use of any software,
 * and you are solely responsible for determining whether this SOFTWARE PRODUCT is compatible with your equipment and other software installed on your equipment. You are also
 * solely responsible for the protection of your equipment and backup of your data, and THE PROVIDER will not be liable for any damages you may suffer in connection with using,
 * modifying, or distributing this SOFTWARE PRODUCT
 * 
 * 
 */



#include "LoraWifiMesh.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

Global_Packet pkt;

LoraWifiMesh::LoraWifiMesh(){
};

LoraWifiMesh::~LoraWifiMesh(){  
};

void LoraWifiMesh::setupNode(byte protocol,long Band){
  
  byte localAddress = 0x00;     // address of this device  
  char _mac[6];
      
};


/*!
    @brief  Initial config function. It receives a NODE_CONFIGURATION structure with all relevant configuration parameters.
            This information will then be store in EEPROM memory .
            The contents of the configuratio, can also be receive as an OTA like message and therefore also store in EEPROM
            
    @param  NC
    
            struct NODE_CONFIGURATION which contains relevant configuration parameters

    @return STS_OK status code.

    @note   
*/



STSCODE LoraWifiMesh::setConfig(NODE_CONFIGURATION nc){
  
    LocalAddress = nc.nodeId ;
    MasterNode = nc.masterNode;
    NodeType = nc.nodeType;
    Protocol = nc.protocol;
    Band = nc.band ;
    MaxMsgRetry = nc.maxMsgRetry;
    RetryInterval =nc.retryInterval ;
    KeepAlive = nc.keepAlive ;
    KeepAliveInterval = nc.keepAliveInterval;
    DebugLevel = nc.debugLevel;
    memcpy(Mac,nc.macAddress,6);
    memcpy(BlockNodes,nc.blockNodes,LORA_MESH_MAX_BLOCK_NODES);
    memcpy(BlockBroadcast, nc.blockBroadcast,LORA_MESH_MAX_BLOCK_NODES); 
    _lastReset = millis();
    addStaticRoute(MasterNode,nc.pathToMaster);
  
    return STS_OK;
}


/*!
    @brief  set the protocol that will be used MESH_PROTOCOL_LORA or MESH_PROTOCOL_WIFI
    
    @param  byte protocol
    
            #define MESH_PROTOCOL_LORA 1
            #define MESH_PROTOCOL_WIFI 2

    @return STS_OK status code.

    @note   
*/


 STSCODE LoraWifiMesh::setProtocol(byte protocol) {
    Protocol = protocol;   
    return STS_OK;
 };
 
STSCODE LoraWifiMesh::_send(char *_bmsg, byte len){
    byte result;
    String ss;
    
    switch (Protocol) {
      case MESH_PROTOCOL_LORA :
         #if !defined(ESP32) || defined(_HELTEC_)
                LoRa.beginPacket();   
                for (int i = 0;i<len;i++){
                  LoRa.write(_bmsg[i]);
                }
                LoRa.endPacket();
          #endif
                break;
            #if defined(ESP8266) ||  defined(ESP32)
                case MESH_PROTOCOL_WIFI:
                        memcpy(dataToSend2,_bmsg,len);
    
                        byte tt = (byte) random(1,5);
                        delay(tt);            
                        result = esp_now_send(broadcastAddress, dataToSend2, len); 
                        if (result == 0 /*ESP_OK*/) {
                        }
                        else {
                            Serial.println("Error sending the data");
                        }
       
                          break;
            #endif
    }
    
  return STS_OK;
}

/*!
    @brief  Main processing Function 
            
            It decodes the bytes received beloing to the differents messages types
            
            #define LORA_MESH_MSG_REGISTRATION 1            // node registration message
            #define LORA_MESH_MSG_USER 2                    // user message
            #define LORA_MESH_MSG_RREQ  4                   // Route Request message
            #define LORA_MESH_MSG_RREP  8                   // Route Repply message
            #define LORA_MESH_MSG_RERR  16                  // Route error message
            #define LORA_MESH_MSG_SENDTO  32                // Specific internal Send message
            #define LORA_MESH_MSG_ACK 64                    // ACK message
                
    @param  int packetSize
            Message size
            
    @param  uint8_t *msg
    
            Message 
           
    @return status code.
    
            #define STS_OK 1
            #define ERR_MSG_NOT_FOR_ME -1
            #define ERR_DROP_DUE_TO_RULES -2
            #define ERR_DUP_RREQ -3
            #define ERR_DROP_ROUTING -4
            #define ERR_RREQ_CRC_ERR -5

    @note   
*/

 
STSCODE LoraWifiMesh::processMsg(int packetSize, uint8_t *msg){
  Global_Packet pkt;
  RR_Packet rr;
  char node11=0;
  char node22=0;
  byte cnt;
  char c;
  byte _crc0;
  byte _crc1;
  bool _checkCrc;
  byte _len;
  byte _hdrType;
  byte _size;

  
  memset(&pkt, 0, sizeof(Global_Packet));
  cnt = 0;

  if (LWMesh.Protocol == MESH_PROTOCOL_WIFI){
          cnt = packetSize;
          memcpy (pkt._bmsg,msg,packetSize);
          switch (pkt._send._hdr.hdrType) {
            case  LORA_MESH_MSG_RREQ :_size = sizeof(RREQ_DATAGRAM); break;
            case  LORA_MESH_MSG_RREP :_size = sizeof(RREP_DATAGRAM); break;
            case  LORA_MESH_MSG_SENDTO :_size = sizeof(SEND_DATAGRAM); break;
            case  LORA_MESH_MSG_ACK :_size = sizeof(RREP_DATAGRAM); break;
          }
          if ((cnt > _size)) {
                if ((DebugLevel <=  2) && (DebugLevel >0)) {
                  Serial.print(F("Msg to big:"));
                  Serial.println(cnt);
                }
                return ERR_NO_MSG;
          }
          if (packetSize == 0 ) return ERR_NO_MSG;
      }
 else {
        
 #if !defined(ESP32) || defined(_HELTEC_)
     
      while (LoRa.available())
        {
             c = (char)LoRa.read();
             if (cnt == 0 ) {
               _hdrType = c;
             }
             if (cnt == 1) {
               _len = c;
             }
             
             if (cnt == 2 ) {
                  switch (_hdrType) {
                    case  LORA_MESH_MSG_RREQ :_size = sizeof(RREQ_DATAGRAM); break;
                    case  LORA_MESH_MSG_RREP :_size = sizeof(RREP_DATAGRAM); break;
                    case  LORA_MESH_MSG_SENDTO :_size = sizeof(SEND_DATAGRAM); break;
                    case  LORA_MESH_MSG_ACK :_size = sizeof(RREP_DATAGRAM); break;
                  }
       
             }
             pkt._bmsg[cnt++] = c;
      
             if ((cnt > _size)) {
                if ((DebugLevel <=  2) && (DebugLevel >0)) {
                  Serial.print(F("Msg to big:"));
                  Serial.println(cnt);
                }
                return ERR_NO_MSG;
             }
        };
    #endif
 }

if (cnt == 0 ) return ERR_NO_MSG;
                  

if ((DebugLevel <=  1) && (DebugLevel >0)) {
      Serial.print( F("Message type :" ));        
      Serial.print ( _hdrType );        
      Serial.print ( F(" len:" ));        
      Serial.print ( _len);        
      Serial.print ( F(" Pack:" ));        
      Serial.print ( packetSize);        
      Serial.print ( F(" size:" ));        
      Serial.print ( _size);        
      Serial.println();
 }
  
  byte destinationNode = pkt._send._hdr.destinationNode;
  byte sourceNode = pkt._send._hdr.sourceNode;
  byte hdrType = pkt._send._hdr.hdrType;
  byte destNode = pkt._send._send.destinationNode;
  byte sourcNode = pkt._send._send.sourceNode;
  byte type = pkt._send._send.type;
  byte uniq = pkt._send._send.uniqueId;

/*
  Serial.println("----------------RECEIVING-------------");
  Serial.print(F("Source: "));
  Serial.print((char)sourceNode);
  Serial.print(F(" dest: "));
  Serial.print((char)destinationNode);
  Serial.print(F(" type: "));
  Serial.println(hdrType);
 
  Serial.print(F("Source: "));
  Serial.print((char)sourcNode);
  Serial.print(F(" dest: "));
  Serial.print((char)destNode);
  Serial.print(F(" type: "));
  Serial.print(type);
  Serial.print(F(" uniq: "));
  Serial.println(uniq);
      
  */
  
       
  _checkCrc = checkCRC(pkt._bmsg,sizeof(RREQ_Packet),"END RREQ");
  if (!_checkCrc) { 

       totalCRC++;
       
  
  Serial.println(F("CRC ERROR"));
  return ERR_RREQ_CRC_ERR;
 }
 
  if ((DebugLevel <=  1 ) && (DebugLevel >0)) {
   
        Serial.print(F("Bytes Received:"));
        Serial.println(cnt);
        
        Serial.print(F("Source: "));
        Serial.print((char)sourceNode);
        Serial.print(F(" dest: "));
        Serial.print(destinationNode);
        Serial.print(F(" type: "));
        Serial.println(hdrType);
       
        Serial.print(F("Source: "));
        Serial.print((char)sourcNode);
        Serial.print(F(" dest: "));
        Serial.print(destNode);
        Serial.print(F(" type: "));
        Serial.print(type);
        Serial.print(F(" uniq: "));
        Serial.println(uniq);
  }
 
  if ((destinationNode != LocalAddress) && (destinationNode != 0xFF) ) {
    if ((DebugLevel <=  1 ) && (DebugLevel >0)) {
           Serial.println(F("Drop message.Not for me"));
    }
    return ERR_MSG_NOT_FOR_ME;
  }
  
  switch (LocalAddress){
/*    case 0x31 :
          if (((sourceNode == 0x34) || (sourceNode == 0x35) || (sourceNode == 0x36) ||
              (sourceNode == 0x33) )  && (destinationNode == 0xFF)) {
              if ((DebugLevel <=  1 ) && (DebugLevel >0)) {
                  Serial.println(F(" Drop message.Due to Broadcast Filtering Rules"));
              }
             return DROP_MSG_DUE_TO_FILTER_RULES;
          }
        break;
    case 0x32 :
          if (((sourceNode == 0x34) || (sourceNode == 0x34) || (sourceNode == 0x35) ) && (destinationNode == 0xFF)) {
              if ((DebugLevel <=  1 ) && (DebugLevel >0)) {
                  Serial.println(F(" Drop message.Due to Broadcast Filtering Rules"));
              }
             return DROP_MSG_DUE_TO_FILTER_RULES;
        }
        
        break;*/
   /* case 0x33 :
          if (((sourceNode == 0x31) || ((sourceNode > 0x35) && (sourceNode <= 0x40)) )  && (destinationNode == 0xFF)) {
    //          if ( (DebugLevel <=  1 )  && (DebugLevel >0)){
                  Serial.println(F(" Drop message.Due to Broadcast Filtering Rules"));
    //          }
             return DROP_MSG_DUE_TO_FILTER_RULES;
        }
        break;
    case 0x34 :
          if ((((sourceNode == 0x31) || (sourceNode == 0x32))) ||  ( (sourceNode > 0x36) || ((sourceNode <= 0x40) ))  && (destinationNode == 0xFF)) {
              if ((DebugLevel <=  1 ) && (DebugLevel >0)) {
                  Serial.println(F("Drop message.Due to Broadcast Filtering Rules"));
              }
             return DROP_MSG_DUE_TO_FILTER_RULES;
          }
        break;
      case 0x35 :
          if ((((sourceNode == 0x31) || (sourceNode == 0x32))) ||  ( (sourceNode > 0x37) || ((sourceNode <= 0x40) ))   && (destinationNode == 0xFF)) {
              if ((DebugLevel <=  1 ) && (DebugLevel >0)) {
                  Serial.println(F("Drop message.Due to Broadcast Filtering Rules"));
              }
             return DROP_MSG_DUE_TO_FILTER_RULES;
          }
        break;
          case 0x36 :
          if (((sourceNode >= 0x31) || (sourceNode <= 0x34))  && (destinationNode == 0xFF)) {
              if ((DebugLevel <=  1 ) && (DebugLevel >0)) {
                  Serial.println(F("Drop message.Due to Broadcast Filtering Rules"));
              }
             return DROP_MSG_DUE_TO_FILTER_RULES;
          }
        break;
          case 0x37 :
          if (((sourceNode >= 0x31) || (sourceNode <= 0x34)) && (destinationNode == 0xFF)) {
              if ((DebugLevel <=  1 ) && (DebugLevel >0)) {
                  Serial.println(F("Drop message.Due to Broadcast Filtering Rules"));
              }
             return DROP_MSG_DUE_TO_FILTER_RULES;
          }
        break;
          case 0x38 :
          if (((sourceNode >= 0x31) || (sourceNode <= 0x35)) && (destinationNode == 0xFF)) {
              if ((DebugLevel <=  1 ) && (DebugLevel >0)) {
                  Serial.println(F("Drop message.Due to Broadcast Filtering Rules"));
              }
             return DROP_MSG_DUE_TO_FILTER_RULES;
          }
        break;
          case 0x39 :
          if (((sourceNode >= 0x31) || (sourceNode <= 0x36))  && (destinationNode == 0xFF)) {
              if ((DebugLevel <=  1 ) && (DebugLevel >0)) {
                  Serial.println(F("Drop message.Due to Broadcast Filtering Rules"));
              }
             return DROP_MSG_DUE_TO_FILTER_RULES;
          }
        break;

          case 0x40 :
          if (((sourceNode == 0x31))  && (destinationNode == 0xFF)) {
              if ((DebugLevel <=  1 ) && (DebugLevel >0)) {
                  Serial.println(F("Drop message.Due to Broadcast Filtering Rules"));
              }
             return DROP_MSG_DUE_TO_FILTER_RULES;
          }
        break;
        */
  }
 


  switch (pkt._send._hdr.hdrType) {
  
        case (char) LORA_MESH_MSG_RREQ :
        
               RREQ_Packet rreq;
               memset(&rreq, 0, sizeof(RREQ_Packet));
               memcpy((char*)rreq._bmsg,(char*)pkt._bmsg,sizeof(RREQ_Packet));

               if ((DebugLevel <=  2 )  && (DebugLevel >0)){
                  LWMesh.dumpHDR(rreq._msg._hdr);
                  LWMesh.dumpRREQ(rreq._msg._rreq);
               }

               if (findRREQ(rreq._msg._rreq.uniqueId)) {
                  if ((DebugLevel <=  1 )  && (DebugLevel >0)){
                        Serial.println(F("Drop message.Duplicated RREQ"));
                   }
                  return ERR_DUP_RREQ;
               }


              for ( byte k=0; k <LORA_MESH_MAX_ROUTING_PATH_SIZE; k++) {
                if ( rreq._msg._rreq.path[k] ==  LocalAddress ) {
                  return ERR_DUP_RREQ;
                }
              }
               if (rreq._msg._rreq.destinationNode == LocalAddress) {
                    RREP_Packet rrep;
                    memset(&rrep, 0, sizeof(RREP_Packet));
                    memcpy((char*)rrep._bmsg,(char*)pkt._bmsg,sizeof(RREP_Packet));
      
                    _checkCrc = checkCRC(rrep._bmsg,sizeof(RREQ_Packet),"END RREQ");
                    if (!_checkCrc) { return ERR_RREQ_CRC_ERR;}
                    
                         if ((DebugLevel <=  2 )  && (DebugLevel >0)){
                               Serial.print(F("Receiving RREQ from :["));
                               Serial.print(rrep._msg._rrep.path);
                               Serial.println(F("]"));
                         }
                        rrep._msg._hdr.sourceNode = LocalAddress;
                        rrep._msg._hdr.destinationNode =  pkt._rrep._hdr.sourceNode;
                        rrep._msg._hdr.hdrType = LORA_MESH_MSG_RREP; 
                        
                        rrep._msg._rrep.type  = LORA_MESH_MSG_RREP;
                        rrep._msg._rrep.sourceNode  = pkt._rrep._rrep.sourceNode;
                        rrep._msg._rrep.destinationNode  = pkt._rrep._rrep.destinationNode;
                        rrep._msg._rrep.uniqueId  =  pkt._rrep._rrep.uniqueId;
                        sprintf(rrep._msg._rrep.path,"%s%c\0",rrep._msg._rrep.path,LocalAddress);                        
                        rrep._msg._hdr.len = sizeof(RREP_DATAGRAM);
                        rrep._msg._hdr._crc = getCRC(rrep._bmsg,sizeof(RREP_DATAGRAM));  // GET CRC
                        
                        if ((DebugLevel <=  4 ) && (DebugLevel >0)){
                            memcpy(rr._bmsg,rrep._bmsg,sizeof(RREP_Packet));
                            addRRToQueue(rr, rrep._msg._rrep.uniqueId);
                        }
                        else {
                            delay(2);
                            _send (rrep._bmsg, sizeof(RREP_DATAGRAM));                       
                        }
                        
                        if (( DebugLevel <=  1 )  && (DebugLevel >0)){
                           Serial.println(F("Sending RouteReply (RREP)"));
                        }

                        if ((DebugLevel <=  2 )  && (DebugLevel >0)){
                            LWMesh.dumpHDR(rrep._msg._hdr);
                            LWMesh.dumpRREP(rrep._msg._rrep);
                        }
                   
       
                  }    //------------  RE-REROUTE RREQ Request
                  else {
      
                      _checkCrc = checkCRC(rreq._bmsg,sizeof(RREQ_Packet),"RE-ROUTE RREQ");
                      if (!_checkCrc) { return ERR_RREQ_CRC_ERR;}
                    
                      addRREQToQueue (rreq._msg._rreq.uniqueId);
                      if ((DebugLevel <=  3 ) && (DebugLevel >0)){
                         dumpRREQTable();
                      }

                      if (DebugLevel <=  2) {
                          Serial.print(F("RE-Broadcast RREQ :"));
                          Serial.print((char)pkt._rreq._rreq.uniqueId);
                          Serial.print(F(" path:["));
                          Serial.print(pkt._rreq._rreq.path);
                          Serial.println(F("]"));
                      }                  

           
                      rreq._msg._hdr.sourceNode = LocalAddress;
                      rreq._msg._hdr.destinationNode = LORA_MESH_BROADCAST_ADDRESS;
                      rreq._msg._hdr.hdrType = LORA_MESH_MSG_RREQ; 

                      sprintf(rreq._msg._rreq.path,"%s%c\0",rreq._msg._rreq.path,LocalAddress);
     
                      rreq._msg._hdr.len = sizeof(RREQ_Packet);
                      rreq._msg._hdr._crc = getCRC(rreq._bmsg,sizeof(RREQ_Packet));  // GET CRC

                      if ((DebugLevel <=  4 ) && (DebugLevel >0)) {
                            memcpy(rr._bmsg,rreq._bmsg,sizeof(RREQ_Packet));
                            addRRToQueue(rr, rreq._msg._rreq.uniqueId);
                      }
                      else {
                           _send (rreq._bmsg, sizeof(RREQ_Packet));
                       } 
                      if ( (DebugLevel <=  2) && (DebugLevel >0)){
                          LWMesh.dumpHDR(rreq._msg._hdr);
                          LWMesh.dumpRREQ(rreq._msg._rreq);
                      }
                  }
          
              break;
       
        case LORA_MESH_MSG_RREP :
      
                   RREP_Packet rrep;
                   char ss[LORA_MESH_MAX_ROUTING_PATH_SIZE];
                   memset(&rrep, 0, sizeof(RREQ_Packet));
                   memset(&ss, 0, LORA_MESH_MAX_ROUTING_PATH_SIZE);
              
                   memcpy(ss,rrep._msg._rrep.path,LORA_MESH_MAX_ROUTING_PATH_SIZE);
                   memcpy((char*)rrep._bmsg,(char*)pkt._bmsg,sizeof(RREP_Packet));
              
                   _checkCrc = checkCRC(rrep._bmsg,sizeof(RREP_Packet),"REROUTE-RREP");
                   if (!_checkCrc) { return ERR_RREQ_CRC_ERR;}
                 
                   if ((DebugLevel <=  3) && (DebugLevel >0)) {
                      LWMesh.dumpHDR(pkt._send._hdr);
                      LWMesh.dumpRREP(pkt._rrep._rrep);
                   }
  
                   char node1;
                   char node2;
                   for (char i = 0; i < LORA_MESH_MAX_ROUTING_PATH_SIZE-1; i++) {
                       node1 = pkt._rrep._rrep.path[i];
                       node2 = pkt._rrep._rrep.path[i+1];
                       if (rrep._msg._hdr.destinationNode == LocalAddress) break;
                   }
              
                  if (rrep._msg._rrep.sourceNode == LocalAddress) {
                  
                        for(byte slot0 = 0; slot0<LORA_MESH_RECEIVED_QUEUE_SIZE; slot0++) {
                             if (receivedQueue[slot0].sts == LORA_MESH_QUEUE_FREE) {
                                receivedQueue[slot0].sts = LORA_MESH_QUEUE_USED;
                                receivedQueue[slot0]._pkt._pkt.msgId = rrep._msg._hdr.msgId;
                                receivedQueue[slot0]._pkt._pkt.sts = STS_ROUTE_RETURNED;
                                memcpy(receivedQueue[slot0]._pkt._pkt.msg,rrep._msg._rrep.path,LORA_MESH_MAX_ROUTING_PATH_SIZE);
                                break;
                             }
                        }
                        
                        rrep._msg._hdr.len = sizeof(RREP_Packet);
                        addRoute(rrep._msg._rrep.destinationNode,rrep._msg._rrep.path);
                        
                        if ((DebugLevel <=  1 ) && (DebugLevel >0)) {
                             dumpRTable();
                             dumpMSGTable();
                        }
                        return STS_ROUTE_RETURNED;
                  }

                  rrep._msg._hdr.sourceNode = node2;
                  rrep._msg._hdr.destinationNode = node1;
                  rrep._msg._rrep.type  = LORA_MESH_MSG_RREP;

                  rrep._msg._hdr.len = sizeof(RREP_Packet);
                  rrep._msg._hdr._crc = getCRC(rrep._bmsg,sizeof(RREP_Packet));  // GET CRC

                  if ((DebugLevel <=  2 ) && (DebugLevel >0)){
                      Serial.print(F("RE-Broadcast RREP :"));
                      Serial.print(rrep._msg._rrep.uniqueId);
                      Serial.print(F(" to node:"));
                      Serial.print((char)node1);
                      Serial.print(F(" path:["));
                      Serial.print(pkt._rrep._rrep.path);
                      Serial.println(F("]"));
                  }

                  if ( (DebugLevel <=  4 ) && (DebugLevel >0)){
                        memcpy(rr._bmsg,rrep._bmsg,sizeof(RREP_Packet));
                        addRRToQueue(rr, rrep._msg._rrep.uniqueId);
                  }
                  else {
                          _send (rrep._bmsg, sizeof(RREP_DATAGRAM));
                  }
                        
                  if ((DebugLevel <=  2) && (DebugLevel >0)){
                          LWMesh.dumpHDR(rrep._msg._hdr);
                          LWMesh.dumpRREP(rrep._msg._rrep);
                  } 
                    
             break;
         
         case LORA_MESH_MSG_ACK:
      
                 RREP_Packet _rrep;   
                 memset(&_rrep, 0, sizeof(RREP_Packet));
                 memcpy((char*)_rrep._bmsg,(char*)pkt._bmsg,sizeof(RREP_Packet));
            
                 _checkCrc = checkCRC(_rrep._bmsg,sizeof(RREP_Packet),"RE-ACK");
                 if (!_checkCrc) { return ERR_RREQ_CRC_ERR;}
            
                 char _node0;
                 char _node1;
                 for (char i = 0; i < LORA_MESH_MAX_ROUTING_PATH_SIZE-1; i++) {
                     _node0 = pkt._rrep._rrep.path[i];
                     _node1 = pkt._rrep._rrep.path[i+1];
                     if (_rrep._msg._hdr.destinationNode == LocalAddress) break;
                 }
            
                    if (_rrep._msg._rrep.sourceNode == LocalAddress) {    
                          removeMSGfromQueue(_rrep._msg._rrep.uniqueId, _rrep._msg._rrep.msg);
    
                          if ((DebugLevel <=  1) && (DebugLevel >0)){
                            Serial.print(F(" ACK received: "));
                            Serial.println((char)_node0);
                          }
    
                          return STS_MSG_ACK_RECEIVED;
                      }
                       
                      _rrep._msg._hdr.hdrType = LORA_MESH_MSG_ACK; 
                      _rrep._msg._hdr.sourceNode = _node1;
                      _rrep._msg._hdr.destinationNode = _node0;
                      _rrep._msg._rrep.type  = LORA_MESH_MSG_ACK;
              
                      _rrep._msg._hdr.len = sizeof(RREP_Packet);
                      _rrep._msg._hdr._crc = getCRC(_rrep._bmsg,sizeof(RREP_Packet));  // GET CRC
     
                     if ( (DebugLevel <=  4 ) && (DebugLevel >0)){
                            memcpy(rr._bmsg,_rrep._bmsg,sizeof(RREP_Packet));
                            addRRToQueue(rr, _rrep._msg._rrep.uniqueId);
                      }
                      else {
                            _send (_rrep._bmsg, sizeof(RREP_Packet));
                      }

                      if ((DebugLevel <=  1) && (DebugLevel >0)){
                          Serial.print(F(" RE-ACK to: "));
                          Serial.println((char)_node0);
                      }

             break;
             
        case LORA_MESH_MSG_RERR :
              break;
        case LORA_MESH_MSG_SENDTO:
      
                    RREP_Packet ack;   
                    memset(&ack, 0, sizeof(RREP_Packet));
                    memcpy((char*)ack._bmsg,(char*)pkt._bmsg,sizeof(RREP_Packet));
                  
                    _checkCrc = checkCRC(pkt._bmsg,sizeof(SEND_DATAGRAM),"SENDTO");
                    if (!_checkCrc) { return ERR_RREQ_CRC_ERR;}
              
                    for (int i = 0; i < LORA_MESH_MAX_ROUTING_PATH_SIZE; i++) {
                         if (pkt._send._send.path[i] == '\0') break;
                         node11 = pkt._send._send.path[i];
                    }
                
                    if (destinationNode != LocalAddress) {
                        return ERR_DROP_ROUTING;
                    }
                
                    if (node11 == LocalAddress){                 
                         char _nod0;
                         char _nod1;
                         for (char i = 0; i < LORA_MESH_MAX_ROUTING_PATH_SIZE-1; i++) {
                             _nod0 = pkt._send._send.path[i];
                             _nod1 = pkt._send._send.path[i+1];
                             if (pkt._send._hdr.destinationNode == LocalAddress) break;
                         }
                               
                          ack._msg._hdr.hdrType = LORA_MESH_MSG_ACK; 
                          ack._msg._hdr.sourceNode = _nod1;
                          ack._msg._hdr.destinationNode = _nod0;
                          ack._msg._rrep.type  = LORA_MESH_MSG_ACK;

                          byte retSts = doMsg(&ack);
                      
                          ack._msg._hdr.len = sizeof(RREP_Packet);
                          ack._msg._hdr._crc = getCRC(ack._bmsg,sizeof(RREP_Packet));  // GET CRC
                          
                          if ((DebugLevel <=  4 ) && (DebugLevel >0)) {
                                memcpy(rr._bmsg,ack._bmsg,sizeof(RREP_Packet));
                                addRRToQueue(rr, ack._msg._rrep.uniqueId);
                          }
                          else {
                              delay(1);
                              _send (ack._bmsg, sizeof(RREP_Packet));
                              delay(2);
                              _send (ack._bmsg, sizeof(RREP_Packet));
                          }

                          for(byte slot0 = 0; slot0<LORA_MESH_RECEIVED_QUEUE_SIZE; slot0++) {
                               if (receivedQueue[slot0].sts == LORA_MESH_QUEUE_FREE) {
                                  receivedQueue[slot0].sts = LORA_MESH_QUEUE_USED;
                                  receivedQueue[slot0]._pkt._pkt.msgId = pkt._send._hdr.msgId;
                                  receivedQueue[slot0]._pkt._pkt.sts = STS_RECEIVED;
                                  memcpy(receivedQueue[slot0]._pkt._pkt.msg,pkt._send._send.msg,LORA_MESH_MAX_MSG_SIZE);
                                  break;
                               }
                          }

                          return STS_MSG_REACH_DESTINATION;
                    }
                  
                    for (int i = 0; i < LORA_MESH_MAX_ROUTING_PATH_SIZE-1; i++) {
                         node11 = pkt._send._send.path[i];
                         node22 = pkt._send._send.path[i+1];
                         if (node11 == LocalAddress) break;
                    }

                    if ((DebugLevel <=  2) && (DebugLevel >0)){
                        Serial.print (F("RE-ROUTE MSG to Node : "));
                        Serial.print((char)node22);
                        Serial.print(F(" path:["));
                        Serial.print( pkt._send._send.path );
                        Serial.print(F("] msg:["));
                        Serial.print(pkt._send._send.msg);
                        Serial.println(F("]"));
                    }
                    
                    pkt._send._hdr.sourceNode = LocalAddress;
                    pkt._send._hdr.destinationNode = node22;
                
                    pkt._send._hdr.len = sizeof(SEND_DATAGRAM);
                    pkt._send._hdr._crc = getCRC(pkt._bmsg,sizeof(SEND_DATAGRAM));  // GET CRC
                    
                    _send (pkt._bmsg, sizeof(SEND_DATAGRAM));
 
            break;
                           
      
        default : 
                    if ((DebugLevel <=  2 ) && (DebugLevel >0)){
                          Serial.print(F("Wrong hdrType:")); 
                          Serial.print(pkt._send._hdr.hdrType); 
                    }
                    break;
        
       }
        
       return STS_OK;
}

STSCODE LoraWifiMesh::doMsg(RREP_Packet *ack){

  USER_PACKET up;
  byte retSts;
 
  memcpy(up._b, ack->_msg._rrep.msg ,sizeof(NODE_REGISTRATION));
  memcpy(up._reg.path, ack->_msg._rrep.path ,LORA_MESH_MAX_ROUTING_PATH_SIZE);

  switch (up._reg.userMsgType) {
    case LORA_MESH_MSG_REGISTRATION : 
                retSts = STS_MSG_ACK_REGISTRATION_DONE; 
                registerNode(up);
                break;
    case LORA_MESH_MSG_USER :  break;
    default : retSts = STS_DELIVERED; break;
  }

  up._reg.userMsgType = STS_MSG_ACK_REGISTRATION_DONE;
  memcpy(ack->_msg._rrep.msg ,up._b, sizeof(NODE_REGISTRATION));
  
  return retSts;
}


bool LoraWifiMesh::setMac(char *_mac){
  memcpy(Mac,_mac,6);
}


STSCODE LoraWifiMesh::registerNode(USER_PACKET up){

    byte _nodeId = up._reg.nodeId;
    byte slot;

    //--- Found registration, update it -----
    
    for(slot = 0; slot<LORA_MESH_MAX_NETWORK_SIZE; slot++) {
        if (meshNetwork[slot].nodeId == _nodeId){
            memcpy(meshNetwork[slot].path,up._reg.path,LORA_MESH_MAX_ROUTING_PATH_SIZE);
            meshNetwork[slot].sts = LORA_MESH_NODE_REGISTERED;
            memcpy(meshNetwork[slot].macAddress,up._reg.macAddress,6);
            meshNetwork[slot].lastKeepAlive = millis();
            #if defined(ESP8266) || defined (ESP32)
                  meshNetwork[slot].RSSI = WiFi.RSSI();
            #endif

            netUpdate = true;
            
            return STS_OK;
        }
    }
    
    //--- Not Found registration, create it -----

    for(slot = 0; slot<LORA_MESH_MAX_NETWORK_SIZE; slot++) {
        if (meshNetwork[slot].sts == 0x00){
            meshNetwork[slot].nodeId = _nodeId;
            memcpy(meshNetwork[slot].path,up._reg.path,LORA_MESH_MAX_ROUTING_PATH_SIZE);
            meshNetwork[slot].sts = LORA_MESH_NODE_REGISTERED;
            memcpy(meshNetwork[slot].macAddress,up._reg.macAddress,6);
            meshNetwork[slot].lastKeepAlive = millis();
            #if defined(ESP8266) || defined (ESP32)
                  meshNetwork[slot].RSSI = WiFi.RSSI();
            #endif
            
            netUpdate = true; 
            return STS_OK;
        }
    }

    if (slot >= LORA_MESH_MAX_NETWORK_SIZE) return NETWORK_QUEUE_FULL;

}

/*!
    @brief  Defines the debug level returned.
   
            Values from 0 to 3.
            1 to 3 will print several debug messages in the Serial
            
    @param  byte debugLevel
    
    @return STS_OK status code.

    @note   
*/

STSCODE LoraWifiMesh::setDebugLevel(byte debugLevel){
    DebugLevel = debugLevel; 
    if (( DebugLevel <=  1 )  && (DebugLevel >0)){ Serial.print("Debug Level: "); Serial.println(DebugLevel); }
    return STS_OK;
}


/*!
    @brief   LoraWifiMesh::stringSts(uint8_t sts)
    
             Helpful function to print a descriprion of the error messages
             
    
    @param  uint8_t sts
            
            Successul messages have values > 0 
            Error messages hace values < 0
            
            //------- success codes 
      
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

            //------- error codes      
               
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

    @return STS_OK status code.

    @note   
*/


void LoraWifiMesh::stringSts(uint8_t sts){
    switch (sts){

      // sucess codes 

      case 0 : Serial.print(F("UNKNOWN"));break;
      case 1 : Serial.print(F("OK"));break;
      case 2 :  if (VT200)
                  Serial.print(F("\u001b[32mDELIVERED\u001b[37m"));
                else    Serial.print(F("DELIVERED"));
                  break;
      
      case 4 :  if (VT200)
                Serial.print(F("\u001b[34mRECEIVED\u001b[37m"));
              else Serial.print(F("RECEIVED")); break;
      
      case 8 :  if (VT200) Serial.print(F("\u001b[31mTIMEOUT\u001b[37m"));
                else Serial.print(F("TIMEOUT"));break;

       case 16 :  if (VT200) Serial.print(F("\u001b[31mCRC_ERR\u001b[37m"));
                  else Serial.print(F("CRC_ERR")); break;
       case 32 :  if (VT200) Serial.print(F("\u001b[35mROUTE_RETURNED\u001b[37m"));
                  else Serial.print(F("ROUTE_RETURNED"));break;
      
       case 40 :  Serial.print(F("MSG_REACH_DESTINATION"));break;
       case 41 :  Serial.print(F("MSG_ACK_RECEIVED"));break;

       case 42 : Serial.print(F("REGISTRATION_DONE")); break;
       
      // error codes     
       case -1 :  Serial.print(F("\u001b[31mCRC_ERR\u001b[37m"));break;
       case -50 :  Serial.print(F("ROUTING_QUEUE_FULL"));break;
       case -51 :  Serial.print(F("DROPNODES_QUEUE_FULL"));break;
       case -52 :  Serial.print(F("RREQ_QUEUE_FULL"));break;
       case -53 :  Serial.print(F("MSG_QUEUE_FULL"));break;

       case -70 :  Serial.print(F("NO_MSG"));break;
       case -71 :  Serial.print(F("MSG_NOT_FOR_ME"));break;
       case -72 :  Serial.print(F("DROP_DUE_TO_RULES"));break;
       case -73 :  Serial.print(F("DUP_RREQ"));break;
       case -74 :  Serial.print(F("DROP_ROUTING"));break;
       case -75 :  Serial.print(F("RREQ_CRC_ERR"));break;
       case -90 :  Serial.print(F("DROP_MSG_DUE_TO_FILTER_RULES"));break;

       case -100 :  Serial.print(F("ERR_CANNOT_SEND_TO_SELF"));break;
       case -101 :  Serial.print(F("ERR_CANNOT_ROUTE_TO_SELF"));break;

    }
}


/*!
    @brief  LoraWifiMesh::hasMsg( RECEIVED_Packet *rec, int packetSize)
        
            Main function to receive messages
            Either "normal" received messages, either ACK and TIMEOUT messages
            
         
            
    @param  RECEIVED_Packet *rec
    
            typedef struct RECEIVED_MSG {
                uint8_t msgId;
                STSCODE sts;
                char msg[LORA_MESH_MAX_MSG_SIZE];
            };

            typedef union RECEIVED_Packet{
                    struct RECEIVED_MSG _pkt;
                    char _bmsg[sizeof(RECEIVED_MSG)];
            };
            
    @param  int packetSize

            
            Bytes received
            
    @return STS_OK status code.

    @note   
*/


bool LoraWifiMesh::hasMsg( RECEIVED_Packet *rec, int packetSize){
  bool retSts = false;
  
  if (LWMesh.processMsg(packetSize,0x00) < 0 ) return retSts;
 
  for(byte slot = 0; slot<LORA_MESH_RECEIVED_QUEUE_SIZE; slot++) {
         if (receivedQueue[slot].sts == LORA_MESH_QUEUE_USED ) {
            receivedQueue[slot].sts = LORA_MESH_QUEUE_FREE;
            rec->_pkt.msgId = receivedQueue[slot]._pkt._pkt.msgId;
            rec->_pkt.sts = receivedQueue[slot]._pkt._pkt.sts;
            memcpy(rec->_pkt.msg,receivedQueue[slot]._pkt._pkt.msg,LORA_MESH_MAX_MSG_SIZE);
            retSts = true;
            break;
         }
   }  

  return retSts;
};


byte LoraWifiMesh::CRC(const char *data, byte len) {
  byte crc = 0x00;
  while (len--) {
    byte extract = *data++;
    for (byte tempI = 8; tempI; tempI--) {
      byte sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}

bool LoraWifiMesh::findRoute(uint8_t destNode,char *path){
     bool retSts = false;
     byte slot = 0;
     for(slot = 0; slot<LORA_MESH_MAX_ROUTING_TABLE_SIZE; slot++) {
            if (( routingTable[slot].sts == LORA_MESH_QUEUE_USED ) && (routingTable[slot].destNode == destNode)){
              strncpy(path,routingTable[slot].path,LORA_MESH_MAX_ROUTING_PATH_SIZE);
              retSts = true;
              break;
            }
     }
     return retSts;
}

STSCODE LoraWifiMesh::dropSourceNode (uint8_t _sourceAddr){
    return dropBroadcastNode (_sourceAddr,0x00);
}

STSCODE LoraWifiMesh::dropBroadcastNode (uint8_t _sourceAddr, uint8_t _destAddr){
   byte slot = 0;
   for(slot = 0; slot<LORA_MESH_MAX_DROPNODES_TABLE_SIZE; slot++) {
        if (( dropNodes[slot]._sts == LORA_MESH_QUEUE_FREE )){
              dropNodes[slot]._sts = LORA_MESH_QUEUE_USED;
              dropNodes[slot]._sourceAddr = _sourceAddr;
              dropNodes[slot]._destAddr = _destAddr;
              return STS_OK;
         }
   }
   if (slot >= LORA_MESH_MAX_DROPNODES_TABLE_SIZE) return DROPNODES_QUEUE_FULL;
}


/*!
    @brief  LoraWifiMesh::addNodeToNetwork(uint8_t nodeAddr, char *mac,  byte protocol)
    
            Main function to registered a node and add it to the mesh Network
            This isn't mandatory per see. Nodes can see send and receive messages... but to be visible in the network they need to be registered with the MASTER node.
            
            
    @param  uint8_t nodeAddr
    
    @param  char *mac
    
            device mac Address

    @param  byte protocol
            
            #define MESH_PROTOCOL_LORA 1
            #define MESH_PROTOCOL_WIFI 2

    @return STS_OK status code.

    @note   
*/


STSCODE LoraWifiMesh::addNodeToNetwork(uint8_t nodeAddr, char *mac,  byte protocol){
    USER_PACKET up;
    NODE_REGISTRATION nr;
    char _p[LORA_MESH_MAX_ROUTING_PATH_SIZE];

    initAddress(nodeAddr);
    setMac(mac);
    setProtocol(protocol);
  
    memset(&up, 0x00, sizeof(USER_PACKET));
    memset(&nr, 0x00, sizeof(NODE_REGISTRATION));
    
    nr.userMsgType = LORA_MESH_MSG_REGISTRATION;
    nr.nodeId = nodeAddr;  
    memcpy(nr.macAddress, mac, 6);
    memcpy(&up._b, &nr , sizeof(NODE_REGISTRATION));
    

  bool found = LWMesh.findRoute(MasterNode, _p);
  if (found) {
    byte _id = LWMesh.sendMsg(MasterNode, up._b,_p);
    if (_id < 0) LWMesh.stringSts (_id);
  } else {
    LWMesh.getRREQ(MasterNode);
  }

  return STS_OK; 
}


/*!
    @brief  LoraWifiMesh::addStaticRoute (uint8_t _destAddr, char * _path )
    
            In order to speedup the process of sending a message and therefore avoid the delaty of finding a route,
            Static routes can be defned.
            Note however that static route take precedent and therefore no dynamic routes will be tried.
            Dynamic route , wiil also be periodically refreshed.
            So use static route at your own descrition :-) 
            
    @param  uint8_t _destAddr
    
            Destination node address 
    @param  char * _path 
    
            Path to destination node address.
            Example:
                source node (this one) 'A'
                Destination node 'D'
                Assuming nodes 'B' and 'C' exist and can route messages.
                
                possible paths : 'ABD' 'ACD' 'ABCD'
                
    

    @return 
            STS_OK 
            ROUTING_QUEUE_FULL

    @note   
*/


STSCODE LoraWifiMesh::addStaticRoute (uint8_t _destAddr, char * _path ){

   byte slot = 0;
   for(slot = 0; slot<LORA_MESH_MAX_ROUTING_TABLE_SIZE; slot++) {
        if (( routingTable[slot].sts == LORA_MESH_QUEUE_FREE )){
              routingTable[slot].sts = LORA_MESH_QUEUE_USED;
              routingTable[slot].type = ROUTE_DYNAMIC;
              strncpy(routingTable[slot].path,_path,LORA_MESH_MAX_ROUTING_PATH_SIZE);
              routingTable[slot].destNode = _destAddr;
              routingTable[slot].timeStamp = millis();
              return STS_OK;
         }
   }
   if (slot >= LORA_MESH_MAX_ROUTING_TABLE_SIZE) return ROUTING_QUEUE_FULL;
}

STSCODE LoraWifiMesh::addRoute(uint8_t destNode,char *path){
      byte slot = 0;
      byte cnt1 = 0;
      byte cnt2 = 0;
      
      for(slot = 0; slot<LORA_MESH_MAX_ROUTING_TABLE_SIZE; slot++) {
            if ((( routingTable[slot].sts == LORA_MESH_QUEUE_USED) || (routingTable[slot].sts == STS_ROUTE_MISSING ) || (routingTable[slot].sts == STS_ROUTE_WAITING)) && (routingTable[slot].destNode == destNode)){
                for (byte c = 0; c < LORA_MESH_MAX_ROUTING_PATH_SIZE; c++) {
                  if (routingTable[slot].path[c] == 0x00) break;
                  cnt1++;
                }

                if (cnt1 == 0 ) cnt1 = LORA_MESH_MAX_ROUTING_PATH_SIZE;

                for (byte c = 0; c < LORA_MESH_MAX_ROUTING_PATH_SIZE; c++) {
                  if (path[c] == 0x00) break;
                  cnt2++;
                }
           
                if (cnt2 < cnt1) {
                    routingTable[slot].timeStamp = millis();
                    routingTable[slot].sts = LORA_MESH_QUEUE_USED;
                    routingTable[slot].type = ROUTE_STATIC;
                    strncpy(routingTable[slot].path,path,LORA_MESH_MAX_ROUTING_PATH_SIZE);
                }
                return STS_OK;
            } 
            if (( routingTable[slot].sts == LORA_MESH_QUEUE_FREE )){
              routingTable[slot].sts = LORA_MESH_QUEUE_USED;
              strncpy(routingTable[slot].path,path,LORA_MESH_MAX_ROUTING_PATH_SIZE);
              routingTable[slot].destNode = destNode;
              routingTable[slot].type = ROUTE_STATIC;
              routingTable[slot].timeStamp = millis();
              return STS_OK;
            }
      }      
      if (slot >= LORA_MESH_MAX_ROUTING_TABLE_SIZE) return ROUTING_QUEUE_FULL; 
}


/*!
    @brief  LoraWifiMesh::yield()
    
            Main function to process asynchronous work
            It should be called in the loop function
            It process all pending tasks
                1) check for messages delivered and not ACK received and retry them
                2) check for messages received and put the on the received queue to be exposed to the user
                3) do some cleanup
                    1) refresh routing tables
                    2) refresh network topology
                    3) refresh node status
            
  

    @return STS_OK status code.

    @note   
*/

STSCODE LoraWifiMesh::yield(){

    USER_PACKET up;
    NODE_REGISTRATION nr;
    char _p[LORA_MESH_MAX_ROUTING_PATH_SIZE];
  
    cleanQueues();

    // ---- retry Route Reuquest--
    for(byte slot = 0; slot<LORA_MESH_MAX_ROUTING_TABLE_SIZE; slot++) {
        if (( routingTable[slot].sts == STS_ROUTE_MISSING )){
               routingTable[slot].sts = STS_ROUTE_WAITING;
               getRREQ(routingTable[slot].destNode);
               break;
        }
    }         

    //---- keep Alive Node Registration


    
    if (KeepAlive) {
          if ((millis() - _lastKeepAlive > KeepAliveInterval))
          {
            _lastKeepAlive = millis();   
          
    
            memset(&up, 0x00, sizeof(USER_PACKET));
            memset(&nr, 0x00, sizeof(NODE_REGISTRATION));
      
            nr.userMsgType = LORA_MESH_MSG_REGISTRATION;
            nr.nodeId = LocalAddress;
      
            memcpy(nr.macAddress, LWMesh.Mac, 6);
            memcpy(&up._b, &nr , sizeof(NODE_REGISTRATION));
            
            bool found = LWMesh.findRoute(MasterNode, _p);
            if (found) {
              byte _id = LWMesh.sendMsg(MasterNode, up._b, _p);
              if (_id < 0) LWMesh.stringSts (_id);
            } else {
              LWMesh.getRREQ(MasterNode);
            }
        }
    }    

    if (( DebugLevel <=  4 )  && (DebugLevel >0)){
        byte slot = 0;
        for ( slot=0; slot < LORA_MESH_RREQ_QUEUE_SIZE; slot++) {
            if ((sentRREQ[slot].sts == LORA_MESH_QUEUE_USED ) && ( sentRREQ[slot]._deliveredStatus== STS_RR_TO_BE_SENT)) {
                  _send (sentRREQ[slot]._rr._bmsg, sizeof(RREP_DATAGRAM));
                  sentRREQ[slot]._deliveredStatus== STS_RR_SENT;
          }
        }
    }    
    return STS_OK;
}

/*!
    @brief  LoraWifiMesh::initAddress(uint8_t locAdd)
    
            Initial setup function. Initiates node with localAddress
            Init and clean queues;
            
    @param  uint8_t locAdd
    
            Node local Address
    

    @return STS_OK status code.

    @note   
*/


STSCODE  LoraWifiMesh::initAddress(uint8_t locAdd){
    LocalAddress = locAdd;

    for(byte slot = 0; slot<LORA_MESH_MAX_ROUTING_TABLE_SIZE; slot++) {
         routingTable[slot].sts = LORA_MESH_QUEUE_FREE;
    }    
    for(byte slot = 0; slot<LORA_MESH_MSG_QUEUE_SIZE; slot++) {
         sentQueue[slot].sts = LORA_MESH_QUEUE_FREE;
    }    

    for(byte slot = 0; slot<LORA_MESH_RREQ_QUEUE_SIZE; slot++) {
         sentRREQ[slot].sts = LORA_MESH_QUEUE_FREE;
    }    

    for(byte slot = 0; slot<LORA_MESH_RECEIVED_QUEUE_SIZE; slot++) {
         receivedQueue[slot].sts = LORA_MESH_QUEUE_FREE;
    }    

    return STS_OK;
 }

 STSCODE  LoraWifiMesh::addRREQToQueue(uint8_t uniqueId){   
   byte slot = 0;
   for ( slot=0; slot < LORA_MESH_RREQ_QUEUE_SIZE; slot++) {
       if (sentRREQ[slot].sts == LORA_MESH_QUEUE_FREE) break;
   }
   
   if (slot >= LORA_MESH_RREQ_QUEUE_SIZE){
       if ((DebugLevel <=  1) && (DebugLevel >0)) {
           Serial.print(F("RREQ queue is full"));
       }
       return RREQ_QUEUE_FULL;
   }
           
   sentRREQ[slot].sts = LORA_MESH_QUEUE_USED;
   sentRREQ[slot].timeStamp = millis();
   sentRREQ[slot].uniqueId = uniqueId;

   return STS_OK;
 }

 STSCODE LoraWifiMesh::removeMSGfromQueue(uint8_t uniqueId, char *_msg){
    byte slot;
    for (slot=0; slot < LORA_MESH_MSG_QUEUE_SIZE; slot++) {
       if ((sentQueue[slot].sts == LORA_MESH_QUEUE_USED) && (sentQueue[slot]._pkt._msg._send.uniqueId == uniqueId)){
             for(byte slot0 = 0; slot0<LORA_MESH_RECEIVED_QUEUE_SIZE; slot0++) {
                 if (receivedQueue[slot0].sts == LORA_MESH_QUEUE_FREE) {
                    receivedQueue[slot0].sts = LORA_MESH_QUEUE_USED;
                    receivedQueue[slot0]._pkt._pkt.msgId = sentQueue[slot]._pkt._msg._send.uniqueId;
                    receivedQueue[slot0]._pkt._pkt.sts = _msg[0];
                    memcpy(receivedQueue[slot0]._pkt._pkt.msg,_msg,LORA_MESH_MAX_MSG_SIZE);
                    break;
                 }
             }
            sentQueue[slot].sts = LORA_MESH_QUEUE_FREE;
            break;
       }
   }  
   return STS_OK;
 }


 STSCODE  LoraWifiMesh::addRRToQueue(RR_Packet _rr, uint8_t _uniqueId){   
   byte slot = 0;
  
   for (slot=0; slot < LORA_MESH_RREQ_QUEUE_SIZE; slot++) {
       if (sentRREQ[slot].sts == LORA_MESH_QUEUE_FREE) break;
   }

   if (slot >= LORA_MESH_RREQ_QUEUE_SIZE){
       if ((DebugLevel <=  1) && (DebugLevel >0)){
           Serial.print(F("RRTABLE queue is full"));
       }
       return RREQ_QUEUE_FULL;
   }
  
   memcpy(sentRREQ[slot]._rr._bmsg,_rr._bmsg,sizeof(RR_Packet)); 
   sentRREQ[slot].uniqueId = _uniqueId;
   sentRREQ[slot].sts = LORA_MESH_QUEUE_USED;
   sentRREQ[slot].retryCount = 1;
   sentRREQ[slot]._deliveredStatus = STS_RR_TO_BE_SENT;
   sentRREQ[slot].timeStamp = millis();

   return STS_OK;
 }


 
STSCODE  LoraWifiMesh::addMSGToQueue(SEND_Packet _msg){   
   byte slot = 0;
   for (slot=0; slot < LORA_MESH_MSG_QUEUE_SIZE; slot++) {
       if (sentQueue[slot].sts == LORA_MESH_QUEUE_FREE) break;
   }

   if (slot >= LORA_MESH_MSG_QUEUE_SIZE){
       if ((DebugLevel <=  1) && (DebugLevel >0)) {
           Serial.print(F("SENT queue is full"));
       }
       return MSG_QUEUE_FULL;
   }
  
   memcpy(sentQueue[slot]._pkt._bmsg,_msg._bmsg,sizeof(SEND_Packet)); 
   sentQueue[slot].sts = LORA_MESH_QUEUE_USED;
   sentQueue[slot].retryCount = 1;
   sentQueue[slot].timeStamp = millis();

   return STS_OK;
 }


bool LoraWifiMesh::findRREQ(byte uniqueId){
    for(byte slot = 0; slot<LORA_MESH_RREQ_QUEUE_SIZE; slot++) {
       if (( sentRREQ[slot].sts == LORA_MESH_QUEUE_USED ) && (sentRREQ[slot].uniqueId == uniqueId  )){
        return true;
       }
    }
    return false;
 }
 
 STSCODE  LoraWifiMesh::cleanQueues(byte queueType){
    byte slot;
    long _now;
    STSCODE _retSts = STS_OK;
    _now = millis();
    
    for(slot = 0; slot<LORA_MESH_RREQ_QUEUE_SIZE; slot++) {
            if (( sentRREQ[slot].sts == LORA_MESH_QUEUE_USED ) && (_now - sentRREQ[slot].timeStamp >  LORA_MESH_RREQ_QUEUE_TIMEOUT )){
              sentRREQ[slot].sts = LORA_MESH_QUEUE_FREE;
            }
    }

    for(slot = 0; slot<LORA_MESH_MSG_QUEUE_SIZE; slot++) {
            if (( sentQueue[slot].sts == LORA_MESH_QUEUE_USED ) && (_now - sentQueue[slot].timeStamp > LORA_MESH_MSG_QUEUE_TIMEOUT)){
                 if (sentQueue[slot].retryCount < LORA_MESH_SEND_MSG_RETRY_COUNT ) {
                      sentQueue[slot].retryCount++;
                      sentQueue[slot].timeStamp = _now;
                      totalRetry++;
                      
                    
                    if ((DebugLevel <=  1 ) && (DebugLevel >0)) { 
                          Serial.print (F("RETRY MSG Id : "));
                          Serial.print(sentQueue[slot]._pkt._msg._send.uniqueId);
                          Serial.print(F(" \""));
                          Serial.print(sentQueue[slot]._pkt._msg._send.msg);
                          Serial.print(F("\" using PATH: \""));
                          Serial.print( sentQueue[slot]._pkt._msg._send.path );
                          Serial.print(F("\" retryCount: "));
                          Serial.println(sentQueue[slot].retryCount);
                     }

                      sendMsg(sentQueue[slot]._pkt._msg._send.destinationNode,sentQueue[slot]._pkt._msg._send.msg,sentQueue[slot]._pkt._msg._send.path, sentQueue[slot]._pkt._msg._send.uniqueId);               
                 }
                 else {
                     for(byte slot0 = 0; slot0<LORA_MESH_RECEIVED_QUEUE_SIZE; slot0++) {
                         if (receivedQueue[slot0].sts == LORA_MESH_QUEUE_FREE) {
                            receivedQueue[slot0].sts = LORA_MESH_QUEUE_USED;
                            receivedQueue[slot0]._pkt._pkt.msgId = sentQueue[slot]._pkt._msg._send.uniqueId;
                            receivedQueue[slot0]._pkt._pkt.sts = STS_TIMEOUT;
                            memcpy(receivedQueue[slot0]._pkt._pkt.msg,sentQueue[slot]._pkt._msg._send.msg,LORA_MESH_MAX_MSG_SIZE);
                            break;
                         }
                     }
                     sentQueue[slot].sts = LORA_MESH_QUEUE_FREE;
                  }
            }   
       }

// -- reset routing queues, start fresh----
   
       if ((millis() - _lastReset > ResetInterval))
          {
            _lastReset = millis();   

            for(byte slot = 0; slot<LORA_MESH_MAX_ROUTING_TABLE_SIZE; slot++) {
                routingTable[slot].sts = LORA_MESH_QUEUE_FREE;
                routingTable[slot].destNode = 0;
              }      

            for(byte slot = 0; slot<LORA_MESH_MAX_ROUTING_TABLE_SIZE; slot++) {
                routingTable[slot].sts = LORA_MESH_QUEUE_FREE;
                routingTable[slot].destNode = 0;
              }      

          
          }
          
       return _retSts;
 }


void LoraWifiMesh::dumpSendTo(SEND_Packet pkt){

   Serial.print(F("Dump SENDTO msg"));
   Serial.print(F(" Source:"));
   Serial.print((char)pkt._msg._hdr.sourceNode);
   Serial.print(F(" Dest:"));
   Serial.print((char)pkt._msg._hdr.destinationNode);
   Serial.print(F(" Type:"));
   Serial.print(pkt._msg._hdr.hdrType );
   Serial.print(F(" msgId:"));
   Serial.print( pkt._msg._send.uniqueId );
   Serial.print(F(" Source:"));
   Serial.print((char) pkt._msg._send.sourceNode );
   Serial.print(F(" dest:"));
   Serial.print((char) pkt._msg._send.destinationNode);
   Serial.print(F("["));
   Serial.print( pkt._msg._send.msg);
   Serial.println(F("]"));
}

/*!
    @brief  byte LoraWifiMesh::sendMsg(uint8_t destination, char *msg, char *_path, byte _uni, byte _ret)
    
            Mais user function to send messages
            
            Before sending  a message to the destination node, it check if there's a route...if not it requests for it.
            and out the message on hold in a queue.
            After the route is received thne the message is sent.
            If an ACK isn't received the system will retry it..a certain number of times until the ACK or TIMEOUT occurs
            this is an automatic process, the user only see two possible message ACK or TIMEOUT

            
            
    @param  uint8_t destination

    @param  char *msg
    
    @param  char *_path
    
    @param  byte _uni
    
    @param  byte byte _ret
    

    @return msgId;
    
            The unique messageId, which will allows for later confirm that it was received or timeout.

    @note   
*/

    
byte LoraWifiMesh::sendMsg(uint8_t destination, char *msg, char *_path, byte _uni, byte _ret){

    SEND_Packet pkt;
    char path[LORA_MESH_MAX_ROUTING_PATH_SIZE] ;
    uint8_t node1=0;
    uint8_t node2=0;

    if ( destination == LocalAddress ) return ERR_CANNOT_SEND_TO_SELF;
    memset(&pkt, 0, sizeof(SEND_Packet));
    memset(&path, 0,LORA_MESH_MAX_ROUTING_PATH_SIZE);

    if (_path[0] == 0x00) {
      bool found = findRoute(destination,path);
      if (!found){
          bool requestExist = false;
          for(byte slot = 0; slot<LORA_MESH_MAX_ROUTING_TABLE_SIZE; slot++) {
              if ((( routingTable[slot].sts == STS_ROUTE_MISSING) || ( routingTable[slot].sts == STS_ROUTE_WAITING))){
                if (routingTable[slot].sts == STS_ROUTE_WAITING) routingTable[slot].sts = STS_ROUTE_MISSING;
                requestExist = true;
               break;
              }
          }   
          if (!requestExist) {
              for(byte slot = 0; slot<LORA_MESH_MAX_ROUTING_TABLE_SIZE; slot++) {
                  if (( routingTable[slot].sts == LORA_MESH_QUEUE_FREE)){
                        routingTable[slot].sts = STS_ROUTE_MISSING;
                        routingTable[slot].destNode = destination;
                        routingTable[slot].timeStamp = millis();
                   break;
                  }
              }      
          }        
      }
      else { 

        }
    } else {
       strncpy(path,_path,LORA_MESH_MAX_ROUTING_PATH_SIZE);
    }

  
    for (int i = 0; i < LORA_MESH_MAX_ROUTING_PATH_SIZE-1; i++) {
         node1 = path[i];
         node2 = path[i+1];
         if (node1 == LocalAddress) break;
    }
  
    strncpy(pkt._msg._send.path,path,LORA_MESH_MAX_ROUTING_PATH_SIZE);
    strncpy(pkt._msg._send.msg,msg,LORA_MESH_MAX_MSG_SIZE);

    if((node1 == 0x00) && (node2 == 0x00) ) node2 = 0xFF;
    pkt._msg._hdr.sourceNode = LocalAddress;
    pkt._msg._hdr.destinationNode = node2;
    pkt._msg._hdr.hdrType = LORA_MESH_MSG_SENDTO; 

    if (_uni == 0xff) {
      pkt._msg._hdr.msgId = _uniqMsgId;
    }
    else {
      pkt._msg._hdr.msgId = _uni;
    }
    
    pkt._msg._send.sourceNode = LocalAddress;
    pkt._msg._send.destinationNode = destination;
    pkt._msg._send.type = LORA_MESH_MSG_SENDTO;
    
    if (_uni == 0xff) {
      pkt._msg._send.uniqueId = _uniqMsgId++;
    }
    else {
       pkt._msg._send.uniqueId = _uni;
    }

    if (_uni == 0xff) addMSGToQueue (pkt);    
    pkt._msg._hdr.len = sizeof(SEND_Packet);
    pkt._msg._hdr._crc = getCRC(pkt._bmsg,sizeof(SEND_Packet)); 

    if (( DebugLevel <=  1) && (DebugLevel >0)) LWMesh.dumpHDR(pkt._msg._hdr);

    _send (pkt._bmsg, sizeof(SEND_Packet));
    if (( DebugLevel <=  2) && (DebugLevel >0)) LWMesh.dumpMSGTable();

    
    return pkt._msg._hdr.msgId;
}


byte LoraWifiMesh::getRREQ(uint8_t destinationAddress){

    RREQ_Packet pkt;
    char path[LORA_MESH_MAX_ROUTING_PATH_SIZE];

    if ( destinationAddress == LocalAddress ) return ERR_CANNOT_ROUTE_TO_SELF;
  
    memset(&pkt, 0, sizeof(RREQ_Packet));
    memset(&path, 0, LORA_MESH_MAX_ROUTING_PATH_SIZE);
    
    pkt._msg._hdr.sourceNode = LocalAddress;
    pkt._msg._hdr.destinationNode = LORA_MESH_BROADCAST_ADDRESS;
    pkt._msg._hdr.hdrType = LORA_MESH_MSG_RREQ; 
    pkt._msg._hdr.msgId = _uniqMsgId++;
    
    pkt._msg._rreq.sourceNode = LocalAddress;
    pkt._msg._rreq.destinationNode = destinationAddress;
    pkt._msg._rreq.uniqueId  = _uniqRReqId++;
    pkt._msg._rreq.type  = LORA_MESH_MSG_RREQ;    
    sprintf(pkt._msg._rreq.path,"%c\0",LocalAddress);

    pkt._msg._hdr.len = sizeof(RREQ_Packet);
    pkt._msg._hdr._crc = getCRC(pkt._bmsg,sizeof(RREQ_Packet));

    _send (pkt._bmsg, sizeof(SEND_Packet));
  
    if ((DebugLevel <=  2) && (DebugLevel >0)) dumpRREQ(pkt._msg._rreq);
    
    return pkt._msg._hdr.msgId;
    
};

void LoraWifiMesh::dumpRTable(){
    Serial.println (F("---- ROUTING TABLE -----"));
    Serial.println(F("Node  Path "));
    for (int i = 0;i<LORA_MESH_MAX_ROUTING_TABLE_SIZE;i++){
      if((LWMesh.routingTable[i].sts == LORA_MESH_QUEUE_USED) ||(LWMesh.routingTable[i].sts == STS_ROUTE_WAITING) ||(LWMesh.routingTable[i].sts == STS_ROUTE_MISSING)){
        Serial.print (F(" "));
        Serial.print(LWMesh.routingTable[i].destNode);   
        Serial.print (" => ");
        Serial.println(LWMesh.routingTable[i].path);           
    }
    }
};

void LoraWifiMesh::dumpRREQTable(){
    Serial.println (F("---- RREQ QUEUE ----"));
    Serial.println(F("UniqueId  DeliveredSts timeStamp"));
    for (int i = 0;i<LORA_MESH_RREQ_QUEUE_SIZE;i++){
      if ((LWMesh.sentRREQ[i].sts == LORA_MESH_QUEUE_USED) ) {
        Serial.print (F("   "));
        Serial.print(LWMesh.sentRREQ[i].uniqueId);   
        Serial.print(F("   "));           
        Serial.print(LWMesh.sentRREQ[i]._deliveredStatus);   
        Serial.print(F("   "));           
        Serial.println(LWMesh.sentRREQ[i].timeStamp);           
        }
    }
};


void LoraWifiMesh::dumpMSGTable(){
    Serial.println (F("---- SENT Queue ----"));
    Serial.println(F("UniqueId  Retry TimeStamp  sourceNode destNode"));
    for (int i = 0;i<LORA_MESH_MSG_QUEUE_SIZE;i++){
      if (LWMesh.sentQueue[i].sts == LORA_MESH_QUEUE_USED) {
        Serial.print (F(" "));
        Serial.print(LWMesh.sentQueue[i]._pkt._msg._send.uniqueId);   
        Serial.print(F("    "));           
        Serial.print(LWMesh.sentQueue[i].retryCount);           
        Serial.print(F("    "));           
        Serial.print(LWMesh.sentQueue[i].timeStamp);           
        Serial.print(F("    "));           
        Serial.print(LWMesh.sentQueue[i]._pkt._msg._send.sourceNode);           
        Serial.print(F("    "));           
        Serial.println(LWMesh.sentQueue[i]._pkt._msg._send.destinationNode);           
      
        }
    }
};

void LoraWifiMesh::dumpRREQ(RREQ_MSG msg){
   Serial.print (F("Dump  RREQ sNode:"));
   Serial.print((char)msg.sourceNode);   
   Serial.print (F(" dNode:"));
   Serial.print((char)msg.destinationNode); 
   Serial.print (F(" uniquw:"));
   Serial.print(msg.uniqueId);   
   Serial.print (F(" type:"));
   Serial.print(msg.type);   
   Serial.print(F(" path:["));
   Serial.print( msg.path);
   Serial.println(F("]"));        
};

void LoraWifiMesh::dumpRREP(RREP_MSG msg){
   Serial.print (F(" Dump RREP sNode:"));
   Serial.print((char)msg.sourceNode);   
   Serial.print (F(" dNode:"));
   Serial.print((char)msg.destinationNode); 
   Serial.print (F(" uniquw:"));
   Serial.print(msg.uniqueId);   
   Serial.print (F(" type:"));
   Serial.print(msg.type);   
   Serial.print(F(" path:["));
   Serial.print( msg.path);
   Serial.println(F("]"));                    
};

void LoraWifiMesh::dumpHDR(HDR_MSG msg){
   Serial.print (F("Dump HDR  sNode:"));
   Serial.print((char)msg.sourceNode);   
   Serial.print (F(" dNode:"));
   Serial.print((char)msg.destinationNode); 
   Serial.print (F(" Type:"));
   Serial.println(msg.hdrType);   
           
};


void LoraWifiMesh::dumpNetwork(){
  
    Serial.println("--- NETWORK MAP ----");
    for(byte slot = 0; slot<LORA_MESH_MAX_NETWORK_SIZE; slot++) {
        if (meshNetwork[slot].sts > 0) {
            Serial.print("Node Id:");
            Serial.print( meshNetwork[slot].nodeId);
            Serial.print(" Path:[");
            Serial.print(meshNetwork[slot].path);
            Serial.print( "] Status [");
            Serial.print(meshNetwork[slot].sts);
            Serial.print("] RSSI[");
            Serial.print(meshNetwork[slot].RSSI);
            Serial.print("] keepAlive:[");
            Serial.print(meshNetwork[slot].lastKeepAlive);
            Serial.print("] mac:[");
              for (int i = 0; i < 6 ; i++) {
                   Serial.print(meshNetwork[slot].macAddress[i],HEX);
                   if (i < 5 ) Serial.print(":");
              }
            Serial.println("]");
        }
    }
}

bool LoraWifiMesh::checkCRC (char *buff, byte len, char *msg){
    byte _crc1, _crc0;

    Global_Packet pkt;
    memset(&pkt, 0, sizeof(Global_Packet));
    memcpy(&pkt, buff, len);

    _crc0 = pkt._send._hdr._crc;
     pkt._send._hdr._crc = 0xAA;
    _crc1 = CRC(pkt._bmsg,len);
    
    if (_crc1 != _crc0) {

          for(byte slot0 = 0; slot0<LORA_MESH_RECEIVED_QUEUE_SIZE; slot0++) {
             if (receivedQueue[slot0].sts == LORA_MESH_QUEUE_FREE) {
                receivedQueue[slot0].sts = LORA_MESH_QUEUE_USED;
                receivedQueue[slot0]._pkt._pkt.msgId = pkt._send._hdr.msgId;
                receivedQueue[slot0]._pkt._pkt.sts = ERR_RREQ_CRC_ERR;
                break;
             }
          }

          if ((DebugLevel <= 1) && (DebugLevel >0)){
                Serial.print(F("Received CRC Error from:" ));
                Serial.println(msg);
                Serial.println(pkt._send._hdr._crc,HEX);
                for (int j=0; j < sizeof(RREQ_Packet); j++) {
                    Serial.print(buff[j],HEX);
                    Serial.print(F(" "));
                 }
                Serial.println();
                if ((DebugLevel <= 2) && (DebugLevel >0)){
                     Serial.print(F("CRC:")); Serial.print(_crc1,HEX);Serial.print(F(" CRC2:"));Serial.println(_crc0,HEX); 
                }
          }
    }
            
}

byte LoraWifiMesh::getCRC (char *buff, byte len){
  
    Global_Packet pkt;
    memset(&pkt, 0, sizeof(Global_Packet));
    memcpy(&pkt, buff, len);
     
    pkt._send._hdr._crc = 0xAA;
    byte _crc = CRC(pkt._bmsg,len);    
   
    return _crc;
}

LoraWifiMesh LWMesh;
