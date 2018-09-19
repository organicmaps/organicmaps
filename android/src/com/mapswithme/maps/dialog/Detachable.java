package com.mapswithme.maps.dialog;

import android.support.annotation.NonNull;

public interface Detachable<T>
{
  void attach(@NonNull T object);
  void detach();
}
