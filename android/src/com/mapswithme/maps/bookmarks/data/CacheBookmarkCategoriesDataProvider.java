package com.mapswithme.maps.bookmarks.data;

import androidx.annotation.NonNull;

import java.util.List;

class CacheBookmarkCategoriesDataProvider extends AbstractBookmarkCategoriesDataProvider
{
  @NonNull
  @Override
  public List<BookmarkCategory> getCategories()
  {
    return BookmarkManager.INSTANCE.getBookmarkCategoriesCache().getCategories();
  }
}
