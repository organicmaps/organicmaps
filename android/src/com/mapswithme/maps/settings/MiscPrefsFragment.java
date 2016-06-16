package com.mapswithme.maps.settings;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.TwoStatePreference;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.Config;
import com.mapswithme.util.statistics.MytargetHelper;
import com.mapswithme.util.statistics.Statistics;

public class MiscPrefsFragment extends BaseXmlSettingsFragment
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_misc;
  }

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    Preference pref = findPreference(getString(R.string.pref_send_statistics));
    ((TwoStatePreference)pref).setChecked(Config.isStatisticsEnabled());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        Statistics.INSTANCE.setStatEnabled((Boolean) newValue);
        return true;
      }
    });

    pref = findPreference(getString(R.string.pref_play_services));

    if (GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(MwmApplication.get()) != ConnectionResult.SUCCESS)
      getPreferenceScreen().removePreference(pref);
    else
    {
      ((TwoStatePreference) pref).setChecked(Config.useGoogleServices());
      pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
      {
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue)
        {
          Config.setUseGoogleService((Boolean) newValue);
          // TODO (trashkalmar): Reinitialize location provider
          return true;
        }
      });
    }

    if (!MytargetHelper.isShowcaseSwitchedOnServer())
      getPreferenceScreen().removePreference(findPreference(getString(R.string.pref_showcase_switched_on)));
  }
}
