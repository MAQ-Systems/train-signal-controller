/**
 * File: NetworkTask.java
 * Author: Matt Jones
 * Date: 7.15.2018
 * Desc: A class for easily handling a network request on a separate thread. To use, call
 *       NetworkTask#execute(URL).
 */

package zone.mattjones.localtrainctc;

import android.os.AsyncTask;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

public class NetworkTask extends AsyncTask<URL, Integer, String> {
    /** This class' tag for logging. */
    public static final String TAG = "NetworkTask";

    /** Interface for a notification that the network task has finished. */
    public interface NetworkTaskFinishedListener {
        /**
         * A notification that the network task has finished.
         * @param result The string result from the network request.
         */
        void onNetworkTaskFinished(String result);
    }

    /** The listener for this task. */
    private NetworkTaskFinishedListener mListener;

    public NetworkTask(NetworkTaskFinishedListener listener) {
        mListener = listener;
    }

    @Override
    protected String doInBackground(URL... urls) {
        URL requestUrl = urls[0];
        HttpURLConnection connection = null;
        String result = "";
        try {
            connection = (HttpURLConnection) requestUrl.openConnection();
            connection.setRequestMethod("GET");
            connection.connect();

            // Throw an exception if the server did not respond with a success code (200 - 299).
            if (connection.getResponseCode() < 200 || connection.getResponseCode() > 299) {
                throw new IOException("Server returned code " + connection.getResponseCode() + "!");
            }

            InputStream is = connection.getInputStream();
            byte[] buffer = new byte[128];
            int readSize = 0;
            while ((readSize = is.read(buffer)) > 0) {
                result += new String(buffer, 0, readSize);
            }
        } catch (IOException e) {
            result = "{\"error\": true, \"message\": \"" + e.getMessage() + "\"}";
            e.printStackTrace();
        } catch (ClassCastException e) {
            result = "{\"error\": true,"
                    + "\"message\": \"Opened connection was not an HttpURLConnection!\"}";
            e.printStackTrace();
        }

        return result;
    }

    @Override
    public void onPostExecute(String result) {
        if (mListener != null) mListener.onNetworkTaskFinished(result);
    }
}
