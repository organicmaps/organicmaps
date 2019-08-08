package com.mapswithme.util;

import android.support.annotation.NonNull;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.widget.StackedButtonDialogFragment;
import com.mapswithme.util.statistics.Analytics;
import com.mapswithme.util.statistics.Statistics;

import java.util.concurrent.TimeUnit;

public final class NetworkPolicy
{
  public enum Type
  {
    ASK(new Analytics(Statistics.ParamValue.ASK)),

    ALWAYS(new Analytics(Statistics.ParamValue.ALWAYS))
        {
          @Override
          public void check(@NonNull FragmentManager fragmentManager,
                            @NonNull NetworkPolicyListener listener, boolean isDialogAllowed)
          {
            boolean nowInRoaming = ConnectionState.isInRoaming();
            boolean acceptedInRoaming = Config.getMobileDataRoaming();

            if (nowInRoaming && !acceptedInRoaming)
              showDialog(fragmentManager, listener);
            else
              listener.onResult(new NetworkPolicy(true));
          }
        },

    NEVER(new Analytics(Statistics.ParamValue.NEVER))
        {
          @Override
          public void check(@NonNull FragmentManager fragmentManager,
                            @NonNull NetworkPolicyListener listener, boolean isDialogAllowed)
          {
            if (isDialogAllowed)
              showDialog(fragmentManager, listener);
            else
              listener.onResult(new NetworkPolicy(false));
          }
        },


    NOT_TODAY(new Analytics(Statistics.ParamValue.NOT_TODAY))
        {
          @Override
          public void check(@NonNull FragmentManager fragmentManager,
                            @NonNull NetworkPolicyListener listener, boolean isDialogAllowed)
          {
            if (isDialogAllowed)
              showDialog(fragmentManager, listener);
            else
              showDialogIfNeeded(fragmentManager, listener, new NetworkPolicy(false));
          }
        },

    TODAY(new Analytics(Statistics.ParamValue.TODAY))
        {
          @Override
          public void check(@NonNull FragmentManager fragmentManager,
                            @NonNull NetworkPolicyListener listener, boolean isDialogAllowed)
          {
            boolean nowInRoaming = ConnectionState.isInRoaming();
            boolean acceptedInRoaming = Config.getMobileDataRoaming();

            if (nowInRoaming && !acceptedInRoaming)
              showDialog(fragmentManager, listener);
            else
              showDialogIfNeeded(fragmentManager, listener, new NetworkPolicy(true));
          }
        },

    NONE
        {
          @NonNull
          @Override
          public Analytics getAnalytics()
          {
            throw new UnsupportedOperationException("Not supported here " + name());
          }
        };

    @NonNull
    private final Analytics mAnalytics;

    Type(@NonNull Analytics analytics)
    {

      mAnalytics = analytics;
    }

    Type()
    {
      this(new Analytics(""));
    }

    public void check(@NonNull FragmentManager fragmentManager,
                      @NonNull final NetworkPolicyListener listener,
                      boolean isDialogAllowed)
    {
      showDialog(fragmentManager, listener);
    }

    @NonNull
    public Analytics getAnalytics()
    {
      return mAnalytics;
    }
  }


  public static final int NONE = -1;

  private static final String TAG_NETWORK_POLICY = "network_policy";

  public static void checkNetworkPolicy(@NonNull FragmentManager fragmentManager,
                                        @NonNull final NetworkPolicyListener listener,
                                        boolean isDialogAllowed)
  {
    if (ConnectionState.isWifiConnected())
    {
      listener.onResult(new NetworkPolicy(true));
      return;
    }

    if (!ConnectionState.isMobileConnected())
    {
      listener.onResult(new NetworkPolicy(false));
      return;
    }

    Type type = Config.getUseMobileDataSettings();
    type.check(fragmentManager, listener, isDialogAllowed);
  }

  public static void checkNetworkPolicy(@NonNull FragmentManager fragmentManager,
                                        @NonNull final NetworkPolicyListener listener)
  {
    checkNetworkPolicy(fragmentManager, listener, false);
  }

  public static boolean getCurrentNetworkUsageStatus()
  {
    if (ConnectionState.isWifiConnected())
      return true;

    if (!ConnectionState.isMobileConnected())
      return false;

    boolean nowInRoaming = ConnectionState.isInRoaming();
    boolean acceptedInRoaming = Config.getMobileDataRoaming();
    if (nowInRoaming && !acceptedInRoaming)
      return false;

    Type type = Config.getUseMobileDataSettings();
    return type == Type.ALWAYS || (type == Type.TODAY && isToday());
  }

  private static boolean isToday()
  {
    long timestamp = Config.getMobileDataTimeStamp();
    return TimeUnit.MILLISECONDS.toDays(System.currentTimeMillis() - timestamp) < 1;
  }

  private static void showDialogIfNeeded(@NonNull FragmentManager fragmentManager,
                                         @NonNull NetworkPolicyListener listener,
                                         @NonNull NetworkPolicy policy)
  {
    if (isToday())
    {
      listener.onResult(policy);
      return;
    }
    showDialog(fragmentManager, listener);
  }

  private static void showDialog(@NonNull FragmentManager fragmentManager,
                                 @NonNull NetworkPolicyListener listener)
  {
    StackedButtonDialogFragment dialog = (StackedButtonDialogFragment) fragmentManager
        .findFragmentByTag(TAG_NETWORK_POLICY);
    if (dialog != null)
      dialog.dismiss();

    dialog = new StackedButtonDialogFragment();
    dialog.setListener(listener);
    dialog.show(fragmentManager, TAG_NETWORK_POLICY);
  }

  public static NetworkPolicy newInstance(boolean canUse)
  {
    return new NetworkPolicy(canUse);
  }

  private final boolean mCanUseNetwork;

  private NetworkPolicy(boolean canUse)
  {
    mCanUseNetwork = canUse;
  }

  public boolean canUseNetwork()
  {
    return mCanUseNetwork;
  }

  public interface NetworkPolicyListener
  {
    void onResult(@NonNull NetworkPolicy policy);
  }
}
