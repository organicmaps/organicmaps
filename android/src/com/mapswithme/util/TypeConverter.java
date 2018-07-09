package com.mapswithme.util;

import android.support.annotation.NonNull;

public interface TypeConverter<D, T>
{
  T convert(@NonNull D data);
}
