package com.mapswithme.maps.bookmarks;

import android.support.annotation.NonNull;
import android.view.View;

public interface OnItemLongClickListener<T>
{
  void onItemLongClick(@NonNull View v, @NonNull T item);
}
