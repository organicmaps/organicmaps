package app.organicmaps.sdk.location;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.Location;
import android.location.LocationManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.location.LocationListenerCompat;
import androidx.core.location.LocationManagerCompat;
import androidx.core.location.LocationRequestCompat;
import app.organicmaps.sdk.util.log.Logger;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.TimeUnit;

public class AndroidNativeProvider extends BaseLocationProvider
{
  private static final String TAG = AndroidNativeProvider.class.getSimpleName();
  private static final long PROBE_INTERVAL_NANOS = TimeUnit.SECONDS.toNanos(60);

  private final Context mContext;
  private final LocationManager mLocationManager;
  private final Handler mHandler;
  private final Set<String> mEnabled = new HashSet<>();
  private final HashMap<String, NativeLocationListener> mSubscriptions = new HashMap<>();
  private NativeLocationSelector mSelector;
  private long mInterval;
  private long mGeneration;
  private long mProbeUntil;
  private long mNextProbe;
  private boolean mActive;
  private boolean mDegraded;
  @Nullable
  private BroadcastReceiver mProviderChanges;
  private final Runnable mCheck = this::evaluate;
  private final Runnable mReportDisabled = () ->
  {
    if (mActive && mEnabled.isEmpty())
      mListener.onLocationDisabled();
  };

  private final class NativeLocationListener implements LocationListenerCompat
  {
    private final String mProvider;

    NativeLocationListener(String provider)
    {
      mProvider = provider;
    }

    private boolean isCurrent()
    {
      return mActive && mSubscriptions.get(mProvider) == this;
    }

    @Override
    public void onLocationChanged(@NonNull Location location)
    {
      if (!isCurrent())
        return;
      final long now = SystemClock.elapsedRealtimeNanos();
      deliver(mSelector.observe(mProvider, location, now), now);
      if (isCurrent())
        evaluate();
    }

    @Override
    public void onProviderDisabled(@NonNull String provider)
    {
      if (isCurrent())
        refreshProviders();
    }

    @Override
    public void onProviderEnabled(@NonNull String provider)
    {
      if (isCurrent())
        refreshProviders();
    }

    @Override
    public void onStatusChanged(String provider, int status, Bundle extras)
    {
      Logger.d(TAG, "Status changed for location provider: " + provider + "; new status = " + status);
    }
  }

  public AndroidNativeProvider(@NonNull Context context, @NonNull Listener listener)
  {
    this(context, listener, (LocationManager) context.getSystemService(Context.LOCATION_SERVICE),
         new Handler(Looper.getMainLooper()));
  }

  AndroidNativeProvider(@NonNull Context context, @NonNull Listener listener, @NonNull LocationManager manager,
                        @NonNull Handler handler)
  {
    super(listener);
    if (manager == null)
      throw new IllegalStateException("Can't get LOCATION_SERVICE");
    mContext = context;
    mLocationManager = manager;
    mHandler = handler;
  }

  @Override
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  public void start(long interval)
  {
    if (mActive)
      throw new IllegalStateException("Already started");
    mActive = true;
    ++mGeneration;
    mInterval = interval;
    mSelector = new NativeLocationSelector(interval);
    mDegraded = false;
    final long now = SystemClock.elapsedRealtimeNanos();
    beginProbe(now);
    final long generation = mGeneration;
    mProviderChanges = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        if (mActive && generation == mGeneration)
          refreshProviders();
      }
    };
    mContext.registerReceiver(mProviderChanges, new IntentFilter(LocationManager.PROVIDERS_CHANGED_ACTION));
    refreshProviders();
  }

  @Override
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  protected void updateInterval(long interval)
  {
    if (!mActive)
      return;
    mInterval = interval;
    mSelector.updateInterval(interval);
    final LocationRequestCompat request = request();
    final long generation = mGeneration;
    for (var entry : new HashMap<>(mSubscriptions).entrySet())
    {
      if (!mActive || generation != mGeneration)
        return;
      if (mSubscriptions.get(entry.getKey()) != entry.getValue())
        continue;
      LocationManagerCompat.requestLocationUpdates(mLocationManager, entry.getKey(), request, entry.getValue(),
                                                   mHandler.getLooper());
    }
    if (mActive && generation == mGeneration)
      evaluate();
  }

  @Override
  protected void onForeground()
  {
    if (mActive)
      refreshProviders();
  }

  private LocationRequestCompat request()
  {
    return new LocationRequestCompat.Builder(mInterval).setQuality(LocationRequestCompat.QUALITY_HIGH_ACCURACY).build();
  }

  private void beginProbe(long now)
  {
    mProbeUntil = now + TimeUnit.MILLISECONDS.toNanos(Math.max(3000, 2 * mInterval + 100));
    mNextProbe = mProbeUntil + PROBE_INTERVAL_NANOS;
  }

  private void deliver(@Nullable NativeLocationSelector.Selection selection, long now)
  {
    if (selection == null)
      return;
    if (selection.sourceChanged())
    {
      beginProbe(now);
      Logger.d(TAG, "Selected location source: " + selection.provider() + " reason=" + selection.reason()
                        + " previous={" + selection.previousQuality() + "} selected={" + selection.quality() + "}");
    }
    mListener.onLocationChanged(selection.location(), selection.sourceChanged());
  }

  private void refreshProviders()
  {
    if (!mActive)
      return;
    // Android filters inaccessible providers; newer versions also allow coarse GPS observations.
    final Set<String> enabled = new HashSet<>(mLocationManager.getProviders(true));
    enabled.remove(LocationManager.PASSIVE_PROVIDER);
    final boolean newlyEnabled = !mEnabled.containsAll(enabled);
    for (String previous : mEnabled)
      if (!enabled.contains(previous))
        mSelector.unavailable(previous);
    mEnabled.clear();
    mEnabled.addAll(enabled);
    final long now = SystemClock.elapsedRealtimeNanos();
    if (newlyEnabled && !mSelector.selectedMock())
      beginProbe(now);
    mHandler.removeCallbacks(mReportDisabled);
    if (mEnabled.isEmpty())
      mHandler.post(mReportDisabled);
    evaluate();
  }

  @SuppressLint("MissingPermission")
  private void evaluate()
  {
    if (!mActive)
      return;
    final long generation = mGeneration;
    long now = SystemClock.elapsedRealtimeNanos();
    deliver(mSelector.expire(now), now);
    if (!mActive || generation != mGeneration)
      return;
    // Application callbacks can consume the remaining lifetime of a cached selection.
    now = SystemClock.elapsedRealtimeNanos();
    if (mProbeUntil > 0 && mProbeUntil <= now)
    {
      Logger.d(TAG, "Native location probe ended: requested=" + mSubscriptions.keySet()
                        + " observations=" + mSelector.probeSummary(now));
      mProbeUntil = 0;
    }
    final boolean degraded = mSelector.degraded(now) && LocationUtils.checkFineLocationPermission(mContext);
    if (degraded && !mDegraded && mProbeUntil <= now)
      beginProbe(now);
    mDegraded = degraded;
    if (!mSelector.selectedMock() && mNextProbe <= now)
      beginProbe(now);
    final Set<String> wanted = new HashSet<>();
    if (!mSelector.hasFreshSelection(now) || mProbeUntil > now && !mSelector.selectedMock())
      wanted.addAll(mEnabled);
    final String selected = mSelector.selectedProvider();
    if (selected != null && mEnabled.contains(selected))
      wanted.add(selected);
    if (mSelector.needsSatelliteAcquisition(now) && mEnabled.contains(LocationManager.GPS_PROVIDER))
      wanted.add(LocationManager.GPS_PROVIDER);

    for (var iterator = mSubscriptions.entrySet().iterator(); iterator.hasNext();)
    {
      final var entry = iterator.next();
      if (wanted.contains(entry.getKey()))
        continue;
      final NativeLocationListener listener = entry.getValue();
      iterator.remove();
      LocationManagerCompat.removeUpdates(mLocationManager, listener);
    }
    for (String provider : wanted)
    {
      if (!mActive || generation != mGeneration)
        return;
      if (!mEnabled.contains(provider))
        continue;
      if (mSubscriptions.containsKey(provider))
        continue;
      final NativeLocationListener listener = new NativeLocationListener(provider);
      mSubscriptions.put(provider, listener);
      try
      {
        LocationManagerCompat.requestLocationUpdates(mLocationManager, provider, request(), listener,
                                                     mHandler.getLooper());
      }
      catch (SecurityException error)
      {
        mSubscriptions.remove(provider);
        Logger.w(TAG, "Location permission changed for provider " + provider);
      }
    }
    if (mActive && generation == mGeneration)
      scheduleCheck(now);
  }

  private void scheduleCheck(long now)
  {
    mHandler.removeCallbacks(mCheck);
    // With no selected stream all discovery sources are already requested.
    // Await their callbacks or provider changes instead of polling a silent receiver.
    if (!mSelector.hasFreshSelection(now))
      return;
    long deadline = mSelector.selectedExpiry();
    if (mProbeUntil > now)
      deadline = Math.min(deadline, mProbeUntil);
    if (!mSelector.selectedMock())
      deadline = Math.min(deadline, mNextProbe);
    mHandler.postDelayed(mCheck,
                         Math.max(1, TimeUnit.NANOSECONDS.toMillis(deadline - SystemClock.elapsedRealtimeNanos()) + 1));
  }

  @Override
  @SuppressLint("MissingPermission")
  public void stop()
  {
    if (!mActive)
      return;
    mActive = false;
    ++mGeneration;
    mHandler.removeCallbacks(mCheck);
    mHandler.removeCallbacks(mReportDisabled);
    if (mProviderChanges != null)
      mContext.unregisterReceiver(mProviderChanges);
    mProviderChanges = null;
    final var listeners = mSubscriptions.values().toArray(NativeLocationListener[] ::new);
    mSubscriptions.clear();
    mEnabled.clear();
    for (NativeLocationListener listener : listeners)
      LocationManagerCompat.removeUpdates(mLocationManager, listener);
  }
}
