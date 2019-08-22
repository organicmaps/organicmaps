package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.view.View;

import com.mapswithme.maps.widget.SearchToolbarController;

public class BookmarksToolbarController extends SearchToolbarController
{
  private final BookmarksListFragment mFragment;

  BookmarksToolbarController(View root, Activity activity, BookmarksListFragment fragment)
  {
    super(root, activity);
    mFragment = fragment;
  }

  @Override
  protected boolean alwaysShowClearButton()
  {
    return true;
  }

  @Override
  protected void onClearClick()
  {
    super.onClearClick();
    mFragment.deactivateSearch();
  }

  @Override
  protected void onTextChanged(String query)
  {
    if (hasQuery())
      mFragment.runSearch(getQuery());
    else
      mFragment.cancelSearch();
  }
}
