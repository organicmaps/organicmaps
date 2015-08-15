package com.mapswithme.maps.tests;

import java.util.Random;

import android.test.ActivityInstrumentationTestCase2;

import com.jayway.android.robotium.solo.Solo;
import com.mapswithme.maps.DownloadUI;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.search.SearchActivity;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.squareup.spoon.Spoon;

public class MapActivityTest extends ActivityInstrumentationTestCase2<MwmActivity>
{
  private static final int TIME_OUT = 5*1000;

  public MapActivityTest()
  {
    super(MwmActivity.class);
  }

  public void testAllButtons()
  {
    final MwmActivity activity = getActivity();
    final Solo solo = new Solo(getInstrumentation(), activity);

    Spoon.screenshot(activity, "initial_state");
    // Click all button and count started activities
    solo.clickOnView(activity.findViewById(R.id.map_button_bookmarks));
    solo.waitForActivity(BookmarkCategoriesActivity.class, TIME_OUT);
    solo.getActivityMonitor().getLastActivity().finish();

    solo.clickOnView(activity.findViewById(R.id.map_button_download));
    solo.waitForActivity(DownloadUI.class, TIME_OUT);
    solo.getActivityMonitor().getLastActivity().finish();

    solo.clickOnView(activity.findViewById(R.id.map_button_search));
    solo.waitForActivity(SearchActivity.class, TIME_OUT);
    solo.getActivityMonitor().getLastActivity().finish();

    solo.clickOnView(activity.findViewById(R.id.my_position));

    // check zoom in/out doesn't crash us
    int randomNumderOfClicks = new Random().nextInt(10) + 5;
    for (int i = 0; i <= randomNumderOfClicks; i++)
      solo.clickOnView(activity.findViewById(R.id.map_button_plus));

    randomNumderOfClicks = new Random().nextInt(10) + 5;
    for (int i = 0; i <= randomNumderOfClicks; i++)
      solo.clickOnView(activity.findViewById(R.id.map_button_minus));


    Spoon.screenshot(activity, "on_clicks_tested");
  }




}
