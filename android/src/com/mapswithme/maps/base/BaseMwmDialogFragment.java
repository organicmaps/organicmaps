package com.mapswithme.maps.base;

import android.app.Application;
import android.content.Context;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import androidx.fragment.app.DialogFragment;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;

public class BaseMwmDialogFragment extends DialogFragment
{
  @StyleRes
  protected final int getFullscreenTheme()
  {
    return ThemeUtils.isNightTheme(requireContext()) ? getFullscreenDarkTheme() : getFullscreenLightTheme();
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

  @StyleRes
  protected int getFullscreenLightTheme()
  {
    return R.style.MwmTheme_DialogFragment_Fullscreen;
  }

  @StyleRes
  protected int getFullscreenDarkTheme()
  {
    return R.style.MwmTheme_DialogFragment_Fullscreen_Night;
  }

  @NonNull
  protected Application getAppContextOrThrow()
  {
    Context context = getContext();
    if (context == null)
      throw new IllegalStateException("Before call this method make sure that the context exists");
    return (Application) context.getApplicationContext();
  }

  @NonNull
  protected View getViewOrThrow()
  {
    View view = getView();
    if (view == null)
      throw new IllegalStateException("Before call this method make sure that the view exists");
    return view;
  }

  @NonNull
  protected Bundle getArgumentsOrThrow()
  {
    Bundle args = getArguments();
    if (args == null)
      throw new AssertionError("Arguments must be non-null!");
    return args;
  }
}
