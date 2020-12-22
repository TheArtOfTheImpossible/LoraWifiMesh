// Copyright © 2020 by antónio montez . All rights reserved.
// Licensed under the MIT license.
//
//   Generic code to implement a LoraWifiMesh node. It support 255 Node Address. For readable porposes I'm using : Ascii printable node names :"123456780@ABCDEFEDGH....."
//   There are 3 important parameters :
// 
//        byte localAddress = 0x31;             // defines this node address... however I'm using the ArduinoUniqueID library to automatic generate the localAddress
//        byte Protocol = MESH_PROTOCOL_LORA;   // the two protocols support at the moment  MESH_PROTOCOL_LORA && MESH_PROTOCOL_WIFI
//        #define MASTER_NODE 0x39              // define which node will be used as Master..take in consideration that every node can be "slave" and master at same time... 
//                                              // this is just a parameter that says which will be the MASTER FOR THIS NODE.
//
//       and a couple of mandatory function calls:
//
//                LWMesh.setProtocol(Protocol);                         // self explanatory
//                LWMesh.setConfig(nc);                                 // configure a Structure with all config parameters
//                LWMesh.addNodeToNetwork(localAddress,_mac,Protocol);  // Registers the node to the define MASTER node. This is not mandatory. this is just to tell the MASTER that this node exists.
//
//      You can then check for messages received with:
//              while (LWMesh.hasMsg(&rec)) {}
//
//      and send messages with :
//          byte _id = LWMesh.sendMsg(nodeAddress,chars,path);   // when you known the path : example "942" .... this node (9) wants to send a message to node (2) passing by node (4)
//          byte _id = LWMesh.sendMsg(nodeAddress,chars);        // when you don't know the path... the routing protocol will then try to find a route and if found send the message.
//              
//      all sent and received messages are (non blocking)...and all sent messages will have a ACK ...and retry.
//      so after calling LWMesh.send@sectionMsg... there's no confirmation yet of the deliver.
//      you need to go and check in:
//            LWMesh.hasMsg(&rec)) 
//
//      function will return not only the NORMAL messages receives.... but also the CONFIRMATION (ACK | TIMEOUT) of the sent messages... the message ID (_id)... is then your key for matching pair send/received.
//      The confirmation is Automatic... you don't need to take care this in your code.
//
//      This code has been tested and runs on ESP8266, ESP32 running protocol WIFI
//      This code has been tested and runs on HELTEC Board (ESP32) LORA and Arduino pro-mini, connecting to a  LORA-02 generic board... however the memory available on the Arduino is on the limits...
//      .. therefore, I recommend using a ESP8266 instead.
// 

/* Disclaimer
 * This SOFTWARE PRODUCT is provided by THE PROVIDER "as is" and "with all faults." THE PROVIDER makes no representations or warranties of any kind concerning the safety,
 * suitability, lack of viruses, inaccuracies, typographical errors, or other harmful components of this SOFTWARE PRODUCT. There are inherent dangers in the use of any software,
 * and you are solely responsible for determining whether this SOFTWARE PRODUCT is compatible with your equipment and other software installed on your equipment. You are also
 * solely responsible for the protection of your equipment and backup of your data, and THE PROVIDER will not be liable for any damages you may suffer in connection with using,
 * modifying, or distributing this SOFTWARE PRODUCT
 * 
 */

 
 
#include "Arduino.h"
#include <LoraWifiMesh.h>
#include "ArduinoUniqueID.h"
#define MASTER_NODE 0x39 // 
#define Band    433E6  // LORA Band 433Mhz
#define macFormat "%c%c%c%c%c%c"

void setup(){

    byte localAddress = 0x00; 
    byte Protocol = MESH_PROTOCOL_LORA;
    char _mac[6];
    
    LWMesh.setProtocol(Protocol);
      
    Serial.begin(115200);
  
    #if defined(ESP8266)              //----- PUT here all your ESP8266 based devices
          UniqueIDdump(Serial);
          switch ( UniqueID[3]) {
                case 0x49:  localAddress = 0x31; sprintf(_mac, macFormat, 0x5c, 0xcf, 0x7f, 0x68, 0x0a, 0x49); break;
                case 0xA6 : localAddress = 0x32; sprintf(_mac, macFormat, 0x48, 0x3f, 0xda, 0x5a, 0x8d, 0xa6); break;
                case 0xD8 : localAddress = 0x33; sprintf(_mac, macFormat, 0x48, 0x3f, 0xda, 0x59, 0x82, 0xD8); break;
                case 0x6B:  localAddress = 0x34; sprintf(_mac, macFormat, 0x48, 0x3f, 0xda, 0x02, 0x10, 0x6B); break;
                case 0xF9 : localAddress = 0x35; sprintf(_mac, macFormat, 0x5c, 0xcf, 0x7f, 0x8f, 0x1d, 0xF9); break;
                case 0x8D : localAddress = 0x36; sprintf(_mac, macFormat, 0xFF, 0xFF, 0xFF, 0x8f, 0x1e, 0x8D); break;
                case 0x8B : localAddress = 0x37; sprintf(_mac, macFormat, 0xFF, 0xFF, 0xFF, 0x68, 0x14, 0x8b); break;
                case 0xDF : localAddress = 0x38; sprintf(_mac, macFormat, 0x18, 0xfe, 0x34, 0xe6, 0x1c, 0xDF); break;
                case 0x20 : localAddress = 0x41; sprintf(_mac, macFormat, 0x18, 0xfe, 0x34, 0xF5, 0x9B, 0x20); break;
                case 0xd4 : localAddress = 0x42; sprintf(_mac, macFormat, 0x5c, 0xcf, 0x7f, 0x01, 0x36, 0xd4); break;
                case 0xea : localAddress = 0x43; sprintf(_mac, macFormat, 0x5c, 0xcf, 0x7f, 0x01, 0x34, 0xea); break;
                case 0x5a : localAddress = 0x44; sprintf(_mac, macFormat, 0x18, 0xfe, 0x34, 0xf5, 0x99, 0x5a); break;
          }
    #endif 
    
    #if defined(_HELTEC_)               //----- PUT here all your ESP32 HELTEC Lora based devices
          UniqueIDdump(Serial);
          switch ( UniqueID[5]) {                
                case 0x38 : localAddress = 0x39; sprintf(_mac, macFormat, 0xF0, 0x08, 0xD1, 0xDC, 0x7B, 0x38); break;
                case 0xBC : localAddress = 0x40; sprintf(_mac, macFormat, 0xf0, 0x08, 0xD1, 0xDC, 0x7B, 0xBC); break;
          }
    #endif

    #if defined(ARDUINO_ARCH_AVR)
          UniqueIDdump(Serial);
          Serial.print(F("UniqueID: "));
          for (size_t i = 0; i < UniqueIDsize; i++)
          {
            if (UniqueID[i] < 0x10)
              Serial.print(F("0"));
              Serial.print(UniqueID[i], HEX);
              Serial.print(F(" "));
          }
          Serial.println();      

          Serial.println(UniqueID[8], HEX);   //----- PUT here all your "normal" AVR 386 Arduinos based devices
          switch ( UniqueID[8]) {
                case 0x1a : localAddress = 0x49; sprintf(_mac, macFormat, 0x31, 0x34, 0x34, 0x0f, 0x25, 0x1a); break;
                case 0x1b : localAddress = 0x50; sprintf(_mac, macFormat, 0x31, 0x34, 0x34, 0x01, 0x28, 0x1b); break;
                case 0x07 : localAddress = 0x51; sprintf(_mac, macFormat, 0x31, 0x36, 0x32, 0x04, 0x0e, 0x07); break;
          }
    #endif


  if (LWMesh.Protocol == MESH_PROTOCOL_WIFI ) {  
       #if !defined (_HELTEC_) && defined (ESP8266)          //-------- ESP_NOW for ESP8266 based boards
              WiFi.mode(WIFI_STA);
              WiFi.begin();
              Serial.print(F("MACAddress: "));
              Serial.println(WiFi.macAddress());
              esp_now_init();
              esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
              esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len)
              {                
                memcpy(LWMesh.dataReceived, data, len);   
                LWMesh.processMsg(len, LWMesh.dataReceived);
              });
              
        #elif defined (_HELTEC_)                  //-------- ESP_NOW for ESP32 based boards
        
              Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, Band /*long BAND*/);
              WiFi.mode(WIFI_STA);
              WiFi.begin(); 
              UniqueIDdump(Serial);
  
              esp_now_init();
        
              esp_now_peer_info_t peerInfo;
              memcpy(peerInfo.peer_addr, LWMesh.broadcastAddress, 6);
              peerInfo.channel = 0;
              peerInfo.encrypt = false;
        
              if (esp_now_add_peer(&peerInfo) != ESP_OK) {
                Serial.println(F("Failed to add peer"));
                return;
              }
              esp_now_register_send_cb(OnDataSent);
              esp_now_register_recv_cb(OnDataRecv);
        #endif
             
  } else {  //---------------- PROTOCOL LoRa -----------------------------
    
        #if defined (_HELTEC_)         //----- for Lora Heltec boards
         
            Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, Band /*long BAND*/);         
            Heltec.display->clear();
            Heltec.display->setFont(ArialMT_Plain_10);
            Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
 
        #else                         //----- not for Lora Heltec boards
            if (!LoRa.begin(Band)) {   
                Serial.println(F("LoRa init failed. Check your connections."));
                while (true);      
            }
        #endif
   
        #if !defined(ESP32) || defined(_HELTEC_)
              LoRa.setSpreadingFactor(7);
              LoRa.setSignalBandwidth(125E3);
              LoRa.setCodingRate4(5);
              LoRa.setPreambleLength(8);
              LoRa.disableCrc();
              LoRa.setSyncWord(0x12);
              
              onReceive(LoRa.parsePacket());
        #endif
    }
  

    NODE_CONFIGURATION nc;
  
    memset(&nc, 0x00, sizeof(NODE_CONFIGURATION));
    
    nc.nodeId              = localAddress;
    nc.masterNode          = MASTER_NODE;
    nc.nodeType            = LORA_MESH_NODE_TYPE_GENERIC;
    nc.protocol            = Protocol;
    nc.band                = Band;
    nc.maxMsgRetry         = LORA_MESH_SEND_MSG_RETRY_COUNT;
    nc.retryInterval       = LORA_MESH_MSG_QUEUE_TIMEOUT;
    nc.keepAlive           = true;
    nc.keepAliveInterval   = LORA_MESH_KEEP_ALIVE_INTERVAL;
    nc.debugLevel          = 0;
    memcpy(nc.macAddress,_mac,6);
    //        memcpy(nc.pathToMaster,"xyz",3);
    sprintf((char*)nc.blockNodes,macFormat,0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00);
    sprintf((char*)nc.blockBroadcast,macFormat,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00); 
    
    LWMesh.setConfig(nc);
    LWMesh.addNodeToNetwork(localAddress,_mac,Protocol);
    
   Serial.print(sizeof(NODE_CONFIGURATION));  Serial.print(F("LOCAL ADDRESS:"));Serial.println((char) LWMesh.LocalAddress);

}


void loop()
{

  USER_PACKET up;
  NODE_REGISTRATION nr;
  RECEIVED_Packet rec;

  while (true) {

    char ch;
    byte cnt = 0;
    
    LWMesh.yield();
    
    memset(rec._bmsg, 0, sizeof(RECEIVED_Packet));

    while (LWMesh.hasMsg(&rec)) {
      Serial.print(F("Recv ID: "));
      Serial.print(rec._pkt.msgId);
      Serial.print (F(" ["));
      memcpy(up._b, rec._pkt.msg , sizeof(NODE_REGISTRATION));
      Serial.print(F(" Type:"));
      Serial.print( up._reg.userMsgType);
      for (int i = 0; i < 6 ; i++) {
        Serial.print(up._reg.macAddress[i], HEX);
        if (i < 5 ) Serial.print(F(":"));
      }

      Serial.print (F("]"));
      Serial.print(F(" status: "));
      LWMesh.stringSts(rec._pkt.sts);
      Serial.print(F("     "));
      Serial.println();
      byte _sts = rec._pkt.sts;
      memset(rec._bmsg, 0, sizeof(RECEIVED_Packet));
      LWMesh.dumpNetwork();
      LWMesh.dumpRTable();    
    }
     onReceive(LoRa.parsePacket());
  }
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;
}


#if (defined(ESP32) || defined(_HELTEC_)) && !defined (ARDUINO_ARCH_AVR)
      void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
      
        if (status == 0) {
        }
        else {
          Serial.println(F("Delivery Fail :("));
        }
      }
      
      void OnDataRecv(const uint8_t * mac, const uint8_t *data, int len)
      {
        memcpy(LWMesh.dataReceived, data, len);
        LWMesh.processMsg(len, LWMesh.dataReceived);
      };
#endif
