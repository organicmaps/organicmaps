package com.mapswithme.maps.scheduling;

import android.annotation.TargetApi;
import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.content.ComponentName;
import android.content.Context;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.os.Build;
import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.background.ConnectivityChangedReceiver;
import com.mapswithme.util.ConnectionState;

import java.util.Objects;
import java.util.concurrent.atomic.AtomicInteger;

public class ConnectivityJobScheduler implements ConnectivityListener
{
  public static final int PERIODIC_IN_MILLIS = 4000;

  @NonNull
  private final ConnectivityListener mMasterConnectivityListener;

  @NonNull
  private final AtomicInteger mCurrentNetworkType;

  public ConnectivityJobScheduler(@NonNull MwmApplication context)
  {
    mMasterConnectivityListener = Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP
                           ? new NativeConnectivityListener(context)
                           : new LegacyConnectivityListener(context);

    mCurrentNetworkType = new AtomicInteger(getCurrentNetworkType().ordinal());
  }

  @Override
  public void listen()
  {
    mMasterConnectivityListener.listen();
  }

  @NonNull
  public NetworkStatus getNetworkStatus()
  {
    ConnectionState.Type currentNetworkType = getCurrentNetworkType();
    int prevTypeIndex = mCurrentNetworkType.getAndSet(currentNetworkType.ordinal());
    ConnectionState.Type prevNetworkType = ConnectionState.Type.values()[prevTypeIndex];
    boolean isNetworkChanged = prevNetworkType != currentNetworkType;
    return new NetworkStatus(isNetworkChanged, currentNetworkType);
  }

  @NonNull
  private ConnectionState.Type getCurrentNetworkType()
  {
    return ConnectionState.requestCurrentType();
  }

  public static ConnectivityJobScheduler from(@NonNull Context context)
  {
    MwmApplication application = (MwmApplication) context.getApplicationContext();
    return (ConnectivityJobScheduler) application.getConnectivityListener();
  }

  @TargetApi(Build.VERSION_CODES.LOLLIPOP)
  private static class NativeConnectivityListener implements ConnectivityListener
  {
    @NonNull
    private final JobScheduler mJobScheduler;
    @NonNull
    private final Context mContext;

    NativeConnectivityListener(@NonNull Context context)
    {
      mContext = context;
      JobScheduler jobScheduler = (JobScheduler) mContext.getSystemService(Context.JOB_SCHEDULER_SERVICE);
      Objects.requireNonNull(jobScheduler);
      mJobScheduler = jobScheduler;
    }

    @Override
    public void listen()
    {
      ComponentName component = new ComponentName(mContext, NativeJobService.class);
      int jobId = NativeJobService.class.hashCode();
      JobInfo jobInfo = new JobInfo
          .Builder(jobId, component)
          .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY)
          .setPeriodic(PERIODIC_IN_MILLIS)
          .build();
      mJobScheduler.schedule(jobInfo);
    }
  }

  public static class NetworkStatus
  {
    private final boolean mNetworkStateChanged;
    @NonNull
    private final ConnectionState.Type mCurrentNetworkType;

    NetworkStatus(boolean networkStateChanged,
                  @NonNull ConnectionState.Type currentNetworkType)
    {
      mNetworkStateChanged = networkStateChanged;
      mCurrentNetworkType = currentNetworkType;
    }

    public boolean isNetworkStateChanged()
    {
      return mNetworkStateChanged;
    }

    @NonNull
    public ConnectionState.Type getCurrentNetworkType()
    {
      return mCurrentNetworkType;
    }
  }

  private static class LegacyConnectivityListener implements ConnectivityListener
  {
    @NonNull
    private final MwmApplication mContext;
    @NonNull
    private final ConnectivityChangedReceiver mReceiver;

    LegacyConnectivityListener(@NonNull MwmApplication context)
    {
      mContext = context;
      mReceiver = new ConnectivityChangedReceiver();
    }

    @Override
    public void listen()
    {
      IntentFilter filter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
      mContext.registerReceiver(mReceiver, filter);
    }
  }
}
