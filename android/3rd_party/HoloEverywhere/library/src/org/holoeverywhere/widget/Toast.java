
package org.holoeverywhere.widget;

import org.holoeverywhere.R;

import android.content.Context;
import android.util.Log;
import android.view.Gravity;

public class Toast extends android.widget.Toast {
    public static final int LENGTH_LONG = android.widget.Toast.LENGTH_LONG;
    public static final int LENGTH_SHORT = android.widget.Toast.LENGTH_SHORT;
    private static final String TAG = "Toast";

    public static Toast makeText(Context context, CharSequence s, int duration) {
        Toast toast = new Toast(context);
        toast.setDuration(duration);
        TextView view = new TextView(context);
        view.setText(s);
        view.setTextColor(0xFFFFFFFF);
        view.setGravity(Gravity.CENTER);
        view.setBackgroundResource(R.drawable.toast_frame);
        toast.setView(view);
        return toast;
    }

    public static Toast makeText(Context context, int resId, int duration) {
        return Toast.makeText(context, context.getResources().getString(resId),
                duration);
    }

    public Toast(Context context) {
        super(context);
    }

    @Override
    public void setText(CharSequence s) {
        if (getView() == null) {
            return;
        }
        try {
            ((TextView) getView()).setText(s);
        } catch (ClassCastException e) {
            Log.e(Toast.TAG, "This Toast was not created with Toast.makeText",
                    e);
        }
    }
}
