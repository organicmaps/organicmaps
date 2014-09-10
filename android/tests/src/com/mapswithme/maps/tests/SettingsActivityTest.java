package com.mapswithme.maps.tests;

import android.preference.CheckBoxPreference;
import android.test.ActivityInstrumentationTestCase2;

import com.mapswithme.maps.R;
import com.mapswithme.maps.settings.SettingsActivity;
import com.mapswithme.util.statistics.Statistics;
import com.squareup.spoon.Spoon;

public class SettingsActivityTest extends ActivityInstrumentationTestCase2<SettingsActivity>
{

  SettingsActivity activity;

  public SettingsActivityTest()
  {
    super(SettingsActivity.class);
  }

  @Override
  protected void setUp() throws Exception
  {
    super.setUp();
    activity = getActivity();
  }

  public void testSetUp_Settings()
  {
    assertNotNull(activity);
    Spoon.screenshot(activity, "settings");

  }

  @SuppressWarnings("deprecation")
  public void testStat_CheckBox()
  {
   CheckBoxPreference pref =  (CheckBoxPreference)activity.findPreference(activity.getString(R.string.pref_allow_stat));
   assertNotNull(pref);
   assertEquals(Statistics.INSTANCE.isStatisticsEnabled(), pref.isChecked());
  }
}
