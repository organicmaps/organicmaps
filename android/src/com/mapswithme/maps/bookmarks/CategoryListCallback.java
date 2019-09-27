package com.mapswithme.maps.bookmarks;

import androidx.annotation.NonNull;

import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

interface CategoryListCallback
{
  void onFooterClick();

  void onMoreOperationClick(@NonNull BookmarkCategory item);
}
