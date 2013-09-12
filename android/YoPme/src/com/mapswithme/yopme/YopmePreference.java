package com.mapswithme.yopme;

import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;

public class YopmePreference extends PreferenceActivity
                             implements OnSharedPreferenceChangeListener
{
  public final static String LOCATION_UPDATE_DEFAULT = "-1";

  private ListPreference mLocationUpdatePref;

  @SuppressWarnings("deprecation")
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.prefs);
    mLocationUpdatePref = (ListPreference) findPreference(getString(R.string.pref_loc_update));
    updateSummary();
  }

  private void updateSummary()
  {
    final String prefValue = PreferenceManager
      .getDefaultSharedPreferences(this)
      .getString(getString(R.string.pref_loc_update), LOCATION_UPDATE_DEFAULT);
    final String summary = getResources()
      .getStringArray(R.array.update_frequency)[mLocationUpdatePref.findIndexOfValue(prefValue)];

    mLocationUpdatePref.setSummary(summary);
  }

  @SuppressWarnings("deprecation")
  @Override
  protected void onResume()
  {
    super.onResume();
    getPreferenceScreen()
      .getSharedPreferences()
      .registerOnSharedPreferenceChangeListener(this);
  }


  @SuppressWarnings("deprecation")
  @Override
  protected void onPause()
  {
      super.onPause();
      getPreferenceScreen()
        .getSharedPreferences()
        .unregisterOnSharedPreferenceChangeListener(this);
  }

  @Override
  public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key)
  {
    if (key.equals(getString(R.string.pref_loc_update)))
      updateSummary();
  }


}
