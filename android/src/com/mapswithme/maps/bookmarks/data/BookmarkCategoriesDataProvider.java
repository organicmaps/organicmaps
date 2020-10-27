package com.mapswithme.maps.bookmarks.data;

import androidx.annotation.NonNull;

import java.util.List;

public interface BookmarkCategoriesDataProvider
{
  @NonNull
  List<BookmarkCategory> getCategories();
  @NonNull
  List<BookmarkCategory> getChildrenCategories(long parentId);
  @NonNull
  List<BookmarkCategory> getChildrenCollections(long parentId);
  @NonNull
  BookmarkCategory getCategoryById(long categoryId);
}
