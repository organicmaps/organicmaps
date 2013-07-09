package com.mapswithme.maps.tests;

import com.mapswithme.maps.settings.SettingsActivity;
import com.squareup.spoon.Spoon;

import android.test.ActivityInstrumentationTestCase2;

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
}
