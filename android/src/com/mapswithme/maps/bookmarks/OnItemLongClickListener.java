package com.mapswithme.maps.bookmarks;

import androidx.annotation.NonNull;
import android.view.View;

public interface OnItemLongClickListener<T>
{
  void onItemLongClick(@NonNull View v, @NonNull T item);
}
