package com.mapswithme.maps.bookmarks;

import android.view.View;

public interface OnItemLongClickListener<T>
{
  void onItemLongClick(View v, T item);
}
