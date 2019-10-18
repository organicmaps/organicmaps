package com.mapswithme.maps.bookmarks.data;

import androidx.annotation.NonNull;

import java.util.Arrays;
import java.util.List;

class CoreBookmarkCategoriesDataProvider extends AbstractBookmarkCategoriesDataProvider
{
  @NonNull
  @Override
  public List<BookmarkCategory> getCategories()
  {
    BookmarkCategory[] categories = BookmarkManager.INSTANCE.nativeGetBookmarkCategories();
    return Arrays.asList(categories);
  }
}
