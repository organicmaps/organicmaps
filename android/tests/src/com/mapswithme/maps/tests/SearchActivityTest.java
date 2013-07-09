package com.mapswithme.maps.tests;

import com.mapswithme.maps.SearchActivity;
import com.squareup.spoon.Spoon;

import android.app.Instrumentation;
import android.test.ActivityInstrumentationTestCase2;

public class SearchActivityTest extends ActivityInstrumentationTestCase2<SearchActivity>
{

  Instrumentation instrumentation;
  SearchActivity activity;

  public SearchActivityTest()
  {
    super(SearchActivity.class);
  }


  @Override
  protected void setUp() throws Exception
  {
    super.setUp();

    instrumentation = getInstrumentation();
    activity = getActivity();
  }


  public void testSetUp_Search()
  {
    assertNotNull(activity);
    Spoon.screenshot(activity, "search");
  }

}
