package com.mapswithme.maps.maplayer.traffic;

import android.support.annotation.MainThread;
import android.support.annotation.NonNull;

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

  @NonNull
  private TrafficState.Type mState = TrafficState.Type.DISABLED;

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

    if (isEnabled())
      disable();
    else
      enable();
  }

  private void enable()
  {
    mLogger.d(mTag, "Enable traffic");
    TrafficState.nativeEnable();
  }

  private void disable()
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
    mStateChangeListener.onTrafficStateChanged(mState.ordinal());
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

  public void setEnabled(boolean enabled)
  {
    checkInitialization();

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
      TrafficState.Type newTrafficState = TrafficState.getType(state);
      mLogger.d(mTag, "onTrafficStateChanged current state = " + mState
                      + " new value = " + newTrafficState);

      newTrafficState.onReceived(mCallbacks, mState);
      mState = newTrafficState;
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
