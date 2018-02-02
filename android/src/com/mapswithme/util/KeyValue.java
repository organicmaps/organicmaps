package com.mapswithme.util;

import android.support.annotation.NonNull;

final class KeyValue
{
  KeyValue(@NonNull String key, @NonNull String value)
  {
    mKey = key;
    mValue = value;
  }

  @NonNull
  public final String mKey;
  @NonNull
  public final String mValue;
}
