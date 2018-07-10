package com.mapswithme.maps.content;

import android.support.annotation.NonNull;

public interface DataSource<D>
{
  @NonNull
  D getData();
}
