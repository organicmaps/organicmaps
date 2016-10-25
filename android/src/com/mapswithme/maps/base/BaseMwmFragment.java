package com.mapswithme.maps.base;

import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.View;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.Utils;

import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

public class BaseMwmFragment extends Fragment
{
  @Nullable
  private View mView;
  @Nullable
  private Bundle mSavedInstanceState;

  @Override
  public void onResume()
  {
    super.onResume();
    org.alohalytics.Statistics.logEvent("$onResume", this.getClass().getSimpleName()
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
    org.alohalytics.Statistics.logEvent("$onPause", this.getClass().getSimpleName());
  }

  @CallSuper
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
