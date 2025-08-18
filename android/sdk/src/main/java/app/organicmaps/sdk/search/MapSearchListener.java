package app.organicmaps.sdk.search;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

public interface MapSearchListener
{
  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  record Result(String countryId, String matchedString)
  {}

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  void onMapSearchResults(@NonNull Result[] results, long timestamp, boolean isLast);
}
