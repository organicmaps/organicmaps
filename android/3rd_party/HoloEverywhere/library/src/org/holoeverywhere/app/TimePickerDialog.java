
package org.holoeverywhere.app;

import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.R;
import org.holoeverywhere.widget.TimePicker;

import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;
import android.view.View;

public class TimePickerDialog extends AlertDialog implements OnClickListener,
        TimePicker.OnTimeChangedListener {
    public interface OnTimeSetListener {
        public void onTimeSet(TimePicker view, int hourOfDay, int minute);
    }

    private static final String HOUR = "hour";
    private static final String IS_24_HOUR = "is24hour";
    private static final String MINUTE = "minute";

    private final OnTimeSetListener mCallback;
    int mInitialHourOfDay, mInitialMinute;

    boolean mIs24HourView;
    private final TimePicker mTimePicker;

    public TimePickerDialog(Context context, int theme,
            OnTimeSetListener callBack, int hourOfDay, int minute,
            boolean is24HourView) {
        super(context, theme);
        mCallback = callBack;
        mInitialHourOfDay = hourOfDay;
        mInitialMinute = minute;
        mIs24HourView = is24HourView;
        setIcon(0);
        setTitle(R.string.time_picker_dialog_title);
        Context themeContext = getContext();
        setButton(DialogInterface.BUTTON_POSITIVE,
                themeContext.getText(R.string.date_time_set), this);
        setButton(DialogInterface.BUTTON_NEGATIVE,
                themeContext.getText(android.R.string.cancel),
                (OnClickListener) null);
        View view = LayoutInflater.inflate(themeContext,
                R.layout.time_picker_dialog);
        setView(view);
        mTimePicker = (TimePicker) view.findViewById(R.id.timePicker);
        mTimePicker.setIs24HourView(mIs24HourView);
        mTimePicker.setCurrentHour(mInitialHourOfDay);
        mTimePicker.setCurrentMinute(mInitialMinute);
        mTimePicker.setOnTimeChangedListener(this);
    }

    public TimePickerDialog(Context context, OnTimeSetListener callBack,
            int hourOfDay, int minute, boolean is24HourView) {
        this(context, 0, callBack, hourOfDay, minute, is24HourView);
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        if (mCallback != null) {
            mTimePicker.clearFocus();
            mCallback.onTimeSet(mTimePicker, mTimePicker.getCurrentHour(),
                    mTimePicker.getCurrentMinute());
        }
    }

    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        int hour = savedInstanceState.getInt(TimePickerDialog.HOUR);
        int minute = savedInstanceState.getInt(TimePickerDialog.MINUTE);
        mTimePicker.setIs24HourView(savedInstanceState
                .getBoolean(TimePickerDialog.IS_24_HOUR));
        mTimePicker.setCurrentHour(hour);
        mTimePicker.setCurrentMinute(minute);
    }

    @Override
    public Bundle onSaveInstanceState() {
        Bundle state = super.onSaveInstanceState();
        state.putInt(TimePickerDialog.HOUR, mTimePicker.getCurrentHour());
        state.putInt(TimePickerDialog.MINUTE, mTimePicker.getCurrentMinute());
        state.putBoolean(TimePickerDialog.IS_24_HOUR,
                mTimePicker.is24HourView());
        return state;
    }

    @Override
    public void onTimeChanged(TimePicker view, int hourOfDay, int minute) {
    }

    public void updateTime(int hourOfDay, int minutOfHour) {
        mTimePicker.setCurrentHour(hourOfDay);
        mTimePicker.setCurrentMinute(minutOfHour);
    }
}
