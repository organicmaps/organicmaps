package com.mapswithme.util;

import androidx.annotation.NonNull;
import com.google.gson.annotations.SerializedName;

public final class KeyValue
{
  @NonNull
  @SerializedName("key")
  private final String mKey;
  @NonNull
  @SerializedName("value")
  private final String mValue;

  public KeyValue(@NonNull String key, @NonNull String value)
  {
    mKey = key;
    mValue = value;
  }

  @NonNull
  public String getKey()
  {
    return mKey;
  }

  @NonNull
  public String getValue()
  {
    return mValue;
  }
}
