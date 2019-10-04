package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

import java.util.List;

class CacheBookmarkCategoriesDataProvider extends AbstractBookmarkCategoriesDataProvider
{
  @NonNull
  @Override
  public List<BookmarkCategory> getCategories()
  {
    return BookmarkManager.INSTANCE.getBookmarkCategoriesCache().getItems();
  }
}
