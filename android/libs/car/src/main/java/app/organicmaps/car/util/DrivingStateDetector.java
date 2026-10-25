package app.organicmaps.car.util;

import android.location.Location;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import androidx.annotation.WorkerThread;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.util.Assert;
import java.util.ArrayDeque;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class DrivingStateDetector implements LocationListener
{
  @MainThread
  public interface Callback {
    void onEnteredDrivingState();
  }

  private static final int DRIVING_SPEED_THRESHOLD_MPS = 5; // 18 km/h
  private static final int DRIVING_SPEED_TIME_THRESHOLD_MS = 30 * 1000; // 30 seconds

  // The Nth percentile speed within the sliding window must reach the threshold, i.e. up to
  // PERCENTILE percent of the (slowest) samples in the window (e.g. a red light or a slow turn)
  // may fall below it without resetting the detection. A higher value tolerates a larger slow
  // fraction of the window (more relaxed); keep it low so sustained driving is still required.
  private static final int DRIVING_SPEED_PERCENTILE = 20;

  private record SpeedSample(long timestampMs, float speedMps)
  {
  }

  @NonNull
  private final LocationHelper mLocationHelper;
  @NonNull
  private final Callback mCallback;
  @NonNull
  private final ExecutorService mExecutor = Executors.newSingleThreadExecutor();
  @NonNull
  private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());
  @NonNull
  private final ArrayDeque<SpeedSample> mSamples = new ArrayDeque<>();
  private boolean mCallbackTriggered;

  public DrivingStateDetector(@NonNull OrganicMaps organicMapsContext, @NonNull Callback callback)
  {
    // noinspection ConstantConditions
    Assert.debug(DRIVING_SPEED_PERCENTILE >= 0 && DRIVING_SPEED_PERCENTILE <= 100,
                 "DRIVING_SPEED_PERCENTILE must be in [0, 100]");
    mLocationHelper = organicMapsContext.getLocationHelper();
    mCallback = callback;
  }

  public void start()
  {
    mLocationHelper.addListener(this);
  }

  public void stop()
  {
    mLocationHelper.removeListener(this);
    mExecutor.execute(this::reset);
  }

  @WorkerThread
  private void reset()
  {
    mSamples.clear();
    mCallbackTriggered = false;
  }

  @Override
  @MainThread
  public void onLocationUpdated(@NonNull Location location)
  {
    final float speedMps = location.hasSpeed() ? location.getSpeed() : 0;
    final long nowMs = SystemClock.elapsedRealtime();
    mExecutor.execute(() -> checkDrivingState(new SpeedSample(nowMs, speedMps)));
  }

  @WorkerThread
  private void checkDrivingState(@NonNull SpeedSample sample)
  {
    if (mCallbackTriggered)
      return;

    final long windowStartMs = sample.timestampMs() - DRIVING_SPEED_TIME_THRESHOLD_MS;
    final SpeedSample oldestBeforeTrim = mSamples.peekFirst();
    SpeedSample oldest = oldestBeforeTrim;
    while (oldest != null && oldest.timestampMs() < windowStartMs)
    {
      mSamples.removeFirst();
      oldest = mSamples.peekFirst();
    }
    mSamples.addLast(sample);

    if (oldestBeforeTrim == null || oldestBeforeTrim.timestampMs() > windowStartMs)
      return;

    if (calculateSustainedSpeedMps() < DRIVING_SPEED_THRESHOLD_MPS)
      return;

    mCallbackTriggered = true;
    mSamples.clear();
    mMainThreadHandler.post(() -> {
      mLocationHelper.removeListener(this);
      mCallback.onEnteredDrivingState();
    });
  }

  @WorkerThread
  private float calculateSustainedSpeedMps()
  {
    final float[] speeds = new float[mSamples.size()];
    int i = 0;
    for (final SpeedSample sample : mSamples)
      speeds[i++] = sample.speedMps();

    final int index = (int) ((speeds.length - 1) * (DRIVING_SPEED_PERCENTILE / 100.0));
    return quickSelect(speeds, index);
  }

  // Returns the k-th smallest value of `values` (0-based), partially reordering it in the process
  // (Hoare's quickselect). Unlike Arrays.sort(), this doesn't fully order the array: O(n) on
  // average vs O(n log n).
  private static float quickSelect(@NonNull float[] values, int k)
  {
    int lo = 0, hi = values.length - 1;
    while (lo < hi)
    {
      final float pivot = values[(lo + hi) >>> 1];
      int i = lo, j = hi;
      while (i <= j)
      {
        while (values[i] < pivot)
          i++;
        while (values[j] > pivot)
          j--;
        if (i <= j)
        {
          final float tmp = values[i];
          values[i] = values[j];
          values[j] = tmp;
          i++;
          j--;
        }
      }
      if (k <= j)
        hi = j;
      else if (k >= i)
        lo = i;
      else
        break;
    }
    return values[k];
  }
}
