package com.mapswithme.maps.traffic;

import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.util.log.DebugLogger;
import com.mapswithme.util.log.Logger;

import java.util.ArrayList;
import java.util.List;

public enum TrafficManager
{
  INSTANCE;

  @NonNull
  private final Logger mLogger;
  @NonNull
  private final TrafficState.StateChangeListener mStateChangeListener = new TrafficStateListener();
  @TrafficState.Value
  //TODO: Figure out what is the default state??
  private int mState = TrafficState.DISABLED;
  @Nullable
  private TrafficCallback mCallback;
  @NonNull
  private List<Integer> mPendingStates = new ArrayList<>();

  TrafficManager()
  {
    mLogger = new DebugLogger(TrafficManager.class.getSimpleName());
    logInitialization();
    setCoreStateChangedListener();
  }

  private void logInitialization()
  {
    mLogger.d("Initialization");
  }

  private void setCoreStateChangedListener()
  {
    mLogger.d("Set core traffic state listener");
    TrafficState.nativeSetListener(mStateChangeListener);
  }

  public void enable()
  {
    mLogger.d("Enable traffic");
    TrafficState.nativeEnable();
  }

  public void disable()
  {
    mLogger.d("Disable traffic");
    TrafficState.nativeDisable();
  }

  public void attach(@NonNull TrafficCallback callback)
  {
    if (mCallback != null)
    {
      throw new IllegalStateException("A callback '" + mCallback
                                      + "' is already attached, but another callback '"
                                      + callback + "' is trying to be attached. " +
                                      "Don't forget to detach callback!");
    }

    mCallback = callback;
    postPendingStatesIfNeeded();
  }

  private void postPendingStatesIfNeeded()
  {
    if (mPendingStates.isEmpty())
      return;

    for (@TrafficState.Value int state : mPendingStates)
      mStateChangeListener.onTrafficStateChanged(state);

    mPendingStates.clear();
  }

  public void detach()
  {
    if (mCallback == null)
      mLogger.e("TrafficCallback is already detached. Invoke the 'detach' method only when it's really needed!");
    mCallback = null;
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

      if (mCallback == null)
      {
        mPendingStates.add(state);
        return;
      }

      mPendingStates.clear();

      switch (mState)
      {
        case TrafficState.DISABLED:
          mCallback.onDisabled();
          break;

        case TrafficState.ENABLED:
          mCallback.onEnabled();
          break;

        case TrafficState.WAITING_DATA:
          mCallback.onWaitingData();
          break;

        case TrafficState.NO_DATA:
          mCallback.onNoData();
          break;

        case TrafficState.OUTDATED:
          mCallback.onOutdated();
          break;

        case TrafficState.NETWORK_ERROR:
          mCallback.onNetworkError();
          break;

        case TrafficState.EXPIRED_DATA:
          mCallback.onExpiredData();
          break;

        case TrafficState.EXPIRED_APP:
          mCallback.onExpiredApp();
          break;

        default:
          throw new IllegalArgumentException("Unsupported traffic state: " + state);
      }
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
