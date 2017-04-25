package com.mapswithme.maps.base;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.annotation.StyleRes;
import android.support.v4.app.DialogFragment;

import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;

public class BaseMwmDialogFragment extends DialogFragment
{
  protected final @StyleRes int getFullscreenTheme()
  {
    return (ThemeUtils.isNightTheme() ? R.style.MwmTheme_DialogFragment_Fullscreen_Night
                                      : R.style.MwmTheme_DialogFragment_Fullscreen);
  }

  protected int getStyle()
  {
    return STYLE_NORMAL;
  }

  protected @StyleRes int getCustomTheme()
  {
    return 0;
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    int style = getStyle();
    int theme = getCustomTheme();
    if (style != STYLE_NORMAL || theme != 0)
      //noinspection WrongConstant
      setStyle(style, theme);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    org.alohalytics.Statistics.logEvent("$onResume", getClass().getSimpleName()
        + ":" + com.mapswithme.util.UiUtils.deviceOrientationAsString(getActivity()));
  }

  @Override
  public void onPause()
  {
    super.onPause();
    org.alohalytics.Statistics.logEvent("$onPause", getClass().getSimpleName());
  }
}
