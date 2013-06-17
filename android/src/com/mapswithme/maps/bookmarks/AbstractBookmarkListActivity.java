package com.mapswithme.maps.bookmarks;

import android.annotation.SuppressLint;
import android.os.Bundle;

import com.mapswithme.maps.base.MapsWithMeBaseListActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public abstract class AbstractBookmarkListActivity extends MapsWithMeBaseListActivity
{
  protected BookmarkManager mManager;

  @SuppressLint("NewApi")
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mManager = BookmarkManager.getBookmarkManager(getApplicationContext());
  }
}
