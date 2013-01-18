package com.mapswithme.maps.bookmarks;

import com.mapswithme.maps.bookmarks.data.BookmarkManager;

import android.app.ListActivity;
import android.os.Bundle;

public abstract class AbstractBookmarkListActivity extends ListActivity
{
  protected BookmarkManager mManager;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mManager = BookmarkManager.getPinManager(getApplicationContext());
  }
}
