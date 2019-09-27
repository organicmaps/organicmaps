package com.mapswithme.maps.content;

import androidx.annotation.NonNull;

public interface DataSource<D>
{
  @NonNull
  D getData();
}
