package com.mapswithme.maps.base;

import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.ListFragment;
import android.support.v7.widget.Toolbar;
import android.view.View;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

@Deprecated
public abstract class BaseMwmListFragment extends ListFragment
{
  private Toolbar mToolbar;
  @Nullable
  private View mView;
  @Nullable
  private Bundle mSavedInstanceState;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    if (MwmApplication.get().isPlatformInitialized() && Utils.isWriteExternalGranted(getActivity()))
      safeOnCreate(savedInstanceState);
  }

  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mView = view;
    mSavedInstanceState = savedInstanceState;
    if (MwmApplication.get().isPlatformInitialized() && Utils.isWriteExternalGranted(getActivity()))
      safeOnViewCreated(view, savedInstanceState);
  }

  @CallSuper
  protected void safeOnViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    mToolbar = (Toolbar) view.findViewById(R.id.toolbar);
    if (mToolbar != null)
    {
      UiUtils.showHomeUpButton(mToolbar);
      mToolbar.setNavigationOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          Utils.navigateToParent(getActivity());
        }
      });
    }
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
    {
      safeOnCreate(mSavedInstanceState);
      if (mView != null)
      {
        safeOnViewCreated(mView, mSavedInstanceState);
        safeOnResume();
      }
    }
  }

  public Toolbar getToolbar()
  {
    return mToolbar;
  }

  @Override
  public void onResume()
  {
    super.onResume();
    org.alohalytics.Statistics.logEvent("$onResume", getClass().getSimpleName() + ":" +
                                                     UiUtils.deviceOrientationAsString(getActivity()));

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
}
