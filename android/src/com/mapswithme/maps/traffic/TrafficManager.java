package com.mapswithme.maps.traffic;

import android.support.annotation.DrawableRes;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;

import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

@MainThread
public enum TrafficManager
{
  INSTANCE;
  private final String mTag = TrafficManager.class.getSimpleName();
  @NonNull
  private final Logger mLogger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.TRAFFIC);
  @NonNull
  private final TrafficState.StateChangeListener mStateChangeListener = new TrafficStateListener();
  @TrafficState.Value
  private int mState = TrafficState.DISABLED;
  @TrafficState.Value
  private int mLastPostedState = mState;
  @NonNull
  private final List<TrafficCallback> mCallbacks = new ArrayList<>();
  private boolean mInitialized = false;

  public void initialize()
  {
    mLogger.d(mTag, "Initialization of traffic manager and setting the listener for traffic state changes");
    TrafficState.nativeSetListener(mStateChangeListener);
    mInitialized = true;
  }

  public void toggle()
  {
    checkInitialization();

    if (mState == TrafficState.DISABLED)
      enable();
    else
      disable();
  }

  public void enable()
  {
    mLogger.d(mTag, "Enable traffic");
    TrafficState.nativeEnable();
  }

  public void disable()
  {
    checkInitialization();

    mLogger.d(mTag, "Disable traffic");
    TrafficState.nativeDisable();
  }
  
  public boolean isEnabled()
  {
    checkInitialization();
    return TrafficState.nativeIsEnabled();
  }

  public void attach(@NonNull TrafficCallback callback)
  {
    checkInitialization();

    if (mCallbacks.contains(callback))
    {
      throw new IllegalStateException("A callback '" + callback
                                      + "' is already attached. Check that the 'detachAll' method was called.");
    }
    mLogger.d(mTag, "Attach callback '" + callback + "'");
    mCallbacks.add(callback);
    postPendingState();
  }

  private void postPendingState()
  {
    mStateChangeListener.onTrafficStateChanged(mState);
  }

  public void detachAll()
  {
    checkInitialization();

    if (mCallbacks.isEmpty())
    {
      mLogger.w(mTag, "There are no attached callbacks. Invoke the 'detachAll' method " +
                                      "only when it's really needed!", new Throwable());
      return;
    }

    for (TrafficCallback callback : mCallbacks)
      mLogger.d(mTag, "Detach callback '" + callback + "'");
    mCallbacks.clear();
  }

  private void checkInitialization()
  {
    if (!mInitialized)
      throw new AssertionError("Traffic manager is not initialized!");
  }

  @DrawableRes
  public int getIconRes()
  {
    return isEnabled() ? getEnabledStateIcon() : getDisabledStateIcon();
  }

  private int getEnabledStateIcon()
  {
    return R.drawable.ic_traffic_on;
  }

  private int getDisabledStateIcon()
  {
    return ThemeUtils.isNightTheme()
           ? R.drawable.ic_traffic_menu_dark_off
           : R.drawable.ic_traffic_menu_light_off;
  }

  public void setEnabled(boolean enabled)
  {
    if (isEnabled() == enabled)
      return;

    if (enabled)
      enable();
    else
      disable();
  }

  private class TrafficStateListener implements TrafficState.StateChangeListener
  {
    @Override
    @MainThread
    public void onTrafficStateChanged(@TrafficState.Value int state)
    {
      mLogger.d(mTag, "onTrafficStateChanged current state = " + TrafficState.nameOf(mState)
                + " new value = " + TrafficState.nameOf(state));
      mState = state;

      iterateOverCallbacks(new Utils.Proc<TrafficCallback>()
      {
        @Override
        public void invoke(@NonNull TrafficCallback callback)
        {
          switch (mState)
          {
            case TrafficState.DISABLED:
              callback.onDisabled();
              break;

            case TrafficState.ENABLED:
              callback.onEnabled();
              break;

            case TrafficState.WAITING_DATA:
              callback.onWaitingData();
              break;

            case TrafficState.NO_DATA:
              callback.onNoData(mLastPostedState != mState);
              break;

            case TrafficState.OUTDATED:
              callback.onOutdated();
              break;

            case TrafficState.NETWORK_ERROR:
              callback.onNetworkError();
              break;

            case TrafficState.EXPIRED_DATA:
                callback.onExpiredData(mLastPostedState != mState);
              break;

            case TrafficState.EXPIRED_APP:
                callback.onExpiredApp(mLastPostedState != mState);
              break;

            default:
              throw new IllegalArgumentException("Unsupported traffic state: " + mState);
          }
          mLastPostedState = mState;
        }
      });
    }

    private void iterateOverCallbacks(@NonNull Utils.Proc<TrafficCallback> proc)
    {
      for (TrafficCallback callback : mCallbacks)
        proc.invoke(callback);
    }
  }

  public interface TrafficCallback
  {
    void onEnabled();
    void onDisabled();
    void onWaitingData();
    void onOutdated();
    void onNetworkError();
    void onNoData(boolean notify);
    void onExpiredData(boolean notify);
    void onExpiredApp(boolean notify);
  }
}
