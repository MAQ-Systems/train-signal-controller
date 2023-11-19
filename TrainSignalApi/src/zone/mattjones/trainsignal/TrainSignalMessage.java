/**
 * File: TrainSignalMessage.java
 * Author: Matt Jones
 * Date: 7.10.2018
 * Desc: Utilities for producing and parsing messages from the train signal controller (the
 *          arduino).
 * 
 * Protocol:
 *       Since there are a grand total of 8 possible things the signal
 *       can do, the message format will be simple:
 *       - One byte for state - based on which bits are set in this byte
 *           - [0] ALWAYS SET TO 1
 *           - [1] blink flag
 *           - [2] red flag
 *           - [3] yellow flag
 *           - [4] green flag
 *           - [5] turn lamp on
 *           - [6] turn lamp off
 *           - [7] unused
 *       - '!' to close the message
 * Reserved Chars:
 *      '[', ']', '|', '\0'
 */

package zone.mattjones.trainsignal;

public class TrainSignalMessage {
    /** Possible colors that the signal can be. */
    public enum SignalColor {
        RED,
        YELLOW,
        GREEN
    }
    
    /** States that the lamp can be in. */
    public enum LampState {
        ON,
        BLINK,
        OFF
    }
    
    // Different states for a signal to be in
    private static final byte SIGNAL_BASE     = 1;  // 00000001
    private static final byte SIGNAL_BLINK    = 2;  // 00000010
    private static final byte SIGNAL_RED      = 4;  // 00000100
    private static final byte SIGNAL_YELLOW   = 8;  // 00001000
    private static final byte SIGNAL_GREEN    = 16; // 00010000
    private static final byte SIGNAL_LAMP_ON  = 32; // 00100000
    private static final byte SIGNAL_LAMP_OFF = 64; // 01000000

    /** The character all messages should end with. */
    private static final char MESSAGE_TERMINATING_CHAR = '!';

    /** The acknowledgement message expected for each message sent to the signal. */
    public static final byte[] ACK_MESSAGE = {SIGNAL_BASE, MESSAGE_TERMINATING_CHAR};

    /** Private constructor to prevent instantiation. */
    private TrainSignalMessage() {}
    
    /**
     * Generate a message to change the state of the train signal. This should be sent to the
     * arduino that's physically connected to the signal.
     * @param signalColor The color the signal should be.
     * @param lampState The state of the lamp (on/off/blink).
     * @return A message byte array.
     */
    public static byte[] generateMessage(SignalColor signalColor, LampState lampState) {
        byte signalState = SIGNAL_BASE;
        
        switch(signalColor) {
            case RED:
                signalState |= SIGNAL_RED;
                break;
            case YELLOW:
                signalState |= SIGNAL_YELLOW;
                break;
            case GREEN:
                signalState |= SIGNAL_GREEN;
                break;
        }
        
        switch(lampState) {
            case ON:
                signalState |= SIGNAL_LAMP_ON;
                break;
            case BLINK:
                signalState |= SIGNAL_BLINK;
                break;
            case OFF:
                signalState |= SIGNAL_LAMP_OFF;
                break;
        }
        
        byte[] message = new byte[2];
        message[0] = signalState;
        message[1] = MESSAGE_TERMINATING_CHAR;
        
        return message;
    }

    /**
     * @param message The message to check.
     * @return Whether a message received from the signal is an acknowledgment of a sent message.
     */
    public static boolean isAckMessage(byte[] message) {
        return message != null && message.length == ACK_MESSAGE.length &&
                ACK_MESSAGE[0] == message[0] && ACK_MESSAGE[1] == message[1];
    }

    /**
     * @param message The message to check.
     * @return Whether the message is a properly terminated message (contains the end character).
     */
    public static boolean isTerminatedMessage(byte[] message) {
        return message != null && message.length > 0 && message[message.length - 1] == MESSAGE_TERMINATING_CHAR;
    }
}
