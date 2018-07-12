/**
 * File: TrainSignalApi.java
 * Author: Matt Jones
 * Date: 6/1/2014
 * Desc: REST API for communicating with server program that controls the train signal.
 */

package zone.mattjones.trainsignal;

import java.io.IOException;
import java.net.Socket;

import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import zone.mattjones.trainsignal.TrainSignalMessage.LampState;
import zone.mattjones.trainsignal.TrainSignalMessage.SignalColor;

public class TrainSignalApi extends HttpServlet {
	private static final long serialVersionUID = 06012014L;

	/** The port for the connection to the train signal to run on. */
    private static final int SERVER_PORT = 19100;
	
	/** The thread handling connections to the train signal. */
	private TrainSignalConnectionHandler mConnectionHandler;
	
    /**
     * Constructor that sets up the server socket listener for the arduino to connect to.
     */
    public TrainSignalApi() {
    	mConnectionHandler = new TrainSignalConnectionHandler(SERVER_PORT);
    }
    
	@Override
    public void init(ServletConfig config) {}

	@Override
	protected void doGet(HttpServletRequest request, HttpServletResponse response)
			throws ServletException, IOException {
		// Read input from the params in the URL.
		String colorString = request.getParameter("color");
		SignalColor color = SignalColor.RED;
		if (colorString != null) {
			switch(colorString.charAt(0)) {
				case 'R':
				case 'r':
					color = SignalColor.RED;
					break;
				case 'Y':
				case 'y':
					color = SignalColor.YELLOW;
					break;
				case 'G':
				case 'g':
					color = SignalColor.GREEN;
					break;
			}
		}
		
		String lampString = request.getParameter("lamp");
		LampState lamp = LampState.OFF;
		if (lampString != null) {
			switch(lampString.charAt(0)) {
				case '1':
					lamp = LampState.ON;
					break;
				case '0':
					lamp = LampState.OFF;
					break;
				case 'B':
				case 'b':
					lamp = LampState.BLINK;
					break;
			}
		}
		
		byte[] signalMessage = TrainSignalMessage.generateMessage(color, lamp);
		mConnectionHandler.addMessage(signalMessage);
		
		String messageString = new String(signalMessage);
		response.getWriter().append("{\"currentState\":\"" + messageString + "\"}");
		response.getWriter().flush();
		response.getWriter().close();
	}
}
