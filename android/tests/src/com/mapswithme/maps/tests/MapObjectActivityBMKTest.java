package com.mapswithme.maps.tests;

import static com.mapswithme.maps.MapObjectActivity.EXTRA_BMK_CAT;
import static com.mapswithme.maps.MapObjectActivity.EXTRA_BMK_INDEX;
import static com.mapswithme.maps.MapObjectActivity.EXTRA_OBJECT_TYPE;
import static org.fest.assertions.api.ANDROID.assertThat;
import android.content.Intent;
import android.test.ActivityInstrumentationTestCase2;
import android.util.Pair;
import android.widget.Button;
import android.widget.TextView;

import com.jayway.android.robotium.solo.Solo;
import com.mapswithme.maps.MapObjectActivity;
import com.mapswithme.maps.MapObjectFragment;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.squareup.spoon.Spoon;

public class MapObjectActivityBMKTest extends ActivityInstrumentationTestCase2<MapObjectActivity>
{

  BookmarkManager bookmarkManager;

  private final static String BMK_NAME = "Питер";
  private final static double BMK_LAT = 59.9393;
  private final static double BMK_LON = 30.3153;
  private Pair<Integer, Integer> catbmk;

  public MapObjectActivityBMKTest()
  {
    super(MapObjectActivity.class);
  }

  @Override
  protected void setUp() throws Exception
  {
    super.setUp();

    bookmarkManager = BookmarkManager.getBookmarkManager(getInstrumentation().getTargetContext());
    catbmk = bookmarkManager.addNewBookmark(BMK_NAME, BMK_LAT, BMK_LON);
  }


  public void testView_Bmk()
  {
    setActivityIntent(getBookmarkIntent());
    final MapObjectActivity activity = getActivity();
    final Solo solo = new Solo(getInstrumentation(), activity);

    Spoon.screenshot(activity, "initial_state_bmk");

    final TextView name = (TextView) activity.findViewById(R.id.name);
    assertThat(name).hasText(BMK_NAME);

    final Button openWithBtn = (Button) activity.findViewById(R.id.openWith);
    assertThat(openWithBtn).isGone();

    final Button addToBmksButton = (Button) activity.findViewById(R.id.addToBookmarks);
    assertThat(addToBmksButton).isGone();

    final TextView coords = (TextView) activity.findViewById(R.id.coords);
    assertThat(coords).containsText(String.valueOf(BMK_LAT));
    assertThat(coords).containsText(String.valueOf(BMK_LON));

    final Button editBtn = (Button) activity.findViewById(R.id.editBookmark);
    solo.clickOnView(editBtn);
    solo.waitForActivity(BookmarkActivity.class, 5000);

    solo.finishOpenedActivities();
  }


  private Intent getBookmarkIntent()
  {
    final Intent i = new Intent();
    i.putExtra(EXTRA_OBJECT_TYPE, MapObjectFragment.MapObjectType.BOOKMARK);
    i.putExtra(EXTRA_BMK_CAT, catbmk.first);
    i.putExtra(EXTRA_BMK_INDEX, catbmk.second);
    return i;
  }
}
