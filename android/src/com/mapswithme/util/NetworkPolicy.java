package com.mapswithme.util;

import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.widget.StackedButtonDialogFragment;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.concurrent.TimeUnit;

public final class NetworkPolicy
{
  public static final int ASK = 0;
  public static final int ALWAYS = 1;
  public static final int NEVER = 2;
  public static final int NOT_TODAY = 3;
  public static final int TODAY = 4;

  private static final String TAG_NETWORK_POLICY = "network_policy";

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ ASK, ALWAYS, NEVER, NOT_TODAY, TODAY })
  @interface NetworkPolicyDef
  {
  }

  public static void checkNetworkPolicy(@NonNull FragmentManager fragmentManager,
                                        @NonNull final NetworkPolicyListener listener)
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


    boolean nowInRoaming = ConnectionState.isInRoaming();
    boolean acceptedInRoaming = Config.getMobileDataRoaming();
    int type = Config.getUseMobileDataSettings();
    switch (type)
    {
      case ASK:
        showDialog(fragmentManager, listener);
        break;
      case ALWAYS:
        if (nowInRoaming && !acceptedInRoaming)
          showDialog(fragmentManager, listener);
        else
          listener.onResult(new NetworkPolicy(true));
        break;
      case NEVER:
        listener.onResult(new NetworkPolicy(false));
        break;
      case NOT_TODAY:
        showDialogIfNeeded(fragmentManager, listener, new NetworkPolicy(false));
        break;
      case TODAY:
        if (nowInRoaming && !acceptedInRoaming)
          showDialog(fragmentManager, listener);
        else
          showDialogIfNeeded(fragmentManager, listener, new NetworkPolicy(true));
        break;
    }
  }

  private static void showDialogIfNeeded(@NonNull FragmentManager fragmentManager,
                                         @NonNull NetworkPolicyListener listener,
                                         @NonNull NetworkPolicy policy)
  {
    long timestamp = Config.getMobileDataTimeStamp();
    boolean showDialog = TimeUnit.MILLISECONDS.toDays(System.currentTimeMillis() - timestamp) >= 1;
    if (!showDialog)
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

  public boolean сanUseNetwork()
  {
    return mCanUseNetwork;
  }

  public interface NetworkPolicyListener
  {
    void onResult(@NonNull NetworkPolicy policy);
  }
}
