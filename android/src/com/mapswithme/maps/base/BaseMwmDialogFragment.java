package com.mapswithme.maps.base;

import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StyleRes;
import android.support.v4.app.DialogFragment;
import android.view.View;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.Utils;

import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

public class BaseMwmDialogFragment extends DialogFragment
{
  @Nullable
  private View mView;
  @Nullable
  private Bundle mSavedInstanceState;

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
    mSavedInstanceState = savedInstanceState;

    int style = getStyle();
    int theme = getCustomTheme();
    if (style != STYLE_NORMAL || theme != 0)
      //noinspection WrongConstant
      setStyle(style, theme);

    if (MwmApplication.get().isPlatformInitialized() && Utils.isWriteExternalGranted(getActivity()))
      safeOnCreate(savedInstanceState);
  }

  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
  }

  @Override
  public void onResume()
  {
    super.onResume();
    org.alohalytics.Statistics.logEvent("$onResume", getClass().getSimpleName()
        + ":" + com.mapswithme.util.UiUtils.deviceOrientationAsString(getActivity()));

    if (MwmApplication.get().isPlatformInitialized() && Utils.isWriteExternalGranted(getActivity()))
      safeOnResume();
  }

  protected void safeOnResume()
  {
  }

  @Override
  public void onPause()
  {
    super.onPause();
    org.alohalytics.Statistics.logEvent("$onPause", getClass().getSimpleName());
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mView = view;
    mSavedInstanceState = savedInstanceState;
    if (MwmApplication.get().isPlatformInitialized() && Utils.isWriteExternalGranted(getActivity()))
      safeOnViewCreated(view, savedInstanceState);
  }

  protected void safeOnViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
  }

  @CallSuper
  @Override
  public void onDestroyView()
  {
    mView = null;
    mSavedInstanceState = null;
    super.onDestroyView();
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults)
  {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    if (grantResults.length == 0)
      return;

    boolean isWriteGranted = false;
    for (int i = 0; i < permissions.length; ++i)
    {
      int result = grantResults[i];
      String permission = permissions[i];
      if (permission.equals(WRITE_EXTERNAL_STORAGE) && result == PERMISSION_GRANTED)
        isWriteGranted = true;
    }

    if (isWriteGranted)
      safeOnCreate(mSavedInstanceState);

    if (isWriteGranted && mView != null)
    {
      safeOnViewCreated(mView, mSavedInstanceState);
      safeOnResume();
    }
  }

  public BaseMwmFragmentActivity getMwmActivity()
  {
    return (BaseMwmFragmentActivity) getActivity();
  }
}
