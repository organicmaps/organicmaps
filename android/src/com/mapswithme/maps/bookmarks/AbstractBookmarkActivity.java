package com.mapswithme.maps.bookmarks;

import com.mapswithme.maps.bookmarks.data.BookmarkManager;

import android.app.Activity;
import android.os.Bundle;

public abstract class AbstractBookmarkActivity extends Activity
{
  protected BookmarkManager mManager;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mManager = BookmarkManager.getPinManager(getApplicationContext());
  }
}
