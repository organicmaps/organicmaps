package com.mapswithme.maps.base;

import android.support.annotation.NonNull;

public interface Detachable<T>
{
  void attach(@NonNull T object);
  void detach();
}
