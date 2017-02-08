package com.mapswithme.maps.settings;

import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.TwoStatePreference;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.search.SearchFragment;
import com.mapswithme.util.Config;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.concurrency.UiThread;
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
            LocationHelper.INSTANCE.restart();
          }
          return true;
        }
      });
    }

    if (!MytargetHelper.isShowcaseSwitchedOnServer())
      getPreferenceScreen().removePreference(findPreference(getString(R.string.pref_showcase_switched_on)));

    pref = findPreference(getString(R.string.pref_enable_logging));
    if (!MwmApplication.prefs().getBoolean(SearchFragment.PREFS_SHOW_ENABLE_LOGGING_SETTING,
                                           BuildConfig.BUILD_TYPE.equals("beta")))
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
            LoggerFactory.INSTANCE.setFileLoggingEnabled(newVal);
          }
          return true;
        }
      });
    }

    int curValue = Config.getUseMobileDataSettings();
    final ListPreference mobilePref = (ListPreference)findPreference(
        getString(R.string.pref_use_mobile_data));
    if (curValue != NetworkPolicy.NOT_TODAY && curValue != NetworkPolicy.TODAY)
    {
      mobilePref.setValue(String.valueOf(curValue));
      mobilePref.setSummary(mobilePref.getEntry());
    }
    else
    {
      mobilePref.setSummary(getString(R.string.mobile_data_description));
    }
    mobilePref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        String valueStr = (String)newValue;
        switch (Integer.parseInt(valueStr))
        {
          case NetworkPolicy.ASK:
            Config.setUseMobileDataSettings(NetworkPolicy.ASK);
            break;
          case NetworkPolicy.ALWAYS:
            Config.setUseMobileDataSettings(NetworkPolicy.ALWAYS);
            break;
          case NetworkPolicy.NEVER:
            Config.setUseMobileDataSettings(NetworkPolicy.NEVER);
            break;
          default:
            throw new AssertionError("Wrong NetworkPolicy type!");
        }

        UiThread.runLater(new Runnable()
        {
          @Override
          public void run()
          {
            mobilePref.setSummary(mobilePref.getEntry());
          }
        });

        return true;
      }
    });
  }
}
