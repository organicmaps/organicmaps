package com.mapswithme.maps.base;

import android.os.Bundle;
import android.support.v4.app.Fragment;

public class MWMFragment extends Fragment
{
  @Override
  public void onCreate(Bundle arg0)
  {
    super.onCreate(arg0);
  }

  @Override
  public void onStart()
  {
    super.onStart();
  }

  @Override
  public void onStop()
  {
    super.onStop();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    org.alohalytics.Statistics.logEvent("$onResume", this.getClass().getSimpleName());
  }

  @Override
  public void onPause()
  {
    super.onPause();
    org.alohalytics.Statistics.logEvent("$onPause", this.getClass().getSimpleName());
  }
}
