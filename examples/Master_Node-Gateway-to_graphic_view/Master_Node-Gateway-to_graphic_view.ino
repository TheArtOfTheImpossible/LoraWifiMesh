// Copyright © 2020 by antónio montez . All rights reserved.
// Licensed under the MIT license.

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
#define Band    433E6  // LORA Band 433Mhz
#define macFormat "%c%c%c%c%c%c"


#if defined(ESP8266)
      #include <SoftwareSerial.h>
      SoftwareSerial mySerial(0, 1); // RX, TX
      #define RXpin 13
      #define TXpin 15
      SoftwareSerial espSerial(RXpin ,TXpin);
      #define EXPIRED 128
#endif

#define MASTER_NODE 0x39 // 


void setup(){

    byte localAddress = MASTER_NODE; 
    char _mac[6];
    
    byte Protocol = MESH_PROTOCOL_LORA;
    
    LWMesh.setProtocol(Protocol);
      
    Serial.begin(115200);
  
    #if defined(ESP8266) 
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
    
    #if defined(_HELTEC_) 
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

          Serial.println(UniqueID[8], HEX);
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
    nc.band                = 433E6;
    nc.maxMsgRetry         = LORA_MESH_SEND_MSG_RETRY_COUNT;
    nc.retryInterval       = LORA_MESH_MSG_QUEUE_TIMEOUT;
    nc.keepAlive           = true;
    nc.keepAliveInterval   = LORA_MESH_KEEP_ALIVE_INTERVAL;
    nc.debugLevel          = 0;
    memcpy(nc.macAddress,_mac,6);
    //        memcpy(nc.pathToMaster,"981",3);
    sprintf((char*)nc.blockNodes,macFormat,0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00);
    sprintf((char*)nc.blockBroadcast,macFormat,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00); 
    
    LWMesh.setConfig(nc);
    LWMesh.addNodeToNetwork(localAddress,_mac,Protocol);
    
   Serial.print(sizeof(NODE_CONFIGURATION));  Serial.print(F("LOCAL ADDRESS:"));Serial.println((char) LWMesh.LocalAddress);

#if !defined(ESP32)  && !defined (ARDUINO_ARCH_AVR)
    if (MASTER_NODE == localAddress ) {
      espSerial.begin(19200);
    }
#endif

}

void sendNetInfo(char opt){

#if !defined(ESP32) && !defined (ARDUINO_ARCH_AVR)
       NET n;
       byte slotCount = 0;
       memset (&n,0x00,sizeof(NET));       
       for(byte slot = 0; slot<LORA_MESH_MAX_NETWORK_SIZE; slot++) {
        
      if ((LWMesh.meshNetwork[slot].sts == LORA_MESH_NODE_REGISTERED)
     || ( LWMesh.meshNetwork[slot].sts == LORA_MESH_NODE_ALIVE) )
     {
              n.meshNetwork[slotCount].nodeId = LWMesh.meshNetwork[slot].nodeId;
              if (millis() - LWMesh.meshNetwork[slot].lastKeepAlive > LWMesh.KeepAliveInterval) {
                 n.meshNetwork[slotCount].sts = LWMesh.meshNetwork[slot].sts  | EXPIRED; 
              } else {
                  n.meshNetwork[slotCount].sts = LWMesh.meshNetwork[slot].sts ;
              }
              for(byte i = 0; i<4; i++) {
                    n.meshNetwork[slotCount].path[i] = LWMesh.meshNetwork[slot].path[i];
              }           
              slotCount++;
          }
       }

 
       espSerial.write(0x07);
       espSerial.write(0x07);     
       espSerial.write(0x07);  
       
       for (int l = 0 ; l < sizeof(netInfo)*slotCount; l++) {
           espSerial.write(n._bmsg[l]);   
       }
       espSerial.write(0x09);
       espSerial.write(0x09);
       espSerial.write(0x09);

      Serial.println();
      Serial.print(F("Sent : "));
      Serial.print(sizeof(netInfo)*slotCount);
      Serial.println(F(" Bytes"));

       for(byte slot = 0; slot<LORA_MESH_MAX_NETWORK_SIZE; slot++) {
        
     if (((n.meshNetwork[slot].sts & LORA_MESH_NODE_REGISTERED) == LORA_MESH_NODE_REGISTERED)
     || (( n.meshNetwork[slot].sts & LORA_MESH_NODE_ALIVE) == LORA_MESH_NODE_ALIVE)  ||
        (( n.meshNetwork[slot].sts & EXPIRED) == EXPIRED))
     {     Serial.print(F("Node Id: ["));
          Serial.print((char)n.meshNetwork[slot].nodeId);
          Serial.print(F("] sts: "));
          Serial.print(n.meshNetwork[slot].sts);
          Serial.print(F(" Path:["));
          for (int l = 0 ; l < 4 ; l++)
            if (n.meshNetwork[slot].path[l]) Serial.print(n.meshNetwork[slot].path[l]);
          Serial.print ("]");

          if((n.meshNetwork[slotCount].sts & EXPIRED) == EXPIRED){
               Serial.println (F("EXPIRED"));
          } else Serial.println();
      }
  }
  #endif
  
}


void loop()
{

  USER_PACKET up;
  NODE_REGISTRATION nr;
  RECEIVED_Packet rec;

  while (true) {

    char ch;
    byte cnt = 0;

 #if !defined(ESP32) && !defined (ARDUINO_ARCH_AVR)
    if (MASTER_NODE == LWMesh.LocalAddress ) {          
          while (espSerial.available() > 0) {          
             ch = (char)espSerial.read();
             cnt++;
      }
          if (cnt >0) {
            sendNetInfo(ch);
          }
    }
#endif
    
    execAnySerialCmd();
    
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
      //   LWMesh.dumpMSGTable();
    }
     onReceive(LoRa.parsePacket());
  }
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;
//  Serial.print(packetSize);
//  Serial.print(" Bytes RECEIVED");
  //  Heltec.display->display();
}


#if (defined(ESP32) || defined(_HELTEC_)) && !defined (ARDUINO_ARCH_AVR) && !defined(ESP8266)

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


///------------------

void execAnySerialCmd() {
  byte _id;
  byte dest;
  byte cnt;
  char ch;
  char c1, c2;
  char _m[LORA_MESH_MAX_MSG_SIZE];
  char _p[LORA_MESH_MAX_ROUTING_PATH_SIZE];  
  char _s[LORA_MESH_MAX_MSG_SIZE + LORA_MESH_MAX_ROUTING_PATH_SIZE];
  char _s1[LORA_MESH_MAX_MSG_SIZE + LORA_MESH_MAX_ROUTING_PATH_SIZE];
  char _s2[LORA_MESH_MAX_MSG_SIZE + LORA_MESH_MAX_ROUTING_PATH_SIZE];

  if (Serial.available()) {
    while (Serial.available()) {
      ch = Serial.read();
      _s[cnt++] = ch;
    }

    if (ch == 0x0A) {
      memcpy(_s1, _s, cnt);
      memcpy(_s2, _s, cnt);
      sscanf ( _s, "%c%c", &c1, &c2);
      uint8_t d;

      if ((c1 == 0x52) || (c1 == 0x72)) {
        sscanf ( _s1, "%c%c%d", &c1, &c2, &d);
        Serial.print(F("Sending: ["));
        Serial.print(_m);
        Serial.print(F("] to : "));
        Serial.println(dest);
        _id = LWMesh.getRREQ(d);
        if (_id >= 0) {
          Serial.print (F("Sent ID: "));
          Serial.print(_id);
          Serial.print(F(" ["));
          Serial.print(_m);
          Serial.print(F("] using PATH: \""));
          Serial.print( _p );
          Serial.println(F("\"              "));
        }
        else LWMesh.stringSts (_id);
      }

      if ((c1 == 0x44) || (c1 == 0x64)) {
        sscanf (_s2, "%c%c%d", &c1, &c2, &d);
        LWMesh.setDebugLevel(d);
      }

      if ((c1 == 0x6D) || (c1 == 0x4D)) {
        sscanf ( _s, "%c%c%s%d%s", &c1, &c2, &_m[0], &d, &_p[0] );
        Serial.print(F("Sending: ["));
        Serial.print(_m);
        Serial.print(F("] to Node : "));
        Serial.print(d);
        Serial.print(F("] using route : "));
        Serial.println(_p);
        _id = LWMesh.sendMsg(d, _m, _p);
        if (_id >= 0) {
          Serial.print (F("Sent ID: "));
          Serial.print(_id);
          Serial.print(F(" ["));
          Serial.print(_m);
          Serial.print(F("] using PATH: \""));
          Serial.print( _p );
          Serial.println(F("\"        "));
        }
        else LWMesh.stringSts (_id);
      }
      cnt = 0;
      memset(_m, 0, LORA_MESH_MAX_MSG_SIZE);
      memset(_p, 0, LORA_MESH_MAX_ROUTING_PATH_SIZE);
      memset(_s, 0, LORA_MESH_MAX_ROUTING_PATH_SIZE + LORA_MESH_MAX_MSG_SIZE);
    }
  }
}



void printAt(byte x, byte y) {
  char ss[20];
  if (VT200) {
    sprintf (ss, "\u001b[%d;%dH", x, y);
    Serial.print(ss);
  }
}

void printCls() {
  char ss[20];
  if (VT200) {
    sprintf(ss, "\u001b[2J");
    Serial.print(ss);
  }
}

void clearLine() {
  char ss[20];
  if (VT200) {
    sprintf(ss, "\u001b[2K");

    Serial.print(ss);
  }
}
void setCor(byte cor) {
  char ss[20];
  if (VT200) {
    sprintf(ss, "\u001b[%dm", cor);
    Serial.print(ss);
  }

}
