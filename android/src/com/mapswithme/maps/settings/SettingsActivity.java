package com.mapswithme.maps.settings;

import android.annotation.SuppressLint;
import android.app.ActionBar;
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
import android.view.MenuItem;
import android.view.inputmethod.InputMethodManager;

import com.mapswithme.maps.ContextMenu;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.statistics.Statistics;

public class SettingsActivity extends PreferenceActivity
{
  private final static String ZOOM_BUTTON_ENABLED = "ZoomButtonsEnabled";

  private native boolean isDownloadingActive();

  @SuppressLint("NewApi")
  @SuppressWarnings("deprecation")
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);


    if (Utils.apiEqualOrGreaterThan(11))
    {
      // http://stackoverflow.com/questions/6867076/getactionbar-returns-null
      final ActionBar bar = getActionBar();
      if (bar != null)
        bar.setDisplayHomeAsUpEnabled(true);
    }

    addPreferencesFromResource(R.xml.preferences);

    final Activity parent = this;

    Preference pref = findPreference(getString(R.string.pref_storage_activity));
    pref.setOnPreferenceClickListener(new OnPreferenceClickListener()
    {
      @Override
      public boolean onPreferenceClick(Preference preference)
      {
        if (isDownloadingActive())
        {
          new AlertDialog.Builder(parent)
          .setTitle(parent.getString(R.string.downloading_is_active))
          .setMessage(parent.getString(R.string.cant_change_this_setting))
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

    final ListPreference lPref = (ListPreference) findPreference(getString(R.string.pref_munits));

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


    final CheckBoxPreference allowStatsPreference = (CheckBoxPreference) findPreference(getString(R.string.pref_allow_stat));
    allowStatsPreference.setChecked(Statistics.INSTANCE.isStatisticsEnabled(this));
    allowStatsPreference.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        Statistics.INSTANCE.setStatEnabled(getApplicationContext(), (Boolean) newValue);
        return true;
      }
    });

    final CheckBoxPreference enableZoomButtons = (CheckBoxPreference) findPreference(getString(R.string.pref_zoom_btns_enabled));
    enableZoomButtons.setChecked(isZoomButtonsEnabled(MWMApplication.get()));
    enableZoomButtons.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        setZoomButtonEnabled(MWMApplication.get(), (Boolean)newValue);
        return true;
      }
    });

    pref = findPreference(getString(R.string.pref_about));
    pref.setOnPreferenceClickListener(new OnPreferenceClickListener()
    {
      @Override
      public boolean onPreferenceClick(Preference preference)
      {
        ContextMenu.onAboutDialogClicked(SettingsActivity.this);
        return true;
      }
    });

    yotaSetup();
  }

  @SuppressWarnings("deprecation")
  private void yotaSetup()
  {
    final Preference yopPreference = findPreference(getString(R.string.pref_yota));
    if (!Yota.isYota())
      getPreferenceScreen().removePreference(yopPreference);
    else
    {
      yopPreference.setOnPreferenceClickListener(new OnPreferenceClickListener()
      {
        @Override
        public boolean onPreferenceClick(Preference preference)
        {
          SettingsActivity.this.startActivity(new Intent(Yota.ACTION_PREFERENCE));
          return true;
        }
      });
      // we dont allow to change maps location
      getPreferenceScreen()
        .removePreference(findPreference(getString(R.string.pref_storage_activity)));
    }
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

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == android.R.id.home)
    {
      final InputMethodManager imm = (InputMethodManager) getSystemService(Activity.INPUT_METHOD_SERVICE);
      imm.toggleSoftInput(InputMethodManager.HIDE_IMPLICIT_ONLY, 0);
      onBackPressed();
      return true;
    }
    else
      return super.onOptionsItemSelected(item);
  }

  public static boolean isZoomButtonsEnabled(MWMApplication app)
  {
    return app.nativeGetBoolean(ZOOM_BUTTON_ENABLED, true);
  }

  public static void setZoomButtonEnabled(MWMApplication app, boolean isEnabled)
  {
    app.nativeSetBoolean(ZOOM_BUTTON_ENABLED, isEnabled);
  }
}
