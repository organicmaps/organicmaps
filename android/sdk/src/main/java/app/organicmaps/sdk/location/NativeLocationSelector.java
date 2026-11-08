package app.organicmaps.sdk.location;

import android.location.Location;
import android.location.LocationManager;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.location.LocationCompat;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/** Selects a coherent native stream; subscription ownership stays in AndroidNativeProvider. */
final class NativeLocationSelector
{
  record Selection(String provider, Location location, boolean sourceChanged, String reason, String previousQuality,
                   String quality)
  {
  }

  private record Source(String provider, boolean mock)
  {
  }

  private static final class Observation
  {
    Location mLocation;
    int mBetterMeasurements;
    int mPoorMeasurements;
    long mDeliveryLatency;

    Observation(Location location)
    {
      mLocation = new Location(location);
    }
  }

  private record Attribute(boolean present, float accuracy)
  {
  }

  private static final double ADEQUATE_HORIZONTAL_ACCURACY_METRES = 25;
  private static final double PROMOTION_RATIO = 0.7;
  private static final double PROMOTION_MARGIN_METRES = 3;
  private static final double COMPARABLE_RATIO = 1.1;
  private static final int PROMOTION_MEASUREMENTS = 2;

  private final Map<Source, Observation> mObservations = new HashMap<>();
  private final Set<String> mUnavailable = new HashSet<>();
  @Nullable
  private Source mSelected;
  @Nullable
  private Source mLastDeliveredSource;
  private long mMaxAgeNanos;

  NativeLocationSelector(long interval)
  {
    updateInterval(interval);
  }

  void updateInterval(long interval)
  {
    mMaxAgeNanos = TimeUnit.MILLISECONDS.toNanos(Math.max(5000, 3 * interval));
  }

  boolean isFresh(@NonNull Location location, long now)
  {
    final long time = location.getElapsedRealtimeNanos();
    return time > 0 && time <= now && now - time < mMaxAgeNanos;
  }

  @Nullable
  String selectedProvider()
  {
    return mSelected == null ? null : mSelected.provider();
  }

  boolean selectedMock()
  {
    return mSelected != null && mSelected.mock();
  }

  boolean hasFreshSelection(long now)
  {
    final Observation current = mObservations.get(mSelected);
    return current != null && isFresh(current.mLocation, now);
  }

  boolean needsSatelliteAcquisition(long now)
  {
    if (mSelected == null || mSelected.mock() || LocationManager.GPS_PROVIDER.equals(mSelected.provider()))
      return false;
    final Observation current = mObservations.get(mSelected);
    return current == null || !isFresh(current.mLocation, now)
 || LocationManager.NETWORK_PROVIDER.equals(mSelected.provider())
 || radius(current.mLocation) > ADEQUATE_HORIZONTAL_ACCURACY_METRES;
  }

  boolean degraded(long now)
  {
    final Observation current = mObservations.get(mSelected);
    return current == null || !isFresh(current.mLocation, now) || current.mPoorMeasurements >= 2;
  }

  long selectedExpiry()
  {
    final Observation current = mObservations.get(mSelected);
    return current == null ? 0 : current.mLocation.getElapsedRealtimeNanos() + mMaxAgeNanos;
  }

  @Nullable
  Selection observe(@NonNull String provider, @NonNull Location location, long now)
  {
    if (!isFresh(location, now) || !LocationUtils.isAccuracySatisfied(location))
      return null;
    final Source source = new Source(provider, LocationCompat.isMock(location));
    final Observation previous = mObservations.get(source);
    final long time = location.getElapsedRealtimeNanos();
    final boolean distinct = previous == null || time > previous.mLocation.getElapsedRealtimeNanos();
    if (previous != null && time < previous.mLocation.getElapsedRealtimeNanos())
      return null;
    if (!distinct && !LocationUtils.isLocationBetterThanLast(location, previous.mLocation))
      return null;
    mUnavailable.remove(provider);

    final Observation incoming = previous == null ? new Observation(location) : previous;
    if (previous != null && time - previous.mLocation.getElapsedRealtimeNanos() >= mMaxAgeNanos)
      incoming.mBetterMeasurements = 0;
    if (distinct)
    {
      incoming.mDeliveryLatency = now - time;
      incoming.mPoorMeasurements =
          radius(location) > ADEQUATE_HORIZONTAL_ACCURACY_METRES ? Math.min(2, incoming.mPoorMeasurements + 1) : 0;
    }
    incoming.mLocation = new Location(location);
    mObservations.put(source, incoming);

    final Observation current = mObservations.get(mSelected);
    if (Objects.equals(source, mSelected))
    {
      return new Selection(provider, new Location(location), false, "", "", "");
    }

    // Respect Android's explicit mock stream rather than mixing it with live fixes.
    if (source.mock() && (!selectedMock() || !hasFreshSelection(now)))
      return select(source, "mock stream", now);
    if (selectedMock() && hasFreshSelection(now) && !source.mock())
      return null;
    if (current == null || !isFresh(current.mLocation, now))
      return select(source, recoveryReason(), now);

    final String reason = promotionReason(current, incoming);
    if (reason != null)
    {
      if (distinct)
        ++incoming.mBetterMeasurements;
      if (incoming.mBetterMeasurements >= PROMOTION_MEASUREMENTS)
        return select(source, reason, now);
    }
    else
      incoming.mBetterMeasurements = 0;
    return null;
  }

  void unavailable(@NonNull String provider)
  {
    mUnavailable.add(provider);
    for (var entry : mObservations.entrySet())
      if (provider.equals(entry.getKey().provider()))
        entry.getValue().mBetterMeasurements = 0;
    if (mSelected != null && provider.equals(mSelected.provider()))
      mSelected = null;
  }

  @Nullable
  Selection expire(long now)
  {
    if (hasFreshSelection(now))
      return null;
    mSelected = null;
    Source best = null;
    for (var entry : mObservations.entrySet())
    {
      if (mUnavailable.contains(entry.getKey().provider()) || !isFresh(entry.getValue().mLocation, now))
        continue;
      if (best == null || entry.getKey().mock() && !best.mock()
          || entry.getKey().mock() == best.mock()
                 && effectiveRadius(entry.getValue().mLocation, now)
                        < effectiveRadius(mObservations.get(best).mLocation, now))
        best = entry.getKey();
    }
    return best == null ? null : select(best, recoveryReason() + "; cached observation", now);
  }

  private String recoveryReason()
  {
    if (mLastDeliveredSource == null)
      return "initial source";
    return mUnavailable.contains(mLastDeliveredSource.provider()) ? "selected source unavailable"
                                                                  : "selected source stale";
  }

  private Selection select(@NonNull Source source, @NonNull String reason, long now)
  {
    final Observation previous = mObservations.get(mLastDeliveredSource);
    final String previousQuality =
        previous == null ? "none" : mLastDeliveredSource.provider() + " " + quality(previous, now);
    final Observation selected = mObservations.get(source);
    final boolean changed = !source.equals(mSelected);
    mSelected = source;
    mLastDeliveredSource = source;
    for (Observation observation : mObservations.values())
      observation.mBetterMeasurements = 0;
    return new Selection(source.provider(), new Location(selected.mLocation), changed, reason, previousQuality,
                         quality(selected, now));
  }

  String probeSummary(long now)
  {
    final StringBuilder summary = new StringBuilder();
    for (var entry : mObservations.entrySet())
      summary.append(entry.getKey()).append("{").append(quality(entry.getValue(), now)).append("} ");
    return summary.toString();
  }

  private static String quality(Observation observation, long now)
  {
    final Location point = observation.mLocation;
    final Attribute speed = speed(point);
    final Attribute bearing = bearing(point, false);
    final Attribute altitude = altitude(point);
    return "hAcc=" + point.getAccuracy() + " deliveryMs=" + TimeUnit.NANOSECONDS.toMillis(observation.mDeliveryLatency)
  + " ageMs=" + TimeUnit.NANOSECONDS.toMillis(now - point.getElapsedRealtimeNanos()) + " speed=" + speed.present()
  + "/" + speed.accuracy() + " bearing=" + bearing.present() + "/" + bearing.accuracy()
  + " altitude=" + altitude.present() + "/" + altitude.accuracy();
  }

  private static double radius(Location location)
  {
    final float accuracy = location.getAccuracy();
    return location.hasAccuracy() && accuracy > 0 && Float.isFinite(accuracy) ? accuracy : Double.POSITIVE_INFINITY;
  }

  private static double effectiveRadius(Location location, long now)
  {
    final double speed = Math.max(5, location.hasSpeed() ? location.getSpeed() : 0);
    return radius(location) + speed * (now - location.getElapsedRealtimeNanos()) * 1e-9;
  }

  @Nullable
  private static String promotionReason(Observation active, Observation incoming)
  {
    final Location current = active.mLocation;
    final Location candidate = incoming.mLocation;
    // Waiting for a source's next normal sample is not delivery latency.
    final double currentRadius = effectiveRadius(current, current.getElapsedRealtimeNanos() + active.mDeliveryLatency);
    final double candidateRadius =
        effectiveRadius(candidate, candidate.getElapsedRealtimeNanos() + incoming.mDeliveryLatency);
    if (!Double.isFinite(currentRadius))
      return Double.isFinite(candidateRadius) && candidateRadius <= ADEQUATE_HORIZONTAL_ACCURACY_METRES
           && compareAttributes(current, candidate) >= 0
        ? "known useful horizontal accuracy"
        : null;
    if (candidateRadius <= currentRadius * PROMOTION_RATIO
        && currentRadius - candidateRadius >= PROMOTION_MARGIN_METRES)
      return "horizontal accuracy or delivery latency";
    if (candidateRadius > currentRadius * COMPARABLE_RATIO)
      return null;

    return compareAttributes(current, candidate) > 0 ? "capability or uncertainty dominance" : null;
  }

  // -1: an attribute becomes worse or unknown, 0: no proven gain, 1: useful gain without a loss.
  private static int compareAttributes(Location current, Location candidate)
  {
    final boolean moving =
        current.hasSpeed() && current.getSpeed() > 1 || candidate.hasSpeed() && candidate.getSpeed() > 1;
    final int speed = compare(speed(current), speed(candidate), 0.5f /* m/s */);
    final int bearing = compare(bearing(current, moving), bearing(candidate, moving), 5 /* degrees */);
    final int altitude = compare(altitude(current), altitude(candidate), 1 /* metre */);
    return Math.min(speed, Math.min(bearing, altitude)) < 0 ? -1 : Math.max(speed, Math.max(bearing, altitude));
  }

  private static int compare(Attribute current, Attribute candidate, float minimumImprovement)
  {
    if (current.present() && !candidate.present())
      return -1;
    if (current.present() && Float.isFinite(current.accuracy())
        && (!Float.isFinite(candidate.accuracy()) || candidate.accuracy() > current.accuracy() * COMPARABLE_RATIO))
      return -1;
    if (candidate.present() && !current.present())
      return 1;
    return current.present() && candidate.present() && Float.isFinite(current.accuracy())
         && Float.isFinite(candidate.accuracy()) && candidate.accuracy() <= current.accuracy() * PROMOTION_RATIO
         && current.accuracy() - candidate.accuracy() >= minimumImprovement
      ? 1
      : 0;
  }

  private static Attribute speed(Location location)
  {
    return new Attribute(location.hasSpeed(), Build.VERSION.SDK_INT >= 26 && location.hasSpeedAccuracy()
                                                  ? location.getSpeedAccuracyMetersPerSecond()
                                                  : Float.NaN);
  }

  private static Attribute bearing(Location location, boolean moving)
  {
    final boolean useful = location.hasSpeed() ? location.getSpeed() > 1 : moving;
    return new Attribute(useful && location.hasBearing(), Build.VERSION.SDK_INT >= 26 && location.hasBearingAccuracy()
                                                              ? location.getBearingAccuracyDegrees()
                                                              : Float.NaN);
  }

  private static Attribute altitude(Location location)
  {
    if (Build.VERSION.SDK_INT >= 34 && location.hasMslAltitude())
    {
      final float accuracy = location.hasMslAltitudeAccuracy() ? location.getMslAltitudeAccuracyMeters() : Float.NaN;
      return new Attribute(Float.isNaN(accuracy) || accuracy >= 0, accuracy);
    }
    final float accuracy = Build.VERSION.SDK_INT >= 26 && location.hasVerticalAccuracy()
                             ? location.getVerticalAccuracyMeters()
                             : Float.NaN;
    return new Attribute(location.hasAltitude() && (Float.isNaN(accuracy) || accuracy >= 0), accuracy);
  }
}
