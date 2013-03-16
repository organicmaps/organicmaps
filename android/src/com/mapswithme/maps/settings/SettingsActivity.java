package com.mapswithme.maps.settings;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;

import com.mapswithme.maps.R;
import com.mapswithme.util.Statistics;

public class SettingsActivity extends PreferenceActivity
{
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
        parent.startActivity(new Intent(parent, StoragePathActivity.class));
        return true;
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
