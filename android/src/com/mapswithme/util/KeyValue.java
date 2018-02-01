package com.mapswithme.util;

import android.support.annotation.NonNull;

final class KeyValue
{
  KeyValue(@NonNull String key, @NonNull String value)
  {
    this.key = key;
    this.value = value;
  }

  @NonNull
  public final String key;
  @NonNull
  public final String value;
}
