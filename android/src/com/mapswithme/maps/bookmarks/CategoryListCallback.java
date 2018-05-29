package com.mapswithme.maps.bookmarks;

import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

interface CategoryListCallback
{
  void onFooterClick();

  void onMoreOperationClick(BookmarkCategory item);
}
