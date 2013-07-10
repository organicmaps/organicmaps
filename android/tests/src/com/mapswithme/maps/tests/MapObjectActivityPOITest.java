package com.mapswithme.maps.tests;

import static com.mapswithme.maps.MapObjectActivity.EXTRA_LAT;
import static com.mapswithme.maps.MapObjectActivity.EXTRA_LON;
import static com.mapswithme.maps.MapObjectActivity.EXTRA_NAME;
import static com.mapswithme.maps.MapObjectActivity.EXTRA_OBJECT_TYPE;
import static com.mapswithme.maps.MapObjectActivity.EXTRA_TYPE;
import static org.fest.assertions.api.ANDROID.assertThat;
import android.content.Intent;
import android.test.ActivityInstrumentationTestCase2;
import android.widget.Button;
import android.widget.TextView;

import com.jayway.android.robotium.solo.Solo;
import com.mapswithme.maps.MapObjectActivity;
import com.mapswithme.maps.MapObjectFragment;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkActivity;
import com.squareup.spoon.Spoon;

public class MapObjectActivityPOITest extends ActivityInstrumentationTestCase2<MapObjectActivity>
{

  public MapObjectActivityPOITest()
  {
    super(MapObjectActivity.class);
  }

  public void testView_POI()
  {
    setActivityIntent(getPOIIntent());
    final MapObjectActivity activity = getActivity();
    final Solo solo = new Solo(getInstrumentation(), activity);
    Spoon.screenshot(activity, "initial_state_poi");

    final TextView name = (TextView) activity.findViewById(R.id.name);
    assertThat(name).hasText(POI_NAME);

    final TextView type = (TextView) activity.findViewById(R.id.type);
    assertThat(type).hasText(POI_TYPE);

    final Button editBtn = (Button) activity.findViewById(R.id.editBookmark);
    assertThat(editBtn).isGone();

    final Button openWithBtn = (Button) activity.findViewById(R.id.openWith);
    assertThat(openWithBtn).isGone();

    final Button addToBmksButton = (Button) activity.findViewById(R.id.addToBookmarks);
    solo.clickOnView(addToBmksButton);
    solo.waitForActivity(BookmarkActivity.class, 5000);

    solo.finishOpenedActivities();
  }

  private static String POI_NAME = "MapsWithMe Office";
  private static String POI_TYPE = "Cool place";

  private Intent getPOIIntent()
  {
    final Intent i = new Intent();
    i.putExtra(EXTRA_OBJECT_TYPE, MapObjectFragment.MapObjectType.POI);
    i.putExtra(EXTRA_NAME, POI_NAME);
    i.putExtra(EXTRA_TYPE, POI_TYPE);
    i.putExtra(EXTRA_LAT, 59.9001);
    i.putExtra(EXTRA_LON, 27.5603);
    return i;
  }

}
