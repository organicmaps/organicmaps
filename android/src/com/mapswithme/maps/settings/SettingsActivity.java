package com.mapswithme.maps.settings;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;

import com.mapswithme.maps.R;


public class SettingsActivity extends PreferenceActivity
{
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    addPreferencesFromResource(R.layout.preferences);

    final Activity parent = this;

    Preference pref = findPreference("StorageActivity");
    pref.setOnPreferenceClickListener(new OnPreferenceClickListener()
    {
      @Override
      public boolean onPreferenceClick(Preference preference)
      {
        parent.startActivity(new Intent(parent, StoragePathActivity.class));
        return true;
      }
    });

    pref = findPreference("MeasurementUnits");
    pref.setOnPreferenceClickListener(new OnPreferenceClickListener()
    {
      @Override
      public boolean onPreferenceClick(Preference preference)
      {
        UnitLocale.showUnitsSelectDlg(parent);
        return true;
      }
    });
  }
}
