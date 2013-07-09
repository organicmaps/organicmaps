package com.mapswithme.maps.tests;

import com.mapswithme.maps.MapObjectActivity;
import com.mapswithme.maps.MapObjectFragment;
import com.mapswithme.maps.R;
import com.squareup.spoon.Spoon;

import android.content.Context;
import android.content.Intent;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.widget.TextView;

import static com.mapswithme.maps.MapObjectActivity.*;

public class MapObjectActivityTest extends ActivityInstrumentationTestCase2<MapObjectActivity>
{
  MapObjectActivity activity;


  public MapObjectActivityTest()
  {
    super(MapObjectActivity.class);
  }


  @Override
  protected void setUp() throws Exception
  {
    super.setUp();
    activity = getActivity();
  }


  public void testSetUp_MapObject()
  {
    assertNotNull(activity);
    Spoon.screenshot(activity, "map_object");
  }


  @UiThreadTest
  public void testView_POI()
  {
    activity.handleIntent(getPOIIntent(activity));

    final TextView nameView = (TextView) activity.findViewById(R.id.name);
    final TextView typeView = (TextView) activity.findViewById(R.id.type);

    // check correct view, just as example
    TestingUtils.assertTextViewHasText(POI_NAME, nameView);
    TestingUtils.assertTextViewHasText(POI_TYPE, typeView);

    // otherwise view could be dirty
    activity.runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        Spoon.screenshot(activity, "map_object_poi");
      }
    });
  }


  private static String POI_NAME = "MapsWithMe Office";
  private static String POI_TYPE = "Cool place";


  private Intent getPOIIntent(Context context)
  {
    final Intent i = new Intent(context, MapObjectActivity.class);
    i.putExtra(EXTRA_OBJECT_TYPE, MapObjectFragment.MapObjectType.POI);
    i.putExtra(EXTRA_NAME, POI_NAME);
    i.putExtra(EXTRA_TYPE, POI_TYPE);
    i.putExtra(EXTRA_LAT, 59.9001);
    i.putExtra(EXTRA_LON, 27.5603);
    return i;
  }
}
