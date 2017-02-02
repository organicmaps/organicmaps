package com.mapswithme.util;

import android.content.Context;
import android.content.DialogInterface;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.StackedButtonsDialog;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.ref.WeakReference;
import java.util.concurrent.TimeUnit;

public final class NetworkPolicy
{
  public static final int ASK = 0;
  public static final int ALWAYS = 1;
  public static final int NEVER = 2;
  public static final int NOT_TODAY = 3;
  public static final int TODAY = 4;

  @NonNull
  private static WeakReference<StackedButtonsDialog> sDialog = new WeakReference<>(null);

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

    if (!ConnectionState.isInRoaming())
    {
      listener.onResult(new NetworkPolicy(true));
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
        showDialogIfNeeded(context, listener, new NetworkPolicy(false));
        break;
      case TODAY:
        showDialogIfNeeded(context, listener, new NetworkPolicy(true));
        break;
    }
  }

  private static void showDialogIfNeeded(@NonNull Context context,
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
    showDialog(context, listener);
  }

  private static void showDialog(@NonNull Context context, @NonNull final NetworkPolicyListener listener)
  {
    StackedButtonsDialog dialog = sDialog.get();
    if (dialog != null && dialog.isShowing())
      dialog.dismiss();

    dialog = new StackedButtonsDialog.Builder(context)
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
            Config.setMobileDataTimeStamp(System.currentTimeMillis());
            listener.onResult(new NetworkPolicy(false));
          }
        })
        .setNeutralButton(R.string.mobile_data_option_today, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            Config.setUseMobileDataSettings(TODAY);
            Config.setMobileDataTimeStamp(System.currentTimeMillis());
            listener.onResult(new NetworkPolicy(true));
          }
        })
        .build();
    dialog.show();
    sDialog = new WeakReference<>(dialog);
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
