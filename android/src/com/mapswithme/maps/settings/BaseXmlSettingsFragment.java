package com.mapswithme.maps.settings;

import android.os.Bundle;
import android.preference.PreferenceFragment;
import android.support.annotation.XmlRes;
import com.mapswithme.util.UiUtils;

abstract class BaseXmlSettingsFragment extends PreferenceFragment
{
  protected abstract @XmlRes int getXmlResources();

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(getXmlResources());
  }

  @Override
  public void onResume()
  {
    super.onResume();
    org.alohalytics.Statistics.logEvent("$onResume", getClass().getSimpleName() + ":" +
                                                     UiUtils.deviceOrientationAsString(getActivity()));
  }

  @Override
  public void onPause()
  {
    super.onPause();
    org.alohalytics.Statistics.logEvent("$onPause", getClass().getSimpleName() + ":" +
                                                    UiUtils.deviceOrientationAsString(getActivity()));
  }
}
