package com.mapswithme.util;

import androidx.annotation.NonNull;

public final class KeyValue
{
  public KeyValue(@NonNull String key, @NonNull String value)
  {
    mKey = key;
    mValue = value;
  }

  @NonNull
  public final String mKey;
  @NonNull
  public final String mValue;
}
