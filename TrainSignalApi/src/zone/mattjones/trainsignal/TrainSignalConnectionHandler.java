/**
 * File: TrainSignalConnectionHandler.java
 * Author: Matt Jones
 * Date: 7.11.2018
 * Desc: The handler that waits for an incoming connection from the train signal so that it can
 *          begin sending commands to it. This class only handles a single connection. If a new
 *       connection is made, the existing client is disconnected.
 */

package zone.mattjones.trainsignal;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.ConcurrentLinkedQueue;

public class TrainSignalConnectionHandler extends Thread {
    /** The port for the server to run on. */
    private final int mPort;
    
    /** The socket listening for incoming connections. */
    private ServerSocket mServerSocket;
    
    /** The client (train signal arduino) currently connected to the server. */
    private Socket mActiveClientSocket;
    
    /** Messages waiting to be sent to the train signal. */
    // TODO(Matt): This probably doesn't need to be a list since messages are sent immediately. The
    //             only reason to keep this would be for diagnostic messages.
    private ConcurrentLinkedQueue<byte[]> mMessages;
    
    /** Whether the server should stop on the next iteration. */
    private boolean mStopServer;
    
    /** Default constructor. */
    public TrainSignalConnectionHandler(int serverPort) {
        mPort = serverPort;
        mMessages = new ConcurrentLinkedQueue<>();
        start();
    }
    
    @Override
    public void run() {
        while (!mStopServer) {
            try {
                if (mServerSocket == null || mServerSocket.isClosed()) {
                    mServerSocket = new ServerSocket(mPort);
                }

                // Block until a connection is made. Currently, it's only possible to run with a
                // single connected client.
                Socket newConnection = mServerSocket.accept();
                if (mActiveClientSocket != null && mActiveClientSocket.isConnected()) {
                    mActiveClientSocket.close();
                }
                mActiveClientSocket = newConnection;

                while (mActiveClientSocket.isConnected()) {
                    sendMessages();

                    // Sleep for one minute. A message being added to the queue will interrupt this.
                    try {
                        Thread.sleep(60000);
                    } catch (InterruptedException e) {
                        System.err.println("[info]: Messaging thread interrupted - a message was "
                                + "likely enqueued.");
                    }
                }

            } catch (IOException e) {
                System.err.println("[error]: Messaging thread exception: " + e.getMessage());
            }
        }
    }
    
    /**
     * Send any messages in the queue to the client.
     */
    private void sendMessages() {
        if (mMessages.isEmpty() || mActiveClientSocket == null
                || !mActiveClientSocket.isConnected()) {
            return;
        }
        
        try {
            while (!mMessages.isEmpty()) {
                byte[] curMessage = mMessages.poll();
                mActiveClientSocket.getOutputStream().write(curMessage);
                mActiveClientSocket.getOutputStream().flush();
            }
        } catch (IOException e) {
            // Do nothing.
        }
    }
    
    /**
     * Add a message to the queue. If a client is connected, the message is sent immediately.
     * @param message The message to send to the client.
     */
    public void addMessage(byte[] message) {
        mMessages.add(message);
        interrupt();
        sendMessages();
    }
    
    /**
     * Cause the server to close the connection and reopen it.
     * @throws IOException
     */
    public void resetServer() throws IOException {
        if (mServerSocket != null) mServerSocket.close();
        if (mActiveClientSocket != null) mActiveClientSocket.close();
    }
    
    /**
     * Stops the server socket and exits the thread.
     * @throws IOException
     */
    public void killServer() throws IOException {
        mStopServer = true;
        resetServer();
    }
}
