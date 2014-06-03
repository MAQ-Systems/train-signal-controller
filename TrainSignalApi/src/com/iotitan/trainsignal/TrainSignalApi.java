/**
 * File: TrainSignalApi.java
 * Author: Matt Jones
 * Date: 6/1/2014
 * Desc: REST API for communicating with server program that controls the train signal.
 */

package com.iotitan.trainsignal;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;

import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;


public class TrainSignalApi extends HttpServlet {
	
	private static final long serialVersionUID = 06012014L;
    private static Socket queueCon;
    //private static final String serverIp = "127.0.0.1";
    private static final String serverIp = "24.15.183.66";
    private static final int serverPort = 19100;
    
    // Different states for a signal to be in
    public static final int SIGNAL_BASE    = 1;  // 00000001
	public static final int SIGNAL_BLINK   = 2;  // 00000010
	public static final int SIGNAL_RED     = 4;  // 00000100
	public static final int SIGNAL_YELLOW  = 8;  // 00001000
	public static final int SIGNAL_GREEN   = 16; // 00010000
	public static final int SIGNAL_LAMP_ON = 32; // 00100000
	public static final int SIGNAL_LAMP_OFF= 64; // 01000000
	
	public static void main(String[] args) {
		Socket soc = connectToQueue(serverIp, serverPort);
		try {
			InputStream is = soc.getInputStream();
			OutputStream os = soc.getOutputStream();
			
			byte[] buff = new byte[32];
			
			int size = is.read(buff);
			for(int i = 0; i < size; i++) {
				System.out.print((char)buff[i]);
			}
			
			os.write("R|F|1\0".getBytes());
			
			
			
/*			os.write("W|F|2\0".getBytes());
			os.write("W|F|3\0".getBytes());
			os.flush();
			os.close();
*/			
			
			
			while((size = is.read(buff)) > 0) {
				for(int i = 0; i < size; i++) {
					System.out.print((char)buff[i]);
				}
			}
		}
		catch(Exception ex) {
			
		}
	}
	
	@Override
    public void init(ServletConfig config) {
		
    }

	@Override
	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		
	}
	
	/**
	 * Connect to the signal queue
	 * @param sIp The server's IP address in the form of a 4 element byte array
	 * @param sPort The server's port
	 */
	private static synchronized Socket connectToQueue(String sIp, int sPort) {
		Socket con = null;
		try {
			con = new Socket(InetAddress.getByName(sIp),sPort);
		}
		catch(Exception ex) {
			// TODO: logger error
			System.out.println(ex.getMessage());
		}
		return con;
	}
	
	
}
