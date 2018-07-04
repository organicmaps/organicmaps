package com.mapswithme.maps.maplayer.traffic;

import android.support.annotation.IntDef;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;

import com.mapswithme.util.statistics.Statistics;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.List;

final class TrafficState
{
  interface StateChangeListener
  {
    // This method is called from JNI layer.
    @SuppressWarnings("unused")
    @MainThread
    void onTrafficStateChanged(@Value int state);
  }

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ DISABLED, ENABLED, WAITING_DATA, OUTDATED, NO_DATA, NETWORK_ERROR, EXPIRED_DATA, EXPIRED_APP})

  @interface Value {}

  // These values should correspond to
  // TrafficManager::TrafficState enum (from map/traffic_manager.hpp)
  private static final int DISABLED = 0;
  private static final int ENABLED = 1;
  private static final int WAITING_DATA = 2;
  private static final int OUTDATED = 3;
  private static final int NO_DATA = 4;
  private static final int NETWORK_ERROR = 5;
  private static final int EXPIRED_DATA = 6;
  private static final int EXPIRED_APP = 7;

  private TrafficState() {}

  @MainThread
  static native void nativeSetListener(@NonNull StateChangeListener listener);
  static native void nativeRemoveListener();
  static native void nativeEnable();
  static native void nativeDisable();
  static native boolean nativeIsEnabled();

  public enum Type
  {
    DISABLED
        {
          @Override
          protected void onReceivedInternal(@NonNull TrafficManager.TrafficCallback param,
                                            @NonNull Type lastPostedState)
          {
            param.onDisabled();
          }
        },
    ENABLED(Statistics.ParamValue.SUCCESS)
        {
          @Override
          protected void onReceivedInternal(@NonNull TrafficManager.TrafficCallback param,
                                            @NonNull Type lastPostedState)
          {
            param.onEnabled();
          }
        },

    WAITING_DATA
        {
          @Override
          protected void onReceivedInternal(@NonNull TrafficManager.TrafficCallback param,
                                            @NonNull Type lastPostedState)
          {
            param.onWaitingData();
          }
        },
    OUTDATED
        {
          @Override
          protected void onReceivedInternal(@NonNull TrafficManager.TrafficCallback param,
                                            @NonNull Type lastPostedState)
          {
            param.onOutdated();
          }
        },
    NO_DATA(Statistics.ParamValue.UNAVAILABLE)
        {
          @Override
          protected void onReceivedInternal(@NonNull TrafficManager.TrafficCallback param,
                                            @NonNull Type lastPostedState)
          {
            param.onNoData(lastPostedState != NO_DATA);
          }
        },
    NETWORK_ERROR(Statistics.ParamValue.ERROR)
        {
          @Override
          protected void onReceivedInternal(@NonNull TrafficManager.TrafficCallback param,
                                            @NonNull Type lastPostedState)
          {
            param.onNetworkError();
          }
        },
    EXPIRED_DATA
        {
          @Override
          protected void onReceivedInternal(@NonNull TrafficManager.TrafficCallback param,
                                            @NonNull Type lastPostedState)
          {
            param.onExpiredData(lastPostedState != EXPIRED_DATA);
          }
        },
    EXPIRED_APP
        {
          @Override
          protected void onReceivedInternal(@NonNull TrafficManager.TrafficCallback param,
                                            @NonNull Type lastPostedState)
          {
            param.onExpiredApp(lastPostedState != EXPIRED_APP);
          }
        };

    @NonNull
    private final String mAnalyticsParamName;

    Type()
    {
      mAnalyticsParamName = name();
    }

    Type(@NonNull String analyticsParamName)
    {
      mAnalyticsParamName = analyticsParamName;
    }

    @NonNull
    private String getAnalyticsParamName()
    {
      return mAnalyticsParamName;
    }

    public void onReceived(@NonNull List<TrafficManager.TrafficCallback> trafficCallbacks,
                           @NonNull Type lastPostedState)
    {
      for (TrafficManager.TrafficCallback callback : trafficCallbacks)
      {
        onReceivedInternal(callback, lastPostedState);
        Statistics.INSTANCE.trackTrafficEvent(getAnalyticsParamName());
      }
    }

    protected abstract void onReceivedInternal(@NonNull TrafficManager.TrafficCallback param,
                                               @NonNull Type lastPostedState);
  }

  public static Type getType(int index)
  {
    if (index < 0 || index >= Type.values().length)
      throw new IllegalArgumentException("Not found value for index = " + index);

    return Type.values()[index];
  }
}
