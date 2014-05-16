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

// network info
byte mac[] = {0x90, 0xA2, 0xDA, 0x0F, 0x2C, 0x47};
byte staticIp[] = {192, 168, 1, 150};
byte serverIp[] = {69, 243, 189, 115};
char serverName[] = "www.iotitan.com";
int serverPort = 19100;

EthernetClient client;

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
  // notify server we intend to read
  client.write('R');
  client.flush();
  return true;
}

/**
 * Main read/update loop
 */
void loop() {
  
}

