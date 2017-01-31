package com.mapswithme.maps.settings;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.TwoStatePreference;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.search.SearchFragment;
import com.mapswithme.util.Config;
import com.mapswithme.util.log.LoggerFactory;
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
          boolean oldVal = Config.useGoogleServices();
          boolean newVal = ((Boolean) newValue).booleanValue();
          if (oldVal != newVal)
          {
            Config.setUseGoogleService(newVal);
            LocationHelper.INSTANCE.initProvider(false /* forceNative */);
          }
          return true;
        }
      });
    }

    if (!MytargetHelper.isShowcaseSwitchedOnServer())
      getPreferenceScreen().removePreference(findPreference(getString(R.string.pref_showcase_switched_on)));

    pref = findPreference(getString(R.string.pref_enable_logging));
    if (!MwmApplication.prefs().getBoolean(SearchFragment.PREFS_SHOW_ENABLE_LOGGING_SETTING, false))
    {
      getPreferenceScreen().removePreference(pref);
    }
    else
    {
      final boolean isLoggingEnabled = LoggerFactory.INSTANCE.isFileLoggingEnabled();
      ((TwoStatePreference) pref).setChecked(isLoggingEnabled);
      pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
      {
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue)
        {
          boolean oldVal = isLoggingEnabled;
          boolean newVal = (Boolean) newValue;
          if (oldVal != newVal)
          {
            LoggerFactory.INSTANCE.setFileLoggingEnabled((Boolean) newValue);
          }
          return true;
        }
      });
    }
  }
}
