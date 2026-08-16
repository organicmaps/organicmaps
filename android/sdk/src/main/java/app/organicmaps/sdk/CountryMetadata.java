package app.organicmaps.sdk;

import androidx.annotation.IntRange;
import androidx.annotation.NonNull;

public final class CountryMetadata
{
  public enum DrivingSide
  {
    Left, // e.g. England, Japan, Australia
    Right // e.g. USA, Germany
  }

  @NonNull
  public static DrivingSide getDrivingSide(@NonNull String countryId)
  {
    return switch (nativeGetDrivingSide(countryId))
    {
      case 0 -> DrivingSide.Left;
      case 1 -> DrivingSide.Right;
      default -> throw new IllegalStateException("Unexpected driving side value");
    };
  }

  @IntRange(from = 0, to = 1)
  private static native int nativeGetDrivingSide(@NonNull String countryId);
}
