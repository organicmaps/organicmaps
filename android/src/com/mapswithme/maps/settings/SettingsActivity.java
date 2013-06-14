package com.mapswithme.maps.settings;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;

import com.mapswithme.maps.R;
import com.mapswithme.util.Statistics;
import com.mapswithme.util.Utils;

public class SettingsActivity extends PreferenceActivity
{
  private native boolean isDownloadingActive();

  @SuppressWarnings("deprecation")
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    addPreferencesFromResource(R.xml.preferences);

    final Activity parent = this;

    Preference pref = findPreference("StorageActivity");
    pref.setOnPreferenceClickListener(new OnPreferenceClickListener()
    {
      @Override
      public boolean onPreferenceClick(Preference preference)
      {
        if (isDownloadingActive())
        {
          new AlertDialog.Builder(parent)
          .setTitle(parent.getString(R.string.cant_change_this_setting))
          .setMessage(parent.getString(R.string.downloading_is_active))
          .setPositiveButton(parent.getString(R.string.ok), new DialogInterface.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dlg, int which)
            {
              dlg.dismiss();
            }
          })
          .create()
          .show();

          return false;
        }
        else
        {
          parent.startActivity(new Intent(parent, StoragePathActivity.class));
          return true;
        }
      }
    });

    ListPreference lPref = (ListPreference) findPreference("MeasurementUnits");
    lPref.setValue(String.valueOf(UnitLocale.getUnits()));
    lPref.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        UnitLocale.setUnits(Integer.parseInt((String) newValue));
        return true;
      }
    });


    //Statistics preference
    if (Utils.apiEqualOrGreaterThan(9))
    {
      CheckBoxPreference allowStatsPreference = (CheckBoxPreference) findPreference("allow_stats");
      allowStatsPreference.setChecked(Statistics.INSTANCE.isStatisticsEnabled(this));
      allowStatsPreference.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
      {
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue)
        {
          Statistics.INSTANCE.setStatEnabled(getApplicationContext(), (Boolean)newValue);
          return true;
        }
      });
    }
    // no else
  }

  @Override
  protected void onStart()
  {
    super.onStart();

    Statistics.INSTANCE.startActivity(this);
  }

  @Override
  protected void onStop()
  {
    super.onStop();

    Statistics.INSTANCE.stopActivity(this);
  }
}
