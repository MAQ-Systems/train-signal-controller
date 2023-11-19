/**
 * File: SearchlightSignal.ino
 * Author: Matt Jones
 * Model: Arduino Uno
 * Date: 2023.11.16 (original: 2014.05.15)
 * Desc: The arduino client program to consume and translate signal messages
 *       received from the server.
 *       This program does not poll the server for updates, but instead opens
 *       a TCP socket that the server hosts. This allows commands to be
 *       processed immediately after sending rather than blocking or timing out
 *       waiting for a HTTP response.
 */
 
#include <SPI.h>
#include <Ethernet.h>
#include <string.h>

// 32 bytes is plenty for signal messages (usually 1 byte).
// TODO(Matt): If the signal is eventually named, that will be sent _from_ the
//             signal and used to reference the device on the server. So this
//             buffer can probably be even smaller. Name can possibly be
//             provided via SD card.
#define BUFFER_SIZE 32

// Different states for a signal to be in
#define SIGNAL_BASE     1  // 00000001
#define SIGNAL_BLINK    2  // 00000010
#define SIGNAL_RED      4  // 00000100
#define SIGNAL_YELLOW   8  // 00001000
#define SIGNAL_GREEN    16 // 00010000
#define SIGNAL_LAMP_ON  32 // 00100000
#define SIGNAL_LAMP_OFF 64 // 01000000

#define MESSAGE_TERMINATING_CHAR '!'

// Use pins that aren't dual purpose.
#define ASPECT_POWER_PIN 5
#define ASPECT_IN_PIN 6
#define ASPECT_OUT_PIN 7
#define LAMP_FLASH_PIN 8

// A message to send back to the server for each one that is received. This is
// just the "0" state with the terminating character.
byte ACK_MESSAGE[] = {SIGNAL_BASE, MESSAGE_TERMINATING_CHAR};
int ACK_MESSAGE_SIZE = 2;

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
  bool isAck;
} SignalMessage;

// network info
byte mac[] = {0x90, 0xA2, 0xDA, 0x0F, 0x2C, 0x47};
byte staticIp[] = {192, 168, 32, 100};
char serverName[] = "mattjones.zone";
int serverPort = 19100;

EthernetClient client;
char readBuffer[BUFFER_SIZE];
int readBufferPos;

// prototypes
boolean connectToServer();
void printData(char* msg, int len);
boolean isValidMessage(char* msg, int len);
SignalMessage* parseSignalMessage(char* msg, int len);
void handleSignalMessage(SignalMessage* smp);
bool isAckMessage(char* msg, int len);
void setupPins();
void setColor(SignalColor color);
void setLamp(LampState state);

/**
 * Setup initial 
 */
void setup() {
  Serial.begin(9600);

  setupPins();

  setColor(SignalColor::RED);
  setLamp(LampState::ON);
  
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
}

/**
 * Connect/reconnect client to server
 * @return True if successful
 */
boolean connectToServer() {
  //byte serverIp[] = {192, 168, 32, 42};
  //client.connect(serverIp, serverPort);
  client.connect(serverName, serverPort);
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
  
  // wait for signal to switch for a bit before trying to read another (2.5 sec)
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

      // If there wasn't a null message send back and acknowledge that the
      // message was received.
      client.write(ACK_MESSAGE, ACK_MESSAGE_SIZE);
      client.flush();
    }
  }

  // If a signal was received, wait 2.5 sec for it to change.
  if (readWasMessage) {
    delay(2500);
  } else {
    // Otherwise poll every 500ms.
    delay(500);
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
void handleSignalMessage(SignalMessage* sm) {
  Serial.println("Handling message:");

  if (sm->isAck) {
    Serial.println("  Ping pong");
  } else {
    setColor(sm->color);
    setLamp(sm->lampState);
    Serial.println("\n");
  }
}

void setupPins() {
  pinMode(ASPECT_POWER_PIN, OUTPUT);
  pinMode(ASPECT_IN_PIN, OUTPUT);
  pinMode(ASPECT_OUT_PIN, OUTPUT);
  pinMode(LAMP_FLASH_PIN, OUTPUT);
}

void setColor(SignalColor color) {
  // Cut power to the aspect/color changer prior to switching the color so we
  // don't temporarily short the power supply.
  digitalWrite(ASPECT_POWER_PIN, HIGH);
  delay(250);

  bool enableAspectPower = false;

  if (color == RED) {
    // Turning off the power to the aspect/color switcher means the magnet
    // receives no power and falls to "neutral" which is red (center roundlet).
    enableAspectPower = false;

    // The other aspect in/out pins don't matter in this case, but we set them
    // HIGH so the relays go to their "normally open" position.
    digitalWrite(ASPECT_IN_PIN, HIGH);
    digitalWrite(ASPECT_OUT_PIN, HIGH);
    
    Serial.println("  COLOR: RED");
  } else if (color == YELLOW) {
    // Send power to the aspect/color switcher.
    enableAspectPower = true;

    // Set the polarity to swing the arm to yellow (right roundlet).
    // THE IN AND OUT PINS MUST MATCH FOR YELLOW AND GREEN.
    digitalWrite(ASPECT_IN_PIN, LOW);
    digitalWrite(ASPECT_OUT_PIN, LOW);

    Serial.println("  COLOR: YELLOW");
  } else if (color == GREEN) {
    // Send power to the aspect/color switcher.
    enableAspectPower = true;

    // Set the polarity to swing the arm to green (left roundlet).
    // THE IN AND OUT PINS MUST MATCH FOR YELLOW AND GREEN.
    digitalWrite(ASPECT_IN_PIN, HIGH);
    digitalWrite(ASPECT_OUT_PIN, HIGH);

    Serial.println("  COLOR: GREEN");
  } else {
    Serial.println("  COLOR: NOT HANDLED!");
  }

  if (enableAspectPower) {
    // Same as at the top of this function. Wait until the relays have flipped
    // so we don't short the power (if turning it back on).
    delay(250);
    digitalWrite(ASPECT_POWER_PIN, LOW);
  }
}

void setLamp(LampState state) {
  if (state == ON) {
    digitalWrite(LAMP_FLASH_PIN, HIGH);
    Serial.println("  LAMP: ON");
  } else if (state == BLINK) {
    digitalWrite(LAMP_FLASH_PIN, LOW);
    Serial.println("  LAMP: BLINK");
  } else if (state == OFF) {
    // Currently unsupported. If the signal is plugged in the lamp is either on
    // or blinking.
    Serial.println("  LAMP: OFF (currently unsupported)");
  } else {
    Serial.println("  LAMP: STATE NOT HANDLED!");
  }
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
  sm->isAck = false;

  // First check if this is a ping pong message. If it is, we don't need to do
  // anything else.
  if (isAckMessage(msg, len)) {
    sm->isAck = true;
    return sm;
  }

  if ((msg[0] & SIGNAL_RED) > 0) {
    sm->color = RED;
  } else if ((msg[0] & SIGNAL_YELLOW) > 0) {
    sm->color = YELLOW;
  } else if ((msg[0] & SIGNAL_GREEN) > 0) {
    sm->color = GREEN;
  }

  if ((msg[0] & SIGNAL_LAMP_ON) > 0) {
    sm->lampState = ON;
  } else if ((msg[0] & SIGNAL_BLINK) > 0) {
    sm->lampState = BLINK;
  } else if ((msg[0] & SIGNAL_LAMP_OFF) > 0) {
    sm->lampState = OFF;
  }
  
  return sm;
}

bool isAckMessage(char* msg, int len) {
  return isValidMessage(msg, len) && (msg[0] | SIGNAL_BASE) == SIGNAL_BASE;
}
