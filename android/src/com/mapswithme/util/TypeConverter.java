package com.mapswithme.util;

import androidx.annotation.NonNull;

public interface TypeConverter<D, T>
{
  T convert(@NonNull D data);
}
