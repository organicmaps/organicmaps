
package com.mapswithme.maps;

import android.annotation.SuppressLint;
import android.app.ActionBar;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.view.MenuItem;
import android.view.inputmethod.InputMethodManager;

import com.mapswithme.maps.MapObjectFragment.MapObjectType;
import com.mapswithme.maps.api.MWMRequest;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public class MapObjectActivity extends FragmentActivity
{
  
  private MapObjectFragment mFragment;
  
  public static String EXTRA_OBJECT_TYPE = "object_type";
  
  //for Bookmark
  private static String EXTRA_BMK_CAT = "bookmark_category";
  private static String EXTRA_BMK_INDEX = "bookmark_index";
  //for POI and API point
  private static String EXTRA_NAME = "name";
  private static String EXTRA_TYPE = "type";
  private static String EXTRA_ADDRESS = "address";
  private static String EXTRA_LAT = "lat";
  private static String EXTRA_LON = "lon";
  
  public static void startWithBookmark(Context context, int categoryIndex, int bookmarkIndex)
  {
    final Intent i = new Intent(context, MapObjectActivity.class);
    i.putExtra(EXTRA_OBJECT_TYPE, MapObjectFragment.MapObjectType.BOOKMARK);
    i.putExtra(EXTRA_BMK_CAT, categoryIndex);
    i.putExtra(EXTRA_BMK_INDEX, bookmarkIndex);
    context.startActivity(i);
  }
  
  public static void startWithPoi(Context context, String name, String type, String address, double lat, double lon)
  {
    final Intent i = new Intent(context, MapObjectActivity.class);
    i.putExtra(EXTRA_OBJECT_TYPE, MapObjectFragment.MapObjectType.POI);
    i.putExtra(EXTRA_NAME, name);
    i.putExtra(EXTRA_TYPE, type);
    i.putExtra(EXTRA_ADDRESS, address);
    i.putExtra(EXTRA_LAT, lat);
    i.putExtra(EXTRA_LON, lon);
    context.startActivity(i);
  }
  
  public static void startWithApiPoint(Context context, String name, String type, String address, double lat, double lon)
  {
    final Intent i = new Intent(context, MapObjectActivity.class);
    i.putExtra(EXTRA_OBJECT_TYPE, MapObjectFragment.MapObjectType.API_POINT);
    i.putExtra(EXTRA_NAME, name);
    i.putExtra(EXTRA_LAT, lat);
    i.putExtra(EXTRA_LON, lon);
    context.startActivity(i);
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_map_object);
    
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
    {
      // http://stackoverflow.com/questions/6867076/getactionbar-returns-null
      ActionBar bar = getActionBar();
      if (bar != null)
        bar.setDisplayHomeAsUpEnabled(true);
    }
    
    // Show the Up button in the action bar.
    mFragment = (MapObjectFragment)getSupportFragmentManager().findFragmentById(R.id.mapObjFragment);
    handleIntent(getIntent());
  }
  
  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);
    handleIntent(intent);
  }
  
  private void handleIntent(Intent intent)
  {
    final MapObjectType type = (MapObjectType) intent.getSerializableExtra(EXTRA_OBJECT_TYPE);
    if (type == MapObjectType.BOOKMARK)
    {
      final int categoryIndex = intent.getIntExtra(EXTRA_BMK_CAT, -1);
      final int bmkIndex      = intent.getIntExtra(EXTRA_BMK_INDEX, -1);
      mFragment.setForBookmark(BookmarkManager.getBookmarkManager(this).getBookmark(categoryIndex, bmkIndex));
    } 
    else if (type == MapObjectType.API_POINT)
    {
      mFragment.setForApiPoint(MWMRequest.getCurrentRequest());
    }
    else if (type == MapObjectType.POI)
    {
      final String name    = intent.getStringExtra(EXTRA_NAME);
      final String poiType = intent.getStringExtra(EXTRA_TYPE);
      final String address = intent.getStringExtra(EXTRA_ADDRESS);
      
      final double lat  = intent.getDoubleExtra(EXTRA_LAT, 0);
      final double lon  = intent.getDoubleExtra(EXTRA_LON, 0);
      mFragment.setForPoi(name, poiType, address, lat, lon);
    }
    
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == android.R.id.home)
    {
      InputMethodManager imm = (InputMethodManager) getSystemService(Activity.INPUT_METHOD_SERVICE);
      imm.toggleSoftInput(InputMethodManager.HIDE_IMPLICIT_ONLY, 0);
      onBackPressed();
      return true;
    }
    else
      return super.onOptionsItemSelected(item);
  }
  
}
