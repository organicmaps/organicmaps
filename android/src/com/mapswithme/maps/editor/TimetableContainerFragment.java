package com.mapswithme.maps.editor;

import android.app.Activity;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.util.UiUtils;

public class TimetableContainerFragment extends BaseMwmFragment implements OnBackPressListener,
                                                                           TimetableChangedListener
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

      void setTimetableChangedListener(@NonNull Fragment fragment,
                                       @NonNull TimetableChangedListener listener)
      {
        ((AdvancedTimetableFragment) fragment).setTimetableChangedListener(listener);
      }
    };

    @NonNull
    abstract String getFragmentClassname();
    @StringRes
    abstract int getSwitchButtonLabel();
    void setTimetableChangedListener(@NonNull Fragment fragment,
                                     @NonNull TimetableChangedListener listener)
    {
    }
    @NonNull
    static TimetableProvider getTimetableProvider(@NonNull Fragment fragment)
    {
      return (TimetableProvider) fragment;
    }
  }

  @NonNull
  private Mode mMode = Mode.ADVANCED;
  @NonNull
  private final Fragment[] mFragments = new Fragment[Mode.values().length];
  @Nullable
  private TimetableProvider mTimetableProvider;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mSwitchMode;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mBottomBar;

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

    Activity activity = requireActivity();
    if (activity != null)
      activity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);

    initViews(view);

    final Bundle args = getArguments();
    String time = null;
    if (args != null)
      time = args.getString(EXTRA_TIME);

    // Show Simple fragment when opening hours can be represented by UI.
    if (TextUtils.isEmpty(time) || OpeningHours.nativeTimetablesFromString(time) != null)
      setMode(Mode.SIMPLE, time);
    else
      setMode(Mode.ADVANCED, time);
  }

  @Nullable
  public String getTimetable()
  {
    if (mTimetableProvider == null)
      return null;

    return mTimetableProvider.getTimetables();
  }

  @Override
  public void onTimetableChanged(@Nullable String timetable)
  {
    boolean isValidTimetable = TextUtils.isEmpty(timetable)
                               || OpeningHours.nativeTimetablesFromString(timetable) != null;
    UiUtils.showIf(isValidTimetable, mSwitchMode);
    UiUtils.showIf(isValidTimetable, mBottomBar);
  }

  @Override
  public boolean onBackPressed()
  {
    return false;
  }

  private void initViews(@NonNull View root)
  {
    mSwitchMode = root.findViewById(R.id.tv__mode_switch);
    mSwitchMode.setOnClickListener(v -> switchMode());
    mBottomBar = root.findViewById(R.id.v__bottom_bar);
  }

  private void switchMode()
  {
    final String filledTimetables = mTimetableProvider != null ? mTimetableProvider.getTimetables()
                                                               : null;

    if (filledTimetables != null && !OpeningHours.nativeIsTimetableStringValid(filledTimetables))
    {
      FragmentActivity activity = requireActivity();
      if (activity == null)
        return;

      new AlertDialog.Builder(activity)
        .setMessage(R.string.editor_correct_mistake)
        .setPositiveButton(android.R.string.ok, null)
        .show();
      return;
    }

    switch (mMode)
    {
      case SIMPLE:
        setMode(Mode.ADVANCED, filledTimetables);
        break;
      case ADVANCED:
        setMode(Mode.SIMPLE, filledTimetables);
        break;
    }
  }

  private void setMode(@NonNull Mode mode, @Nullable String timetables)
  {
    mMode = mode;
    mSwitchMode.setText(mMode.getSwitchButtonLabel());

    if (mFragments[mMode.ordinal()] == null)
      mFragments[mMode.ordinal()] = Fragment.instantiate(requireActivity(), mMode.getFragmentClassname());
    Fragment fragment = mFragments[mMode.ordinal()];
    getChildFragmentManager().beginTransaction().replace(R.id.fragment_container, fragment).commit();
    mMode.setTimetableChangedListener(fragment, this);
    mTimetableProvider = Mode.getTimetableProvider(fragment);
    mTimetableProvider.setTimetables(timetables);
  }
}
