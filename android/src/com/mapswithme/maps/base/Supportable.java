package com.mapswithme.maps.base;

import androidx.annotation.NonNull;

public interface Supportable<T>
{
  boolean support(@NonNull T object);
}
