package app.organicmaps.util;

import android.location.Location;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.time.Duration;
import java.time.LocalTime;
import java.time.ZonedDateTime;
import java.time.temporal.ChronoUnit;
import java.util.concurrent.TimeUnit;

/**
 * Maintains a theme that depends on time of day for the current app session.
 *
 * <p>The controller applies an immediate fallback based on local time when
 * location is still unavailable, switches to location-based sunrise/sunset
 * calculation after the first valid location, and keeps scheduling the next stable
 * transition while the time-dependent mode stays active.</p>
 *
 * <p>Sunrise and sunset are handled with a symmetric buffer. Outside that
 * transition window the stable day or night theme is applied immediately.
 * Inside the window the currently visible theme is preserved, and the next
 * re-evaluation is scheduled for the end of the window. This avoids abrupt
 * corrections when the precise location becomes available near a boundary.</p>
 *
 * <p>The native astronomical day type is used only to split polar and non-polar
 * cases. Polar day and polar night are applied immediately. Ordinary day and
 * night still go through the buffered sunrise/sunset path, because applying
 * them directly would break the "keep current theme" behavior inside the
 * transition window.</p>
 *
 * <p>It does not decide whether the app should use a time-dependent theme at
 * all. That decision belongs to {@link ThemeSwitcher}. This class only handles
 * the mechanics of the day/night schedule once that mode is selected.</p>
 */
final class SynchronizedThemeController
{
  interface ThemeDelegate
  {
    void applyTheme(@NonNull Config.UiTheme theme);
  }

  private static final LocalTime DEFAULT_LIGHT_THEME_START = LocalTime.of(7, 0);
  private static final LocalTime DEFAULT_LIGHT_THEME_END = LocalTime.of(18, 0);
  private static final Duration TRANSITION_BUFFER = Duration.ofMinutes(15);

  @NonNull
  private final LocationHelper mLocationHelper;

  @NonNull
  private final ThemeDelegate mThemeDelegate;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Config.UiTheme mDaytimeTheme;

  // Only the first valid location should interrupt fallback mode immediately.
  // After that, LocationHelper may keep refreshing savedLocation, but the active
  // timer must stay stable until the next scheduled re-evaluation.
  private boolean mWaitingForInitialLocation = false;

  SynchronizedThemeController(@NonNull LocationHelper locationHelper, @NonNull ThemeDelegate themeDelegate)
  {
    mLocationHelper = locationHelper;
    mThemeDelegate = themeDelegate;
  }

  private final Runnable mAutoDarkChecker = new Runnable() {
    @Override
    public void run()
    {
      // Keep a single active timer and always recompute from current state.
      UiThread.cancelDelayedTasks(mAutoDarkChecker);

      final Location location = mLocationHelper.getSavedLocation();
      if (location == null)
      {
        scheduleNextCheck(computeFallbackDelayUntilNextBoundaryMs());
        waitForInitialLocation();
        applyFallbackTheme();
      }
      else
      {
        stopWaitingForInitialLocation();
        applyAstronomicalTheme(location, System.currentTimeMillis() / 1000L);
      }
    }
  };

  private final LocationListener mInitialLocationListener = location ->
  {
    if (!mWaitingForInitialLocation)
      return;

    stopWaitingForInitialLocation();
    UiThread.run(mAutoDarkChecker);
  };

  @androidx.annotation.UiThread
  void synchronizeTimeDependentTheme(@NonNull Config.UiTheme dayTheme)
  {
    mDaytimeTheme = dayTheme;
    UiThread.run(mAutoDarkChecker);
  }

  void stop()
  {
    UiThread.cancelDelayedTasks(mAutoDarkChecker);
    stopWaitingForInitialLocation();
  }

  private void applyFallbackTheme()
  {
    final LocalTime now = LocalTime.now();
    final boolean isDayNow = !now.isBefore(DEFAULT_LIGHT_THEME_START) && now.isBefore(DEFAULT_LIGHT_THEME_END);
    if (isDayNow)
      mThemeDelegate.applyTheme(mDaytimeTheme);
    else
      mThemeDelegate.applyTheme(Config.UiTheme.DARK);
  }

  private void applyAstronomicalTheme(@NonNull Location location, long nowSeconds)
  {
    final double latitude = location.getLatitude();
    final double longitude = location.getLongitude();
    final @Framework.DayTimeType int dayTimeType = Framework.nativeGetDayTimeType(nowSeconds, latitude, longitude);

    if (dayTimeType == Framework.DAY_TIME_TYPE_POLAR_DAY)
    {
      applyDayTheme();
      return;
    }
    if (dayTimeType == Framework.DAY_TIME_TYPE_POLAR_NIGHT)
    {
      applyNightTheme();
      return;
    }

    final long sunriseSeconds = Framework.nativeGetSunriseTime(nowSeconds, latitude, longitude);
    final long sunsetSeconds = Framework.nativeGetSunsetTime(nowSeconds, latitude, longitude);
    applyBufferedAstronomicalTheme(nowSeconds, latitude, longitude, sunriseSeconds, sunsetSeconds);
  }

  /**
   * Schedules the next stable sunrise/sunset-based boundary for a non-polar location.
   *
   * <p>The controller does not switch theme exactly at sunrise or sunset. Instead, it uses a
   * symmetric buffer around both events. For a 15-minute buffer, the morning transition window
   * is {@code [sunrise - 15m, sunrise + 15m)} and the evening transition window is
   * {@code [sunset - 15m, sunset + 15m)}.</p>
   *
   * <p>The purpose of this buffer is to avoid abrupt corrections around astronomical boundaries.
   * Near sunrise or sunset the exact result may shift when a more accurate location becomes
   * available, and even without a location change the user can cross the boundary while the app
   * is starting or resuming. Inside the buffer window we preserve the currently visible theme and
   * only wake up again at the end of that window, when the theme becomes stable.</p>
   *
   * <p>Example: if sunrise is 06:00 and the buffer is 15 minutes, then from 05:45 until 06:15
   * the controller keeps the current theme unchanged. If the app is still in that window, the next
   * check is scheduled for 06:15. After 06:15 the daytime theme is considered stable. The same
   * rule is applied symmetrically around sunset.</p>
   *
   * <p>This method also applies the stable day or night theme immediately when the current time
   * is outside the buffer window. Inside the buffer window it preserves the current theme and only
   * plans the next wake-up time.</p>
   */
  private void applyBufferedAstronomicalTheme(long nowSeconds, double latitude, double longitude, long sunriseSeconds,
                                              long sunsetSeconds)
  {
    final long bufferSeconds = TRANSITION_BUFFER.getSeconds();
    final long morningStart = sunriseSeconds - bufferSeconds;
    final long morningEnd = sunriseSeconds + bufferSeconds;
    final long eveningStart = sunsetSeconds - bufferSeconds;
    final long eveningEnd = sunsetSeconds + bufferSeconds;

    if (eveningStart < morningEnd)
    {
      // Extremely rare high-latitude edge case: the daylight interval is shorter than the
      // combined transition buffer. Fall back to the dark theme
      applyNightTheme();
      return;
    }

    if (nowSeconds < morningStart)
    {
      applyNightTheme();
      scheduleNextAstronomicalCheck(morningEnd);
      return;
    }
    if (nowSeconds < morningEnd)
    {
      scheduleNextAstronomicalCheck(morningEnd);
      return;
    }
    if (nowSeconds < eveningStart)
    {
      applyDayTheme();
      scheduleNextAstronomicalCheck(eveningEnd);
      return;
    }
    if (nowSeconds < eveningEnd)
    {
      scheduleNextAstronomicalCheck(eveningEnd);
      return;
    }
    // After today's sunset, ask for the next calendar day's sunrise using any timestamp from that day.
    final long nextDayReferenceSeconds = nowSeconds + TimeUnit.DAYS.toSeconds(1);
    final long nextSunriseSeconds = Framework.nativeGetSunriseTime(nextDayReferenceSeconds, latitude, longitude);
    if (nextSunriseSeconds <= 0)
      return;
    applyNightTheme();
    scheduleNextAstronomicalCheck(nextSunriseSeconds + bufferSeconds);
  }

  private void applyDayTheme()
  {
    mThemeDelegate.applyTheme(mDaytimeTheme);
  }

  private void applyNightTheme()
  {
    mThemeDelegate.applyTheme(Config.UiTheme.DARK);
  }

  private void scheduleNextAstronomicalCheck(long nextChangeTimeSeconds)
  {
    final long currentTimeSeconds = System.currentTimeMillis() / 1000L;
    final long delayMs = Math.max(0L, (nextChangeTimeSeconds - currentTimeSeconds) * 1000L);
    scheduleNextCheck(delayMs);
  }

  /**
   * Returns the delay until the next local fallback boundary while no valid
   * location-based sunrise/sunset calculation is available yet.
   */
  private long computeFallbackDelayUntilNextBoundaryMs()
  {
    final var now = ZonedDateTime.now();
    final var nowTime = now.toLocalTime();
    final LocalTime nextChangeTime;
    if (nowTime.isBefore(DEFAULT_LIGHT_THEME_START))
    {
      nextChangeTime = DEFAULT_LIGHT_THEME_START;
    }
    else if (nowTime.isBefore(DEFAULT_LIGHT_THEME_END))
    {
      nextChangeTime = DEFAULT_LIGHT_THEME_END;
    }
    else
    {
      nextChangeTime = DEFAULT_LIGHT_THEME_START;
    }

    ZonedDateTime nextDateTime = now.with(nextChangeTime);
    if (!nextDateTime.isAfter(now))
      nextDateTime = nextDateTime.plusDays(1);
    return ChronoUnit.MILLIS.between(now, nextDateTime);
  }

  private void scheduleNextCheck(long delayMs)
  {
    UiThread.cancelDelayedTasks(mAutoDarkChecker);
    UiThread.runLater(mAutoDarkChecker, delayMs);
  }

  private void waitForInitialLocation()
  {
    if (mWaitingForInitialLocation)
      return;

    mWaitingForInitialLocation = true;
    mLocationHelper.addListener(mInitialLocationListener);
  }

  private void stopWaitingForInitialLocation()
  {
    if (!mWaitingForInitialLocation)
      return;

    mWaitingForInitialLocation = false;
    mLocationHelper.removeListener(mInitialLocationListener);
  }
}
