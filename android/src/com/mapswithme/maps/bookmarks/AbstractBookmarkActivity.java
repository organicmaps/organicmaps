package com.mapswithme.maps.bookmarks;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.view.MenuItem;

import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public abstract class AbstractBookmarkActivity extends Activity
{
  protected BookmarkManager mManager;

  @SuppressLint("NewApi")
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mManager = BookmarkManager.getBookmarkManager(getApplicationContext());
    if (Integer.parseInt(android.os.Build.VERSION.SDK) >= 11)
    {
      getActionBar().setDisplayHomeAsUpEnabled(true);
    }
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == android.R.id.home)
    {
      onBackPressed();
      return true;
    }
    else
      return super.onOptionsItemSelected(item);
  }
}
