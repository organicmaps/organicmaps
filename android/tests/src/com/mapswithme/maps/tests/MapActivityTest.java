package com.mapswithme.maps.tests;

import com.mapswithme.maps.MWMActivity;
import com.squareup.spoon.Spoon;

import android.app.Instrumentation;
import android.test.ActivityInstrumentationTestCase2;

public class MapActivityTest extends ActivityInstrumentationTestCase2<MWMActivity>
{

  MWMActivity activity;
  Instrumentation instrumentation;


  public MapActivityTest()
  {
    super(MWMActivity.class);
  }


  @Override
  protected void setUp() throws Exception
  {
    super.setUp();

    activity = getActivity();
    instrumentation = getInstrumentation();
  }


  public void testSetUp_Map()
  {
    assertNotNull(activity);

    // wait for initialization
    try
    {
      Thread.sleep(7 * 1000);
    }
    catch (InterruptedException e)
    { fail("Didn't wait for screenshot"); }

    Spoon.screenshot(activity, "map");
  }

}
