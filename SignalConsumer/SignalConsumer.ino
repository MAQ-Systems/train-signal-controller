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

#define MESSAGE_TERMINATING_CHAR '!'

#define GREEN_PIN 5
#define YELLOW_PIN 6
#define RED_PIN 7

typedef enum {
  RED = 0,
  YELLOW = 1,
  GREEN = 2
} SignalColor;

typedef enum {
  ON = 0,
  BLINK = 1,
  OFF = 2
} LampState;

/**
 * SignalMessage struct for parsing signal messages
 */
typedef struct {
  LampState lampState;
  SignalColor color;
} SignalMessage;

// network info
byte mac[] = {0x90, 0xA2, 0xDA, 0x0F, 0x2C, 0x47};
byte staticIp[] = {192, 168, 1, 150};
//byte serverIp[] = {24, 15, 183, 66};
byte serverIp[] = {192, 168, 1, 105};
char serverName[] = "www.iotitan.com";
int serverPort = 19100;

EthernetClient client;
char readBuffer[BUFFER_SIZE];
int readBufferPos;

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
  
  // Try to get connection through dhcp.
  if(Ethernet.begin(mac) == 0) {
    Serial.println("Failed DHCP using static IP.");
    // if dhcp didn't work, use static
    Ethernet.begin(mac, staticIp);
    // Allow ethernet setup to finish.
    delay(1000);
  } else {
    // Allow ethernet setup to finish.
    delay(1000);
    // If dhcp, request that it be maintained.
    Ethernet.maintain();
  }
  
  connectToServer();

  if (client.connected()) {
    Serial.println("Connected!");
  } else {
    Serial.println("Initial connection failed!");
  }

  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
}

/**
 * Connect/reconnect client to server
 * @return True if successful
 */
boolean connectToServer() {
  client.connect(serverIp, serverPort);
  if(!client.connected()) return false;
  return true;
}

/**
 * Main read/update loop
 */
void loop() {
  if(!client.connected()) {
    client.stop();
    if(!connectToServer()) {
      Serial.println("Waiting to reconnect...");
      delay(10000); // if we can't connect, wait 10 sec and try again
    }
    else {
      Serial.println("Reconnected!"); 
    }
  }

  // Whether the network read was a signal message or empty.
  bool readWasMessage = false;
  
  // wait for signal to switch for a bit before trying to read another (5 sec)
  if(client.connected()) {
    readBufferPos = 0;
    while((readBuffer[readBufferPos] = client.read()) != -1
        && readBufferPos < BUFFER_SIZE-1
        && readBuffer[readBufferPos] != MESSAGE_TERMINATING_CHAR) {
      readBufferPos++;
    }
    readBuffer[readBufferPos] = MESSAGE_TERMINATING_CHAR;
    
    // parse the message and switch appropriate pins
    SignalMessage* sm = parseSignalMessage(readBuffer, readBufferPos);
    if(sm != NULL) {
      readWasMessage = true;
      handleSignalMessage(sm);
      delete sm;
    }
  }

  // If a signal was received, wait 2.5 sec for it to change.
  if (readWasMessage) {
    delay(2500);
  } else {
    delay(100);
  }
}

/**
 * Print a c-string (an array of characters) untill null terminator or "len" is reached
 * @param msg Pointer to character array
 * @param len The length of the message/array
 */
void printData(char* msg, int len) {
  Serial.write("\nlength: ");
  Serial.write(len);
  Serial.println();
  int i;
  for(i = 0; i < len; i++) {
    if(msg[i] == MESSAGE_TERMINATING_CHAR) {
      break;
    }
    Serial.write(msg[i]);
  }
  Serial.println();
  for(i = 0; i < len; i++) {
    if(msg[i] == MESSAGE_TERMINATING_CHAR) {
      break;
    }
    Serial.write((int)msg[i]);
    Serial.write(", ");
  }
  Serial.println();
}

/**
 * Parse signal message and set appropriate pins
 * @param smp A pointer to the signal message
 */
void handleSignalMessage(void* smp) {
  SignalMessage* sm = (SignalMessage*)smp;

  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);

  Serial.write("Handling message: ");
  if (sm->color == RED) {
    digitalWrite(RED_PIN, HIGH);
    Serial.write("RED");
  } else if (sm->color == YELLOW) {
    digitalWrite(YELLOW_PIN, HIGH);
    Serial.write("YELLOW");
  } else if (sm->color == GREEN) {
    digitalWrite(GREEN_PIN, HIGH);
    Serial.write("GREEN");
  }

  if (sm->lampState == ON) {
    Serial.write(" ON");
  } else if (sm->lampState == BLINK) {
    Serial.write(" BLINK");
  } else if (sm->lampState == OFF) {
    Serial.write(" OFF");
  }
  
  Serial.println("\n");
}

/**
 * Determine if a given message loosely follows protocol
 * @param msg The character array containing the message from a client
 *            trying to write to the queue.
 * @param len The length of the message
 */
boolean isValidMessage(char* msg, int len) {
  if(msg == NULL || len <= 0) return false;

  // invalid code?
  if((msg[0] & SIGNAL_BASE) == 0) return false;

  // make sure message is not too large
  if(len > 32) return false;

  // make sure message closing character is present
  if(msg[len-1] == MESSAGE_TERMINATING_CHAR) return false;

  return true;
}

/**
 * Parse message into SignalMessage struct
 * @param msg The char array pointer from the client
 * @param len The length of the message
 * @return Pointer to a new SignalMessage
 */
SignalMessage* parseSignalMessage(char* msg, int len) {
  if(!isValidMessage(msg, len)) return NULL;

  SignalMessage* sm = new SignalMessage;

  // init signal message
  sm->lampState = OFF;
  sm->color = RED;

  if ((msg[0] & SIGNAL_RED) > 0) {
    sm->color = RED;
  } else if ((msg[0] & SIGNAL_YELLOW) > 0) {
    sm->color = YELLOW;
  } else if ((msg[0] & SIGNAL_GREEN) > 0) {
    sm->color = GREEN;
  }
  
  return sm;
}

