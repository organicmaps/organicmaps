package com.mapswithme.maps.maplayer.traffic;

import android.support.annotation.MainThread;
import android.support.annotation.NonNull;

import com.mapswithme.util.statistics.Statistics;

import java.util.List;

enum TrafficState
{
  DISABLED
      {
        @Override
        protected void activateInternal(@NonNull TrafficManager.TrafficCallback callback,
                                        @NonNull TrafficState lastPostedState)
        {
          callback.onDisabled();
        }
      },

  ENABLED(Statistics.ParamValue.SUCCESS)
      {
        @Override
        protected void activateInternal(@NonNull TrafficManager.TrafficCallback callback,
                                        @NonNull TrafficState lastPostedState)
        {
          callback.onEnabled();
        }
      },

  WAITING_DATA
      {
        @Override
        protected void activateInternal(@NonNull TrafficManager.TrafficCallback callback,
                                        @NonNull TrafficState lastPostedState)
        {
          callback.onWaitingData();
        }
      },

  OUTDATED
      {
        @Override
        protected void activateInternal(@NonNull TrafficManager.TrafficCallback callback,
                                        @NonNull TrafficState lastPostedState)
        {
          callback.onOutdated();
        }
      },

  NO_DATA(Statistics.ParamValue.UNAVAILABLE)
      {
        @Override
        protected void activateInternal(@NonNull TrafficManager.TrafficCallback callback,
                                        @NonNull TrafficState lastPostedState)
        {
          callback.onNoData(lastPostedState != NO_DATA);
        }
      },

  NETWORK_ERROR(Statistics.EventParam.ERROR)
      {
        @Override
        protected void activateInternal(@NonNull TrafficManager.TrafficCallback callback,
                                        @NonNull TrafficState lastPostedState)
        {
          callback.onNetworkError();
        }
      },

  EXPIRED_DATA
      {
        @Override
        protected void activateInternal(@NonNull TrafficManager.TrafficCallback callback,
                                        @NonNull TrafficState lastPostedState)
        {
          callback.onExpiredData(lastPostedState != EXPIRED_DATA);
        }
      },

  EXPIRED_APP
      {
        @Override
        protected void activateInternal(@NonNull TrafficManager.TrafficCallback callback,
                                        @NonNull TrafficState lastPostedState)
        {
          callback.onExpiredApp(lastPostedState != EXPIRED_APP);
        }
      };

  @NonNull
  private final String mAnalyticsParamName;

  TrafficState()
  {
    mAnalyticsParamName = name();
  }

  TrafficState(@NonNull String analyticsParamName)
  {
    mAnalyticsParamName = analyticsParamName;
  }

  @NonNull
  private String getAnalyticsParamName()
  {
    return mAnalyticsParamName;
  }

  public void activate(@NonNull List<TrafficManager.TrafficCallback> trafficCallbacks,
                       @NonNull TrafficState lastPostedState)
  {
    for (TrafficManager.TrafficCallback callback : trafficCallbacks)
    {
      activateInternal(callback, lastPostedState);
      Statistics.INSTANCE.trackTrafficEvent(getAnalyticsParamName());
    }
  }

  protected abstract void activateInternal(@NonNull TrafficManager.TrafficCallback callback,
                                           @NonNull TrafficState lastPostedState);

  interface StateChangeListener
  {
    // This method is called from JNI layer.
    @SuppressWarnings("unused")
    @MainThread
    void onTrafficStateChanged(int state);
  }

  @MainThread
  static native void nativeSetListener(@NonNull StateChangeListener listener);

  static native void nativeRemoveListener();

  static native void nativeEnable();

  static native void nativeDisable();

  static native boolean nativeIsEnabled();
}
