package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v7.app.AlertDialog;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.base.OnBackPressListener;

public class TimetableContainerFragment extends BaseMwmFragment implements OnBackPressListener
{
  public static final String EXTRA_TIME = "Time";

  private enum Mode
  {
    SIMPLE
    {
      @NonNull
      String getFragmentClassname() { return SimpleTimetableFragment.class.getName(); }
      @StringRes
      int getSwitchButtonLabel() { return R.string.editor_time_advanced; }
    },
    ADVANCED
    {
      @NonNull
      String getFragmentClassname() { return AdvancedTimetableFragment.class.getName(); }
      @StringRes
      int getSwitchButtonLabel() { return R.string.editor_time_simple; }
    };

    @NonNull
    abstract String getFragmentClassname();
    @StringRes
    abstract int getSwitchButtonLabel();
  }

  @NonNull
  private Mode mMode = Mode.ADVANCED;
  @NonNull
  private Fragment[] mFragments = new Fragment[Mode.values().length];
  @Nullable
  private TimetableProvider mTimetableProvider;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mSwitchMode;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_timetable, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    initViews(view);
    mMode = Mode.ADVANCED;
    switchMode();

    final Bundle args = getArguments();
    if (args != null && mTimetableProvider != null && !TextUtils.isEmpty(args.getString(EXTRA_TIME)))
      mTimetableProvider.setTimetables(args.getString(EXTRA_TIME));
  }

  @Nullable
  public String getTimetable()
  {
    if (mTimetableProvider == null)
      return null;

    return mTimetableProvider.getTimetables();
  }

  private void initViews(View root)
  {
    mSwitchMode = root.findViewById(R.id.tv__mode_switch);
    mSwitchMode.setOnClickListener(v -> switchMode());
  }

  @Override
  public boolean onBackPressed()
  {
    return false;
  }

  private void switchMode()
  {
    switch (mMode)
    {
      case SIMPLE:
        setMode(Mode.ADVANCED);
        break;
      case ADVANCED:
        setMode(Mode.SIMPLE);
        break;
    }
  }

  private void setMode(Mode mode)
  {
    final String filledTimetables = mTimetableProvider != null ? mTimetableProvider.getTimetables()
                                                               : OpeningHours.nativeTimetablesToString(OpeningHours.nativeGetDefaultTimetables());
    if (!OpeningHours.nativeIsTimetableStringValid(filledTimetables))
    {
      FragmentActivity activity = getActivity();
      if (activity == null)
        return;

      new AlertDialog.Builder(activity)
        .setMessage(R.string.editor_correct_mistake)
        .setPositiveButton(android.R.string.ok, null)
        .show();
      return;
    }

    mMode = mode;
    mSwitchMode.setText(mMode.getSwitchButtonLabel());

    if (mFragments[mMode.ordinal()] == null)
      mFragments[mMode.ordinal()] = Fragment.instantiate(getActivity(), mMode.getFragmentClassname());
    Fragment fragment = mFragments[mMode.ordinal()];
    getChildFragmentManager().beginTransaction().replace(R.id.fragment_container, fragment).commit();

    mTimetableProvider = (TimetableProvider) fragment;
    mTimetableProvider.setTimetables(filledTimetables);
  }
}
