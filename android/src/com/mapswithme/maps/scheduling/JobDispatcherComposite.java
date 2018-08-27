package com.mapswithme.maps.scheduling;

import android.annotation.TargetApi;
import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.content.ComponentName;
import android.content.Context;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.background.ConnectivityChangedReceiver;

import java.util.Objects;
import java.util.concurrent.atomic.AtomicInteger;

public class JobDispatcherComposite implements JobDispatcher
{
  @NonNull
  private final JobDispatcher mMasterJobDispatcher;

  @NonNull
  private final MwmApplication mContext;

  @NonNull
  private final AtomicInteger mCurrentNetworkType;

  public JobDispatcherComposite(@NonNull MwmApplication context)
  {
    mContext = context;
    mMasterJobDispatcher = Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP
                           ? new NativeJobDispatcher(mContext)
                           : new LegacyJobDispatcher(mContext);

    mCurrentNetworkType = new AtomicInteger(getCurrentNetworkType().ordinal());
  }

  @Override
  public void dispatch()
  {
    mMasterJobDispatcher.dispatch();
  }

  @Override
  public void cancelAll()
  {
    mMasterJobDispatcher.cancelAll();
  }

  @NonNull
  public NetworkStatus getNetworkStatus()
  {
    NetworkType currentNetworkType = getCurrentNetworkType();
    int prevTypeIndex = mCurrentNetworkType.getAndSet(currentNetworkType.mType);
    NetworkType prevNetworkType = NetworkType.getInstance(prevTypeIndex);
    boolean isNetworkChanged = prevNetworkType != currentNetworkType;
    return new NetworkStatus(isNetworkChanged, currentNetworkType);
  }

  private NetworkType getCurrentNetworkType()
  {
    ConnectivityManager cm = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
    NetworkInfo activeNetwork;
    if (cm == null || (activeNetwork = cm.getActiveNetworkInfo()) == null)
      return NetworkType.UNDEFINED;

    return NetworkType.getInstance(activeNetwork.getType());
  }

  public enum NetworkType
  {
    WIFI(ConnectivityManager.TYPE_WIFI),
    MOBILE(ConnectivityManager.TYPE_MOBILE),
    UNDEFINED;

    private final int mType;

    NetworkType(int type)
    {

      mType = type;
    }

    NetworkType()
    {
      this(-1);
    }

    @NonNull
    public static NetworkType getInstance(int type)
    {
      for (NetworkType each : values())
      {
        if (each.mType == type)
          return each;
      }

      return NetworkType.UNDEFINED;
    }
  }

  public static JobDispatcherComposite from(@NonNull Context context)
  {
    MwmApplication application = (MwmApplication) context.getApplicationContext();
    return (JobDispatcherComposite) application.getJobDispatcher();
  }

  @TargetApi(Build.VERSION_CODES.LOLLIPOP)
  private static class NativeJobDispatcher implements JobDispatcher
  {
    private static final int PERIODIC_IN_MILLIS = 4000;
    @NonNull
    private final JobScheduler mJobScheduler;
    @NonNull
    private final Context mContext;

    NativeJobDispatcher(@NonNull Context context)
    {
      mContext = context;
      JobScheduler jobScheduler = (JobScheduler) mContext.getSystemService(Context.JOB_SCHEDULER_SERVICE);
      Objects.requireNonNull(jobScheduler);
      mJobScheduler = jobScheduler;
    }

    @Override
    public void dispatch()
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

    @Override
    public void cancelAll()
    {
      mJobScheduler.cancelAll();
    }
  }

  public static class NetworkStatus
  {
    private final boolean mNetworkStateChanged;
    @NonNull
    private final NetworkType mCurrentNetworkType;

    NetworkStatus(boolean networkStateChanged,
                  @NonNull NetworkType currentNetworkType)
    {
      mNetworkStateChanged = networkStateChanged;
      mCurrentNetworkType = currentNetworkType;
    }

    public boolean isNetworkStateChanged()
    {
      return mNetworkStateChanged;
    }

    @NonNull
    public NetworkType getCurrentNetworkType()
    {
      return mCurrentNetworkType;
    }
  }

  private static class LegacyJobDispatcher implements JobDispatcher
  {
    @NonNull
    private final MwmApplication mContext;
    @NonNull
    private final ConnectivityChangedReceiver mReceiver;

    LegacyJobDispatcher(@NonNull MwmApplication context)
    {
      mContext = context;
      mReceiver = new ConnectivityChangedReceiver();
    }

    @Override
    public void dispatch()
    {
      IntentFilter filter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
      mContext.registerReceiver(mReceiver, filter);
    }

    @Override
    public void cancelAll()
    {
      mContext.unregisterReceiver(mReceiver);
    }
  }
}
