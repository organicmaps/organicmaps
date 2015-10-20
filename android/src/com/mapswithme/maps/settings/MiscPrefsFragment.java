package com.mapswithme.maps.settings;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.SwitchPreference;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.Config;
import com.mapswithme.util.statistics.Statistics;

public class MiscPrefsFragment extends PreferenceFragment
{
  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.prefs_misc);

    Preference pref = findPreference(getString(R.string.pref_send_statistics));
    ((SwitchPreference)pref).setChecked(Config.isStatisticsEnabled());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        Statistics.INSTANCE.setStatEnabled((Boolean) newValue);
        return true;
      }
    });

    if (GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(MwmApplication.get()) != ConnectionResult.SUCCESS)
      getPreferenceScreen().removePreference(findPreference(getString(R.string.pref_play_services)));
  }
}
