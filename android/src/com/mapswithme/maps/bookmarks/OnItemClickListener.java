package com.mapswithme.maps.bookmarks;

import android.view.View;

public interface OnItemClickListener<T>
{
  void onItemClick(View v, T item);
}
