package app.organicmaps.util;

import androidx.annotation.NonNull;

import com.google.gson.annotations.SerializedName;

import java.io.Serializable;

public final class KeyValue implements Serializable
{
  private static final long serialVersionUID = -3079360274128509979L;
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
