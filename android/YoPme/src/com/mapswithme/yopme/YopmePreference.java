package com.mapswithme.yopme;

import android.app.AlertDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.text.SpannableString;
import android.text.util.Linkify;
import android.widget.Toast;

public class YopmePreference extends PreferenceActivity
                             implements OnSharedPreferenceChangeListener
{
  public final static String LOCATION_UPDATE_DEFAULT = "15";

  private ListPreference mLocationUpdatePref;

  @SuppressWarnings("deprecation")
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.prefs);
    mLocationUpdatePref = (ListPreference) findPreference(getString(R.string.pref_loc_update));
    updateSummary();

    findPreference(getString(R.string.pref_about)).setOnPreferenceClickListener(new OnPreferenceClickListener()
    {
      @Override
      public boolean onPreferenceClick(Preference preference)
      {
        final SpannableString linkifiedAbout = new SpannableString(getString(R.string.about));
        Linkify.addLinks(linkifiedAbout, Linkify.ALL);

        new AlertDialog.Builder(YopmePreference.this)
          .setTitle(R.string.about_title)
          .setMessage(linkifiedAbout)
          .create()
          .show();

        return true;
      }
    });

    findPreference(getString(R.string.menu_help)).setOnPreferenceClickListener(new OnPreferenceClickListener()
    {
      @Override
      public boolean onPreferenceClick(Preference preference)
      {
        startActivity(new Intent(getApplicationContext(), ReferenceActivity.class));
        return true;
      }
    });
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
    {
      updateSummary();

      Toast.makeText(this, getString(R.string.save_your_battery), Toast.LENGTH_LONG)
        .show();
    }
  }


}
