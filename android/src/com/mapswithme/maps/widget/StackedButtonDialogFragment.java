package com.mapswithme.maps.widget;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;

import com.mapswithme.maps.R;
import com.mapswithme.util.Config;
import com.mapswithme.util.NetworkPolicy;

public class StackedButtonDialogFragment extends DialogFragment
{

  @Nullable
  private NetworkPolicy.NetworkPolicyListener mListener;

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    return new StackedButtonsDialog.Builder(getContext())
        .setTitle(R.string.mobile_data_dialog)
        .setMessage(R.string.mobile_data_description)
        .setCancelable(false)
        .setPositiveButton(R.string.mobile_data_option_always, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            Config.setUseMobileDataSettings(NetworkPolicy.ALWAYS);
            if (mListener != null)
              mListener.onResult(NetworkPolicy.newInstance(true));
          }
        })
        .setNegativeButton(R.string.mobile_data_option_not_today, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            Config.setUseMobileDataSettings(NetworkPolicy.NOT_TODAY);
            Config.setMobileDataTimeStamp(System.currentTimeMillis());
            if (mListener != null)
              mListener.onResult(NetworkPolicy.newInstance(false));
          }
        })
        .setNeutralButton(R.string.mobile_data_option_today, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            Config.setUseMobileDataSettings(NetworkPolicy.TODAY);
            Config.setMobileDataTimeStamp(System.currentTimeMillis());
            if (mListener != null)
              mListener.onResult(NetworkPolicy.newInstance(true));
          }
        })
        .build();
  }

  @Override
  public void show(@NonNull FragmentManager manager, @NonNull String tag)
  {
    FragmentTransaction ft = manager.beginTransaction();
    ft.add(this, tag);
    ft.commitAllowingStateLoss();
  }

  public void setListener(@Nullable NetworkPolicy.NetworkPolicyListener listener)
  {
    mListener = listener;
  }
}
