package com.mapswithme.maps.base;

import androidx.annotation.Nullable;

public interface Initializable<T>
{
  void initialize(@Nullable T t);
  void destroy();
}
