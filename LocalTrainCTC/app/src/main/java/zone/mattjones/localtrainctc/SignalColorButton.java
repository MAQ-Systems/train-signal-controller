/**
 * File: SignalColorButton.java
 * Author: Matt Jones
 * Date: 7.14.2018
 * Desc: A button that, when disabled, changes to grey.
 */

package zone.mattjones.localtrainctc;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.util.AttributeSet;
import android.widget.ImageButton;

public class SignalColorButton extends ImageButton {
    /** The drawable used for the background of the button. */
    private ColorDrawable mBackground;

    /** The color of the button when enabled. */
    private int mEnabledColor;

    /** The color of the button when disabled. */
    private int mDisabledColor;

    /** Default constructor. */
    public SignalColorButton(Context context, AttributeSet atts) {
        super(context, atts);

        TypedArray a = context.getTheme().obtainStyledAttributes(
                atts, R.styleable.SignalColorButton, 0, 0);
        mEnabledColor = a.getColor(R.styleable.SignalColorButton_enabledColor, Color.WHITE);
        mDisabledColor = a.getColor(R.styleable.SignalColorButton_disabledColor, Color.WHITE);

        mBackground = new ColorDrawable(mEnabledColor);
        setBackground(mBackground);

        a.recycle();
    }

    @Override
    public void setEnabled(boolean enabled) {
        super.setEnabled(enabled);
        mBackground.setColor(enabled ? mEnabledColor : mDisabledColor);
    }

    /**
     * Show the button's enabled color without actually enabling it. This is used to indicate that
     * it was the last button clicked.
     */
    public void showEnabledColor() {
        mBackground.setColor(mEnabledColor);
    }
}
