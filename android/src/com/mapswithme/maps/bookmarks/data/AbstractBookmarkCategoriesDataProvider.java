package com.mapswithme.maps.bookmarks.data;

import androidx.annotation.NonNull;

import java.util.List;

abstract class AbstractBookmarkCategoriesDataProvider implements BookmarkCategoriesDataProvider
{
  @NonNull
  @Override
  public BookmarkCategory getCategoryById(long categoryId)
  {
    List<BookmarkCategory> categories = getCategories();

    for (BookmarkCategory each : categories)
    {
      if (each.getId() == categoryId)
        return each;

    }
    throw new IllegalArgumentException("There is no category for id : " + categoryId);
  }
}
