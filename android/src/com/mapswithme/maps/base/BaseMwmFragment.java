package com.mapswithme.maps.base;

import android.content.Context;
import android.support.v4.app.Fragment;
import android.support.v7.app.AppCompatActivity;

import com.mapswithme.maps.MwmApplication;

public class BaseMwmFragment extends Fragment
{

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    if (context instanceof AppCompatActivity && !MwmApplication.get().isFrameworkInitialized())
    {
      ((AppCompatActivity)context).getSupportFragmentManager()
                                  .beginTransaction()
                                  .detach(this)
                                  .commit();
    }
  }

  @Override
  public void onResume()
  {
    super.onResume();
    org.alohalytics.Statistics.logEvent("$onResume", this.getClass().getSimpleName()
        + ":" + com.mapswithme.util.UiUtils.deviceOrientationAsString(getActivity()));
  }

  @Override
  public void onPause()
  {
    super.onPause();
    org.alohalytics.Statistics.logEvent("$onPause", this.getClass().getSimpleName());
  }

  public BaseMwmFragmentActivity getMwmActivity()
  {
    return (BaseMwmFragmentActivity) getActivity();
  }
}
