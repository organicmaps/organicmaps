package com.mapswithme.maps.traffic;

import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.util.Utils;
import com.mapswithme.util.log.DebugLogger;
import com.mapswithme.util.log.Logger;

import java.util.ArrayList;
import java.util.List;

public enum TrafficManager
{
  INSTANCE;
  @NonNull
  private final Logger mLogger = new DebugLogger(TrafficManager.class.getSimpleName());
  @NonNull
  private final TrafficState.StateChangeListener mStateChangeListener = new TrafficStateListener();
  @TrafficState.Value
  private int mState = TrafficState.DISABLED;
  @NonNull
  private final List<TrafficCallback> mCallbacks = new ArrayList<>();

  TrafficManager()
  {
    mLogger.d("Traffic manager initialization");
    setCoreStateChangedListener();
  }

  private void setCoreStateChangedListener()
  {
    mLogger.d("Set core traffic state listener");
    TrafficState.nativeSetListener(mStateChangeListener);
  }

  public void enableOrDisable()
  {
    if (mState == TrafficState.DISABLED)
    {
      enable();
      return;
    }

    disable();
  }

  private void enable()
  {
    mLogger.d("Enable traffic");
    TrafficState.nativeEnable();
  }

  public void disable()
  {
    mLogger.d("Disable traffic");
    TrafficState.nativeDisable();
  }

  public boolean isEnabled()
  {
    return mState != TrafficState.DISABLED;
  }

  public void attach(@NonNull TrafficCallback callback)
  {
    if (mCallbacks.contains(callback))
    {
      throw new IllegalStateException("A callback '" + callback
                                      + "' is already attached. Check that the 'detach' method was called.");
    }

    mLogger.d("Attach callback '" + callback + "'");
    mCallbacks.add(callback);
    postPendingState();
  }

  private void postPendingState()
  {
    mStateChangeListener.onTrafficStateChanged(mState);
  }

  public void detach()
  {
    if (mCallbacks.isEmpty())
    {
      throw new IllegalStateException("There are no attached callbacks. Invoke the 'detach' method " +
                                      "only when it's really needed!");
    }

    for (TrafficCallback callback : mCallbacks)
      mLogger.d("Detach callback '" + callback + "'");
    mCallbacks.clear();
  }

  private class TrafficStateListener implements TrafficState.StateChangeListener
  {
    @Override
    @MainThread
    public void onTrafficStateChanged(@TrafficState.Value int state)
    {
      mLogger.d("onTrafficStateChanged current state = " + TrafficState.nameOf(mState)
                + " new value = " + TrafficState.nameOf(state));
      mState = state;

      if (mCallbacks.isEmpty())
        return;

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
              callback.onNoData();
              break;

            case TrafficState.OUTDATED:
              callback.onOutdated();
              break;

            case TrafficState.NETWORK_ERROR:
              callback.onNetworkError();
              break;

            case TrafficState.EXPIRED_DATA:
              callback.onExpiredData();
              break;

            case TrafficState.EXPIRED_APP:
              callback.onExpiredApp();
              break;

            default:
              throw new IllegalArgumentException("Unsupported traffic state: " + mState);
          }
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

    void onNoData();

    void onNetworkError();

    void onExpiredData();

    void onExpiredApp();
  }
}
