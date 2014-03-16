package com.mapswithme.maps.bookmarks;

import android.os.Bundle;
import android.widget.BaseAdapter;

import com.mapswithme.maps.base.MapsWithMeBaseListActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public abstract class AbstractBookmarkCategoryActivity extends MapsWithMeBaseListActivity
{
  protected BookmarkManager mManager;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    mManager = BookmarkManager.getBookmarkManager(getApplicationContext());
  }

  @Override
  protected void onStart()
  {
    super.onStart();

    getAdapter().notifyDataSetChanged();
  }

  protected abstract BaseAdapter getAdapter();
}
