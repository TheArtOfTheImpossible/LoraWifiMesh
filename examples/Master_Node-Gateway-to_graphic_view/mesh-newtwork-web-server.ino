/*
 * Disclaimer
 * This SOFTWARE PRODUCT is provided by THE PROVIDER "as is" and "with all faults." THE PROVIDER makes no representations or warranties of any kind concerning the safety,
 * suitability, lack of viruses, inaccuracies, typographical errors, or other harmful components of this SOFTWARE PRODUCT. There are inherent dangers in the use of any software,
 * and you are solely responsible for determining whether this SOFTWARE PRODUCT is compatible with your equipment and other software installed on your equipment. You are also
 * solely responsible for the protection of your equipment and backup of your data, and THE PROVIDER will not be liable for any damages you may suffer in connection with using,
 * modifying, or distributing this SOFTWARE PRODUCT
 * 
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>

#include "Arduino.h"
#include "LoraWifiMesh.h"
#include "ArduinoUniqueID.h"

#ifndef STASSID
#define STASSID ""
#define STAPSK  ""
#endif

#define EXPIRED 128

const char* ssid = STASSID;
const char* password = STAPSK;

#define           RcvCount 200
#define           RXpin      13
#define           TXpin      15 
String            UARTrcvData;                            // AUTO
char              UARTrcvBuffer;                          // AUTO
int               UARTrcvCount;                           // AUTO

String            WIFIsendData;                           // AUTO
String            WIFIrcvData;                            // AUTO
char              WIFIrcvBuffer;                          // AUTO
int               WIFIrcvCount;                           // AUTO


ESP8266WebServer server(80);

SoftwareSerial espSerial(RXpin, TXpin);

void handleNetwork() {
 
 char ch;
 int cnt = 0;
 NET n;
 memset (&n,0x00,sizeof(NET));
 
 bool endStream = false;
 bool startStream = false;

 bool startOfHeading = false;
 bool startOfText = false;
 bool endOfText = false;
 bool endOfTransmission= false;
 byte c = 0;


 Serial.println("Requesting Nodes");
 espSerial.write('N');


char startSeq = 0;
char endSeq = 0;
char lastChar = 0x00;
char lastLastChar = 0x00;

 while ((espSerial.available() > 0)) {
        ch = espSerial.read();
        Serial.print(ch,HEX);
        switch (ch) {
              case 0x07 : if ((lastChar == ch) && (lastLastChar == ch)) {
                              startOfHeading=true; 
                          }
                          break;
            
              case 0x09 : endOfTransmission=true; //*EOT
                          switch ( endSeq ) {
                            case 0 : 
                                    endSeq++;
                                    break;
                            case 1 : if ( lastChar == ch ) {
                                          endSeq++;
                                     } else {
                                        endSeq = 0;
                                        endOfTransmission=false;            
                                     }
                                     break;
                            case 2 : endOfTransmission=true;  
                                     endSeq++;
                                     break;
                            }
                          break;
               default : if ((lastChar == 0x07) && (lastLastChar == 0x07)) {
                                  startOfText=true; 
                              }
           }
      
      if ( startOfText ) n._bmsg[c++] = ch;
      if (startOfHeading) startOfText=true; 
      lastLastChar = lastChar;
      lastChar = ch;
 }

 if ( endOfTransmission == true ) {
   
    Serial.print("End Seq:");
    Serial.println(endSeq,DEC);
    n._bmsg[c-1] = 0x00;
    n._bmsg[c-2] = 0x00;
 }


 Serial.print("Bytes Count:");
 Serial.println(c);

   for (int l = 0 ; l < c ; l++) {
       Serial.print((char)n._bmsg[l],HEX);
   }

  Serial.println("--- NETWORK MAP ----");

    String s = "";
    String ss = "{";
    ss.reserve(2600);
  
  ss += "\"nodes\":[ { \"id\":\"D\", \"label\":\"MASTER\", \"x\":\"1\",\"y\":\"2\"}";
               
  for(byte slot = 0; slot<LORA_MESH_MAX_NETWORK_SIZE; slot++) {
    if (((n.meshNetwork[slot].sts & LORA_MESH_NODE_REGISTERED) == LORA_MESH_NODE_REGISTERED)
     || (( n.meshNetwork[slot].sts & LORA_MESH_NODE_ALIVE) == LORA_MESH_NODE_ALIVE) )
     { 
          Serial.print("Node Id: [");          
          Serial.print((char)n.meshNetwork[slot].nodeId);
          if (slot>=0) ss += ",";
          s = String(n.meshNetwork[slot].nodeId);
          ss += "{ \"id\":\"" + s  + "\",\"label\":\"" + s+ "\",\"x\":\"1\",\"y\":\"" + slot*50 +"\""; 
          
          Serial.print("] sts: ");
          Serial.print(n.meshNetwork[slot].sts);
          if((n.meshNetwork[slot].sts & EXPIRED) == EXPIRED){ 
            ss += ",  \"color\": { \"background\": \"#FF0000\" }}";
            Serial.print(" EXPIRED ");
          } else ss += ",  \"color\": { \"background\": \"#0080ff\" }}";

         
          Serial.print(" Path:[");
          for (int l = 0 ; l < 4 ; l++)
            if (n.meshNetwork[slot].path[l]) Serial.print(n.meshNetwork[slot].path[l]);
          Serial.println("]");
      }
  }
           
  ss += "],";
  ss += "\"edges\":[";


char _nod0;
char _nod1;
char cnt1=0;

  for(byte slot = 0; slot<LORA_MESH_MAX_NETWORK_SIZE; slot++) {
     if (((n.meshNetwork[slot].sts & LORA_MESH_NODE_REGISTERED) == LORA_MESH_NODE_REGISTERED)
     || (( n.meshNetwork[slot].sts & LORA_MESH_NODE_ALIVE) == LORA_MESH_NODE_ALIVE) ){ 
           if (cnt1>0) ss += ",";
           for (char i = 0; i < 3; i++) {           
               _nod0 = n.meshNetwork[slot].path[i];
               _nod1 = n.meshNetwork[slot].path[i+1];
               if (_nod1 != 0x00) { 
                  if (i>0) ss += ",";
                  ss += "{ \"id\":\"" + String(_nod0) + String (_nod1)  + "\",\"from\":\"" + String(_nod0) + "\", \"to\":\"" + String(_nod1) + "\"}" ; 
                  cnt1++;
               }
           }
      }
  }
           
  ss += "]}";

 
server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
server.sendHeader(F("Access-Control-Max-Age"), F("600"));
server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));

server.send(200, "application/json", ss);

}
void setup(void) {

  Serial.begin(115200);
  espSerial.begin(19200);
   
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

 server.on("/getNetwork", HTTP_OPTIONS, []() {
   server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  server.sendHeader(F("Access-Control-Max-Age"), F("600"));
  server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
  server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));

    server.send(204);
 });
 
  server.on("/getNetwork",HTTP_GET, handleNetwork);
  server.on("/", handleNetwork);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();

}
