/**
 * File: TrainSignalApi.java
 * Author: Matt Jones
 * Date: 6/1/2014
 * Desc: REST API for communicating with server program that controls the train signal.
 */

package zone.mattjones.trainsignal;

import java.io.IOException;

import jakarta.servlet.ServletConfig;
import jakarta.servlet.annotation.WebServlet;
import jakarta.servlet.http.HttpServlet;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;

import zone.mattjones.common.scheduler.ThreadScheduler;
import zone.mattjones.trainsignal.TrainSignalMessage.LampState;
import zone.mattjones.trainsignal.TrainSignalMessage.SignalColor;

@WebServlet(name="TrainSignalApi", urlPatterns = "/api", loadOnStartup = 1)
public class TrainSignalApi extends HttpServlet {
    private static final long serialVersionUID = 20231112L;

    /** The port for the connection to the train signal to run on. */
    private static final int SERVER_PORT = 19100;
    
    /** The thread handling connections to the train signal. */
    private TrainSignalConnectionHandler mConnectionHandler;

    /** A scheduler for running tasks asynchronously. */
    private ThreadScheduler mScheduler;

    public TrainSignalApi() {}
    
    @Override
    public void init(ServletConfig config) {
        mScheduler = new ThreadScheduler();

        // Set up the server socket listener for the arduino to connect to.
        mConnectionHandler = new TrainSignalConnectionHandler(SERVER_PORT, mScheduler);
    }

    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
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
        
        String serverActionString = request.getParameter("serverAction");
        if (serverActionString != null) {
            switch(serverActionString.charAt(0)) {
            case 'X':
            case 'x':
                mConnectionHandler.killServer();
                break;
            case 'R':
            case 'r':
                mConnectionHandler.resetServer();
                break;
            }
        }
        
        byte[] signalMessage = TrainSignalMessage.generateMessage(color, lamp);
        boolean success = mConnectionHandler.addMessage(signalMessage);
        
        String messageString = new String(signalMessage);
        response.getWriter().append(
                "{\"error\":" + !success + ",\"currentState\":\"" + messageString + "\"}");
        response.getWriter().flush();
        response.getWriter().close();
    }

    @Override
    public void destroy() {
        if (mConnectionHandler != null) {
            try {
                mConnectionHandler.killServer();
            } catch (IOException ex) {
                System.err.println("[error]: Error shutting down signal connection handler! " +
                        ex.getMessage());
            }
        }
        if (mScheduler != null) {
            mScheduler.shutdown();
        }
        super.destroy();
    }
}
