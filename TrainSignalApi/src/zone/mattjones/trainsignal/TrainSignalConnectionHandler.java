/**
 * File: TrainSignalConnectionHandler.java
 * Author: Matt Jones
 * Date: 7.11.2018
 * Desc: The handler that waits for an incoming connection from the train signal so that it can
 * 		 begin sending commands to it. This class only handles a single connection. If a new
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
				Socket newConnection = mServerSocket.accept();
				if (mActiveClientSocket != null && mActiveClientSocket.isConnected()) {
					mActiveClientSocket.close();
				}
				mActiveClientSocket = newConnection;
				sendMessages();
			} catch (IOException e) {
				// Do nothing
			}
			
			// If we reach this point, the server has connected a new client. Sleep for 5 seconds
			// in case the system is failing to connect (the client should be trying to connect far
			// fewer times than this).
			try {
				Thread.sleep(5000);
			} catch (InterruptedException e) {
				// Do nothing if sleep failed.
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
