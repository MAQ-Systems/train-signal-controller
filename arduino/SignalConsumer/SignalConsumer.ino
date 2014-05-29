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
#include <string.h>

#define BUFFER_SIZE 32 // 32 should be plenty for anything we are receiving

// Different states for a signal to be in
#define SIGNAL_BASE     1  // 00000001
#define SIGNAL_BLINK    2  // 00000010
#define SIGNAL_RED      4  // 00000100
#define SIGNAL_YELLOW   8  // 00001000
#define SIGNAL_GREEN    16 // 00010000
#define SIGNAL_LAMP_ON  32 // 00100000
#define SIGNAL_LAMP_OFF 64 // 01000000

/**
 * SignalMessage struct for parsing signal messages
 */
typedef struct signalMessage {
  char clientType;
  boolean error;
  char signalState;
} SignalMessage;

// network info
byte mac[] = {0x90, 0xA2, 0xDA, 0x0F, 0x2C, 0x47};
byte staticIp[] = {192, 168, 1, 150};
byte serverIp[] = {24, 15, 183, 66};
char serverName[] = "www.iotitan.com";
int serverPort = 19100;

EthernetClient client;
char readBuffer[BUFFER_SIZE];
int readBufferPos;
long prevTime;

// prototypes
boolean connectToServer();
void printData(char* msg, int len);
boolean isValidMessage(char* msg, int len);
SignalMessage* parseSignalMessage(char* msg, int len);
void handleSignalMessage(void* smp);


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
  
  connectToServer();
  
  // if dhcp, request that it be maintained
  //Ethernet.maintain();
  prevTime = millis();
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
  
  printData(readBuffer, readBufferPos);
  Serial.println();
  
  // notify server we intend to read
  client.print("R|F|1\0");
  
  return true;
}

/**
 * Main read/update loop
 */
void loop() {
  
  long curTime = millis();
  
  if(!client.connected()) {
    client.stop();
    if(!connectToServer()) {
      Serial.println("waiting to reconnect");
      delay(10000); // if we can't connect, wait 10 sec and try again
    }
    else {
      Serial.println("reconnected successfully!"); 
    }
  }
  
  // wait for signal to switch for a bit before trying to read another (5 sec)
  if(client.connected() && curTime - prevTime > 5000) {
    prevTime = curTime;
    
    readBufferPos = 0;
    while((readBuffer[readBufferPos] = client.read()) != '\0' && readBufferPos < BUFFER_SIZE-1) {
      Serial.write(readBuffer[readBufferPos]);
      readBufferPos++;
    }
    readBuffer[readBufferPos] = '\0';
    
    printData(readBuffer, readBufferPos);
    Serial.println();
    
    // parse the message and switch appropriate pins
    SignalMessage* sm = parseSignalMessage(readBuffer, readBufferPos);
    if(sm != NULL) {
      handleSignalMessage(sm);
      delete sm;
    }
  }
}

/**
 * Print a c-string (an array of characters) untill null terminator or "len" is reached
 * @param msg Pointer to character array
 * @param len The length of the message/array
 */
void printData(char* msg, int len) {
  int i;
  for(i = 0; i < len; i++) {
    if(msg[i] == '\0') {
      break;
    }
    Serial.write(msg[i]);
  }
}

/**
 * Parse signal message and set appropriate pins
 * @param smp A pointer to the signal message
 */
void handleSignalMessage(void* smp) {
  SignalMessage* sm = (SignalMessage*)smp;
  int state = (int)sm->signalState;
  
  // invalid code?
  if(state & SIGNAL_BASE == 0) {
    return;
  }
  
  Serial.println("\n\n");
  
  // blink?
  if(state & SIGNAL_BLINK > 0) {
    Serial.println("Blinking");
  }
  
  // signal color
  if(state & SIGNAL_RED > 0) {
    Serial.println("Red");
  }
  else if(state & SIGNAL_YELLOW > 0) {
    Serial.println("Yellow");
  }
  else if(state & SIGNAL_GREEN > 0) {
    Serial.println("Green");
  }
  
  // lamp on or off
  if(state & SIGNAL_LAMP_ON > 0) {
    Serial.println("Turning lamp on...");
  }
  
  if(state & SIGNAL_LAMP_OFF > 0) {
    Serial.println("Turning lamp off...");
  }
}

/**
 * Determine if a given message loosely follows protocol
 * @param msg The character array containing the message from a client
 *            trying to write to the queue.
 * @param len The length of the message
 */
boolean isValidMessage(char* msg, int len) {
  if(msg == NULL) {
    return false;
  }

  // make sure message is not too large
  if(len > 32) {
    return false;
  }

  // make sure message closing character is present
  if(msg[len-1] == '\0') {
    return false;
  }

  return true;
}

/**
 * Parse message into SignalMessage struct
 * @param msg The char array pointer from the client
 * @param len The length of the message
 * @return Pointer to a new SignalMessage
 */
SignalMessage* parseSignalMessage(char* msg, int len) {
  if(!isValidMessage(msg, len)) {
    return NULL;
  }

  SignalMessage* sm = new SignalMessage;

  // init signal message
  sm->clientType = 'X';
  sm->error = false;
  sm->signalState = 1;

  char *tokSave;
  char *token = strtok_r(msg, "|", &tokSave);
  int elementNo = 0;

  // fill struct by stepping through tokens (easy to keep default settings)
  while(token != NULL) {
    if(strlen(token) > 0) {
      switch(elementNo) {
        case 0:
          sm->clientType = token[0];
          break;

        case 1:
          sm->error = token[0]=='T'?true:false;
          break;

        case 2:
          sm->signalState = token[0];
          break;
      }
    }
    elementNo++;
    token = strtok_r(NULL, "|", &tokSave);
  }

  return sm;
}

