package com.mapswithme.maps.widget.placepage;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.editor.OpeningHours;
import com.mapswithme.maps.editor.data.Timetable;
import com.mapswithme.util.Utils;

public class TimetableFragment extends BaseMwmToolbarFragment
                            implements View.OnClickListener,
                                       OnBackPressListener
{
  interface TimetableProvider
  {
    Timetable[] getTimetables();
  }

  public static final String EXTRA_TIME = "Time";

  private boolean mIsAdvancedMode;

  private TextView mSwitchMode;

  // TODO @yunikkk simplify, extract interface
  private SimpleTimetableFragment mSimpleModeFragment;
  private AdvancedTimetableFragment mAdvancedModeFragment;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_timetable, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    initViews(view);
    simpleMode();

    final Bundle args = getArguments();
    if (args != null && args.containsKey(EXTRA_TIME))
      mSimpleModeFragment.setTimetables(OpeningHours.nativeTimetablesFromString(args.getString(EXTRA_TIME)));
  }

  private void initViews(View root)
  {
    mToolbarController.setTitle(R.string.editor_time_title);
    mToolbarController.findViewById(R.id.iv__submit).setOnClickListener(this);
    mSwitchMode = (TextView) root.findViewById(R.id.tv__mode_switch);
    mSwitchMode.setOnClickListener(this);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.tv__mode_switch:
      switchMode();
      break;
    case R.id.iv__submit:
      saveTimetable();
    }
  }

  @Override
  public boolean onBackPressed()
  {
    return false;
  }

  private void switchMode()
  {
    if (mIsAdvancedMode)
      simpleMode();
    else
      advancedMode();
  }

  private void simpleMode()
  {
    mIsAdvancedMode = false;
    mSwitchMode.setText(R.string.editor_time_advanced);
    final Timetable[] filledTimetables = getFilledTimetables(mAdvancedModeFragment, mAdvancedModeFragment);
    mSimpleModeFragment = (SimpleTimetableFragment) attachFragment(mSimpleModeFragment, SimpleTimetableFragment.class.getName());
    mSimpleModeFragment.setTimetables(filledTimetables);
  }

  private void advancedMode()
  {
    mIsAdvancedMode = true;
    mSwitchMode.setText(R.string.editor_time_simple);
    final Timetable[] filledTimetables = getFilledTimetables(mSimpleModeFragment, mSimpleModeFragment);
    mAdvancedModeFragment = (AdvancedTimetableFragment) attachFragment(mAdvancedModeFragment, AdvancedTimetableFragment.class.getName());
    mAdvancedModeFragment.setTimetables(filledTimetables);
  }

  private boolean hasFilledTimetables(Fragment fragment)
  {
    return fragment != null && fragment.isAdded();
  }

  private Timetable[] getFilledTimetables(Fragment fragment, TimetableProvider provider)
  {
    if (!hasFilledTimetables(fragment))
      return null;

    final Timetable[] timetables = provider.getTimetables();
    if (timetables == null)
    {
      // FIXME @yunikkk add correct text
      new AlertDialog.Builder(getActivity())
          .setMessage("Invalid formatted timetable!")
          .create();
      return null;
    }

    return timetables;
  }

  private Fragment attachFragment(Fragment current, String className)
  {
    Fragment fragment = current == null ? Fragment.instantiate(getActivity(), className)
                                        : current;
    getChildFragmentManager().beginTransaction().replace(R.id.fragment_container, fragment).commit();
    return fragment;
  }

  private void saveTimetable()
  {
    if (mIsAdvancedMode)
      mAdvancedModeFragment.getTimetables();
    else
      mSimpleModeFragment.getTimetables();

    final Timetable[] timetables = mIsAdvancedMode ? mAdvancedModeFragment.getTimetables() : mSimpleModeFragment.getTimetables();
    // TODO @yunikkk or @deathbaba save timetables to the core

    Utils.navigateToParent(getActivity());
  }
}
