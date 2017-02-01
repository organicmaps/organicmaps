package com.mapswithme.util;

import android.content.Context;
import android.content.DialogInterface;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.StackedButtonsDialog;

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

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ ASK, ALWAYS, NEVER, NOT_TODAY, TODAY })
  @interface NetworkPolicyDef
  {
  }

  public static void checkNetworkPolicy(@NonNull Context context,
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

    int type = Config.getUseMobileDataSettings();
    switch (type)
    {
      case ASK:
        showDialog(context, listener);
        break;
      case ALWAYS:
        listener.onResult(new NetworkPolicy(true));
        break;
      case NEVER:
        listener.onResult(new NetworkPolicy(false));
        break;
      case NOT_TODAY:
        long timestamp = Config.getTodayTimeStamp();
        boolean showDialog = TimeUnit.MILLISECONDS.toDays(System.currentTimeMillis() - timestamp) >= 1;
        if (!showDialog)
        {
          listener.onResult(new NetworkPolicy(false));
          return;
        }
        showDialog(context, listener);
        break;
      case TODAY:
        timestamp = Config.getTodayTimeStamp();
        showDialog = TimeUnit.MILLISECONDS.toDays(System.currentTimeMillis() - timestamp) >= 1;
        if (!showDialog)
        {
          listener.onResult(new NetworkPolicy(true));
          return;
        }
        showDialog(context, listener);
        break;
    }
  }

  private static void showDialog(@NonNull Context context, @NonNull final NetworkPolicyListener listener)
  {
    new StackedButtonsDialog.Builder(context)
        .setTitle(R.string.mobile_data_dialog)
        .setMessage(R.string.mobile_data_description)
        .setCancelable(false)
        .setPositiveButton(R.string.mobile_data_option_always, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            Config.setUseMobileDataSettings(ALWAYS);
            listener.onResult(new NetworkPolicy(true));
          }
        })
        .setNegativeButton(R.string.mobile_data_option_not_today, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            Config.setUseMobileDataSettings(NOT_TODAY);
            Config.setTodayStamp(System.currentTimeMillis());
            listener.onResult(new NetworkPolicy(false));
          }
        })
        .setNeutralButton(R.string.mobile_data_option_today, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            Config.setUseMobileDataSettings(TODAY);
            Config.setTodayStamp(System.currentTimeMillis());
            listener.onResult(new NetworkPolicy(true));
          }
        })
        .build()
        .show();
  }

  private final boolean mCanUseNetwork;

  private NetworkPolicy(boolean canUse)
  {
    mCanUseNetwork = canUse;
  }

  public boolean isCanUseNetwork()
  {
    return mCanUseNetwork;
  }

  public interface NetworkPolicyListener
  {
    void onResult(@NonNull NetworkPolicy policy);
  }
}
