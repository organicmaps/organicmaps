package app.organicmaps.location;

import android.app.PendingIntent;
import android.location.Location;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public interface LocationListener
{
  void onLocationUpdated(@NonNull Location location);

  default void onLocationDisabled(@Nullable PendingIntent pendingIntent)
  {
    // No op.
  }
}
