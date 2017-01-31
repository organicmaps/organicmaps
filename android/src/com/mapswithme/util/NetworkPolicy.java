package com.mapswithme.util;

import android.content.Context;
import android.content.DialogInterface;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;

import com.mapswithme.maps.R;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.concurrent.TimeUnit;

public final class NetworkPolicy
{
  public static final int ASK = 0;
  public static final int ALWAYS = 1;
  public static final int NEVER = 2;
  public static final int NOT_TODAY = 3;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ ASK, ALWAYS, NEVER, NOT_TODAY})
  @interface NetworkPolicyDef {}

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
        long timestamp = Config.getNotTodayTimeStamp();
        boolean showDialog = TimeUnit.MILLISECONDS.toDays(System.currentTimeMillis() - timestamp) >= 1;
        if (!showDialog)
        {
          listener.onResult(new NetworkPolicy(false));
          return;
        }
        showDialog(context, listener);
        break;
    }
  }

  private static void showDialog(@NonNull Context context, @NonNull final NetworkPolicyListener listener)
  {
    AlertDialog alertDialog = new AlertDialog.Builder(context).create();
    alertDialog.setTitle(R.string.mobile_data_dialog);
    alertDialog.setMessage(context.getString(R.string.mobile_data_description));
    alertDialog.setCancelable(false);
    alertDialog.setButton(AlertDialog.BUTTON_NEGATIVE,
                          context.getString(R.string.mobile_data_option_not_today),
                          new DialogInterface.OnClickListener()
                          {
                            @Override
                            public void onClick(DialogInterface dialog, int which)
                            {
                              Config.setUseMobileDataSettings(NOT_TODAY);
                              Config.setNotTodayStamp(System.currentTimeMillis());
                              listener.onResult(new NetworkPolicy(false));
                            }
                          });
    alertDialog.setButton(AlertDialog.BUTTON_POSITIVE,
                          context.getString(R.string.mobile_data_option_always),
                          new DialogInterface.OnClickListener()
                          {
                            @Override
                            public void onClick(DialogInterface dialog, int which)
                            {
                              Config.setUseMobileDataSettings(ALWAYS);
                              listener.onResult(new NetworkPolicy(true));
                            }
                          });
    alertDialog.show();
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
