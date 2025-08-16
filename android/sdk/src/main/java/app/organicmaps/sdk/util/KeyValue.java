package app.organicmaps.sdk.util;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import java.io.Serial;
import java.io.Serializable;

// Used from JNI.
@Keep
@SuppressWarnings("unused")
public final class KeyValue implements Serializable
{
  @Serial
  private static final long serialVersionUID = -3079360274128509979L;
  @NonNull
  private final String mKey;
  @NonNull
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
