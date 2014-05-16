/**
 * File: SignalProgram.ino
 * Author: Matt Jones
 * Model: Arduino Uno
 * Date: 5/15/14
 * Desc: The arduino client program to consume signal commands from
 *       the signal queue program living on the server.
 */
 
#include <SPI.h>
#include <Ethernet.h>

#define BUFFER_SIZE 128 // 128 should be plenty for anything we are receiving

// network info
byte mac[] = {0x90, 0xA2, 0xDA, 0x0F, 0x2C, 0x47};
byte staticIp[] = {192, 168, 1, 150};
byte serverIp[] = {69, 243, 189, 115};
char serverName[] = "www.iotitan.com";
int serverPort = 19100;

EthernetClient client;
char readBuffer[BUFFER_SIZE];
int readBufferPos;

/**
 * Setup initial 
 */
void setup() {
  Serial.begin(9600);
  
  // try to get connection through dhcp
  if(Ethernet.begin(mac) == 0) {
    // if dhcp didn't work, use static
    Ethernet.begin(mac, staticIp);
  }
  
  // allow ethernet setup to finish
  delay(1000);
  
  Serial.println("Connected?");
  
  connectToServer();
  
  // if dhcp, request that it be maintained
  //Ethernet.maintain();
}

/**
 * Connect/reconnect client to server
 * @return True if successful
 */
boolean connectToServer() {
  client.connect(serverIp, serverPort);
  if(!client.connected()) {
    return false; 
  }
  // read intro message
  readBufferPos = 0;
  while((readBuffer[readBufferPos] = client.read()) != '\0' && readBufferPos < BUFFER_SIZE) {
    readBufferPos++;
  }
  readBuffer[readBufferPos] = '\0';
  
  int i;
  for(i = 0; i < readBufferPos; i++) {
    Serial.print(readBuffer[i]);
  }
  Serial.println();
  
  // notify server we intend to read
  client.write('R');
  client.flush();
  return true;
}

/**
 * Main read/update loop
 */
void loop() {
  if(!client.connected()) {
    if(!connectToServer()) {
      delay(10000); // if we can't connect, wait 10 sec and try again
    }
  }
  
  if(client.connected()){
    // read intro message
    readBufferPos = 0;
    while((readBuffer[readBufferPos] = client.read()) != '\0' && readBufferPos < BUFFER_SIZE) {
      readBufferPos++;
    }
    readBuffer[readBufferPos] = '\0';
    
    int i;
    for(i = 0; i < readBufferPos; i++) {
      Serial.print(readBuffer[i]);
    }
    Serial.println();
  }
}

