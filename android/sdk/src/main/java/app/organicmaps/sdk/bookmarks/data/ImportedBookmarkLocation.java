package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/** Original source location and ownership metadata, without lossy coordinate text conversion. */
@Keep
public final class ImportedBookmarkLocation
{
  @NonNull
  public final String key;
  @NonNull
  public final String address;
  public final double latitude;
  public final double longitude;
  public final long categoryId;
  public final boolean canModify;
  @NonNull
  public final String fingerprint;

  public ImportedBookmarkLocation(@NonNull String key, @NonNull String address, double latitude, double longitude,
                                  long categoryId, boolean canModify, @NonNull String fingerprint)
  {
    this.key = key;
    this.address = address;
    this.latitude = latitude;
    this.longitude = longitude;
    this.categoryId = categoryId;
    this.canModify = canModify;
    this.fingerprint = fingerprint;
  }
}
