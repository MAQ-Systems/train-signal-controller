/**
 * File: MainActivity.java
 * Author: Matt Jones
 * Date: 7.14.2018
 * Desc: Main activity for app that controls the train signal.
 */

package zone.mattjones.localtrainctc;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.View;

import java.net.URI;
import java.util.ArrayList;

public class MainActivity extends Activity implements View.OnClickListener{
    /** The preference name that is used to store the API URL. */
    private static final String API_URL_PREF_STRING = "zone.mattjones.localtrainctc.apiurl";

    /** The amount of time to wait to re-enabled the buttons after a signal change. */
    private static final int BUTTON_ENABLE_DELAY = 5000;

    /** The possible colors that the signal can be. */
    private enum SignalColor {
        RED,
        YELLOW,
        GREEN
    }

    /** Possible states that the lamp can be in. */
    private enum LampState {
        ON,
        BLINK,
        OFF
    }

    /** The URL location of the train signal api. */
    private String mApiUrl;

    /** Handles to the signal buttons in the app. */
    private ArrayList<SignalColorButton> mSignalButtons;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Get handles to all the signal buttons.
        mSignalButtons = new ArrayList<>();
        mSignalButtons.add((SignalColorButton) findViewById(R.id.button_green));
        mSignalButtons.add((SignalColorButton) findViewById(R.id.button_yellow));
        mSignalButtons.add((SignalColorButton) findViewById(R.id.button_yellow_blink));
        mSignalButtons.add((SignalColorButton) findViewById(R.id.button_red));
        mSignalButtons.add((SignalColorButton) findViewById(R.id.button_red_blink));

        for (SignalColorButton button : mSignalButtons) {
            button.setOnClickListener(this);
        }

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        if (prefs.contains(API_URL_PREF_STRING)) {

        }

        enableButtons();
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.button_green:
                break;
            case R.id.button_yellow:
                break;
            case R.id.button_yellow_blink:
                break;
            case R.id.button_red:
                break;
            case R.id.button_red_blink:
                break;
            default:
                // If the ID is not one of the buttons, do nothing.
                return;
        }

        // Block the user from spamming messages.
        disableButtons();
        // Show the enabled color for the button that was clicked. This is used for visual feedback
        // to the user.
        ((SignalColorButton) view).showEnabledColor();

        view.postDelayed(new Runnable() {
            @Override
            public void run() {
                enableButtons();
            }
        }, BUTTON_ENABLE_DELAY);
    }

    /**
     * Disable all the signal color buttons.
     */
    private void disableButtons() {
        for (SignalColorButton button : mSignalButtons) button.setEnabled(false);
    }

    /**
     * Enable all the signal color buttons.
     */
    private void enableButtons() {
        for (SignalColorButton button : mSignalButtons) button.setEnabled(true);
    }

    private URI produceApiUri(SignalColor color, LampState state) {
        return null;
    }
}
