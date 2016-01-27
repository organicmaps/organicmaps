package com.mapswithme.maps.editor;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.IntRange;
import android.support.annotation.NonNull;
import android.support.design.widget.TabLayout;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v7.app.AlertDialog;
import android.text.format.DateFormat;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;
import android.widget.TimePicker;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.editor.data.HoursMinutes;

public class HoursMinutesPickerFragment extends BaseMwmDialogFragment
{
  private static final String EXTRA_FROM = "HoursMinutesFrom";
  private static final String EXTRA_TO = "HoursMinutesTo";
  private static final String EXTRA_SELECT_FIRST = "SelectedTab";
  private static final String EXTRA_ID = "Id";

  public static final int TAB_FROM = 0;
  public static final int TAB_TO = 1;

  private HoursMinutes mFrom;
  private HoursMinutes mTo;

  private TimePicker mPicker;
  @IntRange(from = 0, to = 1) private int mSelectedTab;
  private TabLayout mTabs;

  private int mId;

  public interface OnPickListener
  {
    void onHoursMinutesPicked(HoursMinutes from, HoursMinutes to, int id);
  }

  public static void pick(Context context, FragmentManager manager, @NonNull HoursMinutes from, @NonNull HoursMinutes to,
                          @IntRange(from = 0, to = 1) int selectedPosition, int id)
  {
    final Bundle args = new Bundle();
    args.putParcelable(EXTRA_FROM, from);
    args.putParcelable(EXTRA_TO, to);
    args.putInt(EXTRA_SELECT_FIRST, selectedPosition);
    args.putInt(EXTRA_ID, id);
    final HoursMinutesPickerFragment fragment =
        (HoursMinutesPickerFragment) Fragment.instantiate(context, HoursMinutesPickerFragment.class.getName(), args);
    fragment.show(manager, null);
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    readArgs();
    final View root = createView();
    refreshPicker();
    //noinspection ConstantConditions
    mTabs.getTabAt(mSelectedTab).select();

    return new AlertDialog.Builder(getActivity(), R.style.MwmMain_DialogFragment_TimePicker)
               .setView(root)
               .setNegativeButton(android.R.string.cancel, null)
               .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener()
               {
                 @Override
                 public void onClick(DialogInterface dialog, int which)
                 {
                   saveHoursMinutes(mPicker.getCurrentHour(), mPicker.getCurrentMinute());
                   if (getParentFragment() instanceof OnPickListener)
                     ((OnPickListener) getParentFragment()).onHoursMinutesPicked(mFrom, mTo, mId);
                 }
               })
               .setCancelable(true)
               .create();
  }

  private void readArgs()
  {
    final Bundle args = getArguments();
    mFrom = args.getParcelable(EXTRA_FROM);
    mTo = args.getParcelable(EXTRA_TO);
    mSelectedTab = args.getInt(EXTRA_SELECT_FIRST);
    mId = args.getInt(EXTRA_ID);
  }

  private View createView()
  {
    final LayoutInflater inflater = LayoutInflater.from(getActivity());
    @SuppressLint("InflateParams")
    final View root = inflater.inflate(R.layout.fragment_timetable_picker, null);

    mTabs = (TabLayout) root.findViewById(R.id.tabs);
    TextView tabView = (TextView) inflater.inflate(R.layout.tab_timepicker, mTabs, false);
    // TODO @yunik add translations
    tabView.setText("From");
    mTabs.addTab(mTabs.newTab().setCustomView(tabView), true);
    tabView = (TextView) inflater.inflate(R.layout.tab_timepicker, mTabs, false);
    tabView.setText("To");
    mTabs.addTab(mTabs.newTab().setCustomView(tabView), true);
    mTabs.setOnTabSelectedListener(new TabLayout.OnTabSelectedListener()
    {
      @Override
      public void onTabSelected(TabLayout.Tab tab)
      {
        mSelectedTab = tab.getPosition();
        refreshPicker();
      }

      @Override
      public void onTabUnselected(TabLayout.Tab tab)
      {
        saveHoursMinutes(mPicker.getCurrentHour(), mPicker.getCurrentMinute());
      }

      @Override
      public void onTabReselected(TabLayout.Tab tab) {}
    });

    mPicker = (TimePicker) root.findViewById(R.id.picker);
    mPicker.setIs24HourView(DateFormat.is24HourFormat(getActivity()));
    mPicker.setOnTimeChangedListener(new TimePicker.OnTimeChangedListener()
    {
      @Override
      public void onTimeChanged(TimePicker view, int hourOfDay, int minute)
      {
        saveHoursMinutes(hourOfDay, minute);
      }
    });

    return root;
  }

  private void saveHoursMinutes(int hourOfDay, int minute)
  {
    if (mSelectedTab == TAB_FROM)
      mFrom = new HoursMinutes(hourOfDay, minute);
    else
      mTo = new HoursMinutes(hourOfDay, minute);
  }

  private void refreshPicker()
  {
    final HoursMinutes hoursMinutes = mSelectedTab == TAB_FROM ? mFrom : mTo;
    mPicker.setCurrentMinute((int) hoursMinutes.minutes);
    mPicker.setCurrentHour((int) hoursMinutes.hours);
  }
}
