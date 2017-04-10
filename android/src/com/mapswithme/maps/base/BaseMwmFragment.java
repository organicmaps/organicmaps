package com.mapswithme.maps.base;

import android.content.Context;
import android.support.v4.app.Fragment;

import com.mapswithme.util.Utils;

public class BaseMwmFragment extends Fragment
{

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    Utils.detachFragmentIfCoreNotInitialized(context, this);
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
