package com.mapswithme.maps.bookmarks;

import android.view.View;

import androidx.annotation.NonNull;

public interface OnItemMoreClickListener<T>
{
  void onItemMoreClick(@NonNull View v, @NonNull T item);
}
