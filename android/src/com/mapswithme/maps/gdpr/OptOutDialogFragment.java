package com.mapswithme.maps.gdpr;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.widget.Toast;

import com.appsflyer.AppsFlyerLib;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;

public class OptOutDialogFragment extends BaseMwmDialogFragment
{
  public static final String ARGS_TITLE = "opt_out_title";
  public static final String ARGS_OPT_OUT_ACTIONS_FACTORY = "opt_out_actions_factory";
  public static final String TAG = OptOutDialogFragment.class.getCanonicalName();

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Bundle args = getArguments();

    if (args == null)
    {
      throw new IllegalArgumentException("Args can not be null");
    }

    String title = args.getString(ARGS_TITLE);
    String factoryName = args.getString(ARGS_OPT_OUT_ACTIONS_FACTORY);
    OptOutActionsFactory factory = OptOutActionsFactory.getOrThrow(factoryName);
    AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
    builder.setTitle(title)
           .setNegativeButton(android.R.string.cancel,
                              (dialog, which) -> onDataProcessingAllowed(factory))
           .setPositiveButton(android.R.string.ok,
                              (dialog, which) -> onDataProcessingRestricted(factory));
    return builder.create();
  }

  private void onDataProcessingAllowed(OptOutActionsFactory factory)
  {
  }

  private void onDataProcessingRestricted(OptOutActionsFactory factory)
  {
    factory.mPositiveBtnListener.onClick(getActivity().getApplication());
    Toast.makeText(getActivity(), android.R.string.yes, Toast.LENGTH_SHORT).show();
  }

  interface OnClickListener<T>
  {
    void onClick(T data);
  }

  public enum OptOutActionsFactory
  {
    APPSFLYER(
        new AppsFlyerListenerBase.PositiveBtnListener(),
        new AppsFlyerListenerBase.NegativeBtnListener()),
    FABRIC(new FabricPositiveBtnListener(),
           new FabricNegativeBtnListener());

    @NonNull
    private final Bundle mFragArgs;
    @NonNull
    private final OnClickListener<Context> mPositiveBtnListener;
    @NonNull
    private final OnClickListener<Context> mNegativeBtnListener;

    OptOutActionsFactory(@NonNull OnClickListener<Context> positiveBtnListener,
                         @NonNull OnClickListener<Context> negativeBtnListener)
    {
      mPositiveBtnListener = positiveBtnListener;
      mNegativeBtnListener = negativeBtnListener;
      mFragArgs = getBundle(name());
    }

    @NonNull
    private static Bundle getBundle(String name)
    {
      Bundle fragArgs = new Bundle();
      fragArgs.putString(ARGS_OPT_OUT_ACTIONS_FACTORY, name);
      /*FIXME*/
      fragArgs.putString(ARGS_TITLE, "my title");
      return fragArgs;
    }

    @NonNull
    public Bundle getFragArgs()
    {
      return mFragArgs;
    }

    @NonNull
    public static OptOutActionsFactory getOrThrow(String name)
    {
      for (OptOutActionsFactory each : values())
      {
        if (TextUtils.equals(each.name(), name))
        {
          return each;
        }
      }
      throw new IllegalArgumentException("Value not found for arg = " + name);
    }

    private static class AppsFlyerListenerBase implements OnClickListener<Context>
    {

      private final boolean mIsTrackingEnabled;

      private AppsFlyerListenerBase(boolean isTrackingEnabled)
      {
        mIsTrackingEnabled = isTrackingEnabled;
      }

      @Override
      public void onClick(Context data)
      {
        AppsFlyerLib.getInstance().stopTracking(mIsTrackingEnabled, data);
      }

      private static class PositiveBtnListener extends AppsFlyerListenerBase
      {

        private PositiveBtnListener()
        {
          super(false);
        }
      }

      private static class NegativeBtnListener extends AppsFlyerListenerBase
      {
        private NegativeBtnListener()
        {
          super(true);
        }
      }
    }

    private static class FabricPositiveBtnListener implements OnClickListener<Context>
    {
      @Override
      public void onClick(Context context)
      {
        MwmApplication app = (MwmApplication) context.getApplicationContext();
        app.deactivateCrashlytics();
      }

      /*FIXME*/
      private void logAnalytics(Context data)
      {

      }
    }

    private static class FabricNegativeBtnListener implements OnClickListener<Context>
    {
      @Override
      public void onClick(Context context)
      {
        MwmApplication app = (MwmApplication) context.getApplicationContext();
        SharedPreferences prefs = MwmApplication.prefs(context);
        String prefKey = context.getString(R.string.pref_opt_out_fabric_activated);
        prefs
            .edit()
            .putBoolean(prefKey, true)
            .apply();
        app.initCrashlytics();
      }
    }
  }
}
