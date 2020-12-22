LoRa + WiFi Mesh Network for Arduino (AVR,ESP8266,ESP32)
===========================================

# Disclainer

 This SOFTWARE PRODUCT is provided by THE PROVIDER "as is" and "with all faults." THE PROVIDER makes no representations or warranties of any kind concerning the safety, suitability, lack of viruses, inaccuracies, typographical errors, or other harmful components of this SOFTWARE PRODUCT. There are inherent dangers in the use of any software, and you are solely responsible for determining whether this SOFTWARE PRODUCT is compatible with your equipment and other software installed on your equipment. You are also solely responsible for the protection of your equipment and backup of your data, and THE PROVIDER will not be liable for any damages you may suffer in connection with using, modifying, or distributing this SOFTWARE PRODUCT
 

# LoRa + WiFi mesh network
© 2020 by antónio montez . All rights reserved.
Licensed under the MIT license.


This is a (another) library to create a mesh network of different devices and protocols.
It use nonless a different approach
    1) LoRa 
        It doesn't use LoRaWan. it use a custom protocols.
        It has deliver and confirmation of messages
        It has a OTA configuration approach (for setup data not flash code)
        It use retry and refresh interval
    2) WiFi
        It doesn't use "normal" WiFi or TCP over WiFi. 
        IT uses the ESP_NOW library allowing then peer to peer communications
        

# Contents
    Library
    Generic node code (Master & Salve) example
    
    

# use

  Generic code to implement a LoraWifiMesh node. 
  It support 255 Node Address. 
  For readable porposes I'm using : Ascii printable node names :"123456780@ABCDEFEDGH....."
  There are 3 important parameters :

    byte localAddress = 0x31;             // defines this node address... however I'm using the ArduinoUniqueID library to automatic generate the localAddress
    byte Protocol = MESH_PROTOCOL_LORA;   // the two protocols support at the moment  MESH_PROTOCOL_LORA && MESH_PROTOCOL_WIFI
    #define MASTER_NODE 0x39              // define which node will be used as Master..take in consideration that every node can be "slave" and master at same time... 
                                              // this is just a parameter that says which will be the MASTER FOR THIS NODE.
  and a couple of mandatory function calls:

    LWMesh.setProtocol(Protocol);                         // self explanatory
    LWMesh.setConfig(nc);                                 // configure a Structure with all config parameters
    LWMesh.addNodeToNetwork(localAddress,_mac,Protocol);  // Registers the node to the define MASTER node. 
                                                         This is not mandatory. this is just to tell the MASTER that this node exists.

  You can then check for messages received
  and send messages with :
          
    byte _id = LWMesh.sendMsg(nodeAddress,chars,path);   // when you known the path : example "942" .... this node (9) wants to send a message to node (2) passing by node (4)
    byte _id = LWMesh.sendMsg(nodeAddress,chars);        // when you don't know the path... the routing protocol will then try to find a route and if found send the message.
              
  all sent and received messages are (non blocking)...and all sent messages will have a ACK ...and retry.
  so after calling LWMesh.sendMsg... there's no confirmation yet of the deliver.
  you need to go and check in:
  
    LWMesh.hasMsg(&rec)) 

function will return not only the NORMAL messages receives.... 
but also the CONFIRMATION (ACK | TIMEOUT) of the sent messages... 
the message ID (_id)... is then your key for matching pair send/received.
The confirmation is Automatic... you don't need to take care this in your code.

This code has been tested and runs on ESP8266, ESP32 running protocol WIFI
This code has been tested and runs on HELTEC Board (ESP32) LORA and Arduino pro-mini, connecting to a  LORA-02 generic board... 
however the memory available on the Arduino is on the limits...
.. therefore, I recommend using a ESP8266 instead.


# version 1.0.0
    Very first release
    Tested against seveeral ESP8266 , ESP 32 for WiFi
    Tested against Heltec LoRa boards and lora-02 chips
    Tested using Arduino Pro-mini
    
# Next version 2.0.0 ( in beta )
    Added example for WebView of the Network
    Supporting commands directly in the ui.
    Support for OTA cnfiguration commands
    Support for sleep and syncronized wakeup
    Support for both proctocols at same time (LoRa +WiFi) basically having a node actiong as gateway between the two protocols
    
    
# Documentation

    TBD
    
# Issues and support

    TBD

# License and credits


# Other useful links
    
    TBD
