package com.mapswithme.maps;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

import com.mapswithme.maps.editor.ViralFragment;
import com.mapswithme.maps.news.BaseNewsFragment;
import com.mapswithme.maps.news.NewsFragment;
import com.mapswithme.maps.news.WelcomeDialogFragment;
import com.mapswithme.maps.permissions.PermissionsDialogFragment;
import com.mapswithme.maps.permissions.StoragePermissionsDialogFragment;
import com.mapswithme.util.Config;
import com.mapswithme.util.Counters;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.statistics.PushwooshHelper;

public class SplashActivity extends AppCompatActivity
    implements BaseNewsFragment.NewsDialogListener
{
  public static final String EXTRA_INTENT = "extra_intent";
  private static final String EXTRA_ACTIVITY_TO_START = "extra_activity_to_start";
  private static final int REQUEST_PERMISSIONS = 1;
  private static final long FIRST_START_DELAY = 1000;
  private static final long DELAY = 100;

  // The first launch of application ever - onboarding screen will be shown.
  private static boolean sFirstStart;

  private View mIvLogo;

  private boolean mPermissionsGranted;
  private boolean mNeedStoragePermission;
  private boolean mCanceled;

  @Nullable
  private DialogFragment mPermissionsDialog;
  @Nullable
  private DialogFragment mStoragePermissionsDialog;

  @NonNull
  private final Runnable mPermissionsTask = new Runnable()
  {
    @Override
    public void run()
    {
      mPermissionsDialog = PermissionsDialogFragment.show(SplashActivity.this, REQUEST_PERMISSIONS);
    }
  };

  @NonNull
  private final Runnable mDelayedTask = new Runnable()
  {
    @Override
    public void run()
    {
      init();
//    Run delayed task because resumeDialogs() must see the actual value of mCanceled flag,
//    since onPause() callback can be blocked because of UI thread is busy with framework
//    initialization.
      UiThread.runLater(mFinalTask);
    }
  };
  @NonNull
  private final Runnable mFinalTask = new Runnable()
  {
    @Override
    public void run()
    {
      resumeDialogs();
    }
  };

  public static void start(@NonNull Context context,
                           @Nullable Class<? extends Activity> activityToStart)
  {
    Intent intent = new Intent(context, SplashActivity.class);
    intent.putExtra(EXTRA_ACTIVITY_TO_START, activityToStart);
    context.startActivity(intent);
  }

  public static boolean isFirstStart()
  {
    boolean res = sFirstStart;
    sFirstStart = false;
    return res;
  }

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    UiThread.cancelDelayedTasks(mPermissionsTask);
    UiThread.cancelDelayedTasks(mDelayedTask);
    UiThread.cancelDelayedTasks(mFinalTask);
    Counters.initCounters(this);
    initView();
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    mCanceled = false;
    mPermissionsGranted = PermissionsUtils.isExternalStorageGranted();
    if (!mPermissionsGranted)
    {
      mStoragePermissionsDialog = StoragePermissionsDialogFragment.find(this);
      if (mNeedStoragePermission || mStoragePermissionsDialog != null)
      {
        if (mPermissionsDialog != null)
        {
          mPermissionsDialog.dismiss();
          mPermissionsDialog = null;
        }
        if (mStoragePermissionsDialog == null)
          mStoragePermissionsDialog = StoragePermissionsDialogFragment.show(this);
        return;
      }
      mPermissionsDialog = PermissionsDialogFragment.find(this);
      if (mPermissionsDialog == null)
        UiThread.runLater(mPermissionsTask, FIRST_START_DELAY);

      return;
    }
    else
    {
      if (mPermissionsDialog != null)
      {
        mPermissionsDialog.dismiss();
        mPermissionsDialog = null;
      }
      if (mStoragePermissionsDialog != null)
      {
        mStoragePermissionsDialog.dismiss();
        mStoragePermissionsDialog = null;
      }
    }

    UiThread.runLater(mDelayedTask, DELAY);
  }

  @Override
  protected void onPause()
  {
    mCanceled = true;
    UiThread.cancelDelayedTasks(mPermissionsTask);
    UiThread.cancelDelayedTasks(mDelayedTask);
    UiThread.cancelDelayedTasks(mFinalTask);
    super.onPause();
  }

  private void resumeDialogs()
  {
    if (mCanceled)
      return;

    if (Counters.isMigrationNeeded())
    {
      Config.migrateCountersToSharedPrefs();
      Counters.setMigrationExecuted();
    }

    sFirstStart = WelcomeDialogFragment.showOn(this, this);
    if (sFirstStart)
    {
      PushwooshHelper.nativeProcessFirstLaunch();
      UiUtils.hide(mIvLogo);
      return;
    }

    boolean showNews = NewsFragment.showOn(this, this);
    if (!showNews)
    {
      if (ViralFragment.shouldDisplay())
      {
        UiUtils.hide(mIvLogo);
        ViralFragment dialog = new ViralFragment();
        dialog.onDismiss(new DialogInterface()
        {
          @Override
          public void cancel()
          {
            onDialogDone();
          }

          @Override
          public void dismiss()
          {
            onDialogDone();
          }
        });
        dialog.show(getSupportFragmentManager(), "");
      }
      else
      {
        processNavigation();
      }
    }
    else
    {
      UiUtils.hide(mIvLogo);
    }
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                         @NonNull int[] grantResults)
  {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    if (grantResults.length == 0)
      return;

    mPermissionsGranted = PermissionsUtils.computePermissionsResult(permissions, grantResults)
                                          .isExternalStorageGranted();
    mNeedStoragePermission = !mPermissionsGranted;
  }

  @Override
  public void onDialogDone()
  {
    processNavigation();
  }

  private void initView()
  {
    UiUtils.setupStatusBar(this);
    setContentView(R.layout.activity_splash);
    mIvLogo = findViewById(R.id.iv__logo);
  }

  private void init()
  {
    MwmApplication.get().initNativePlatform();
    MwmApplication.get().initNativeCore();
  }

  @SuppressWarnings("unchecked")
  private void processNavigation()
  {
    Intent input = getIntent();
    Intent intent = new Intent(this, DownloadResourcesActivity.class);
    if (input != null)
    {
      Class<? extends Activity> type = (Class<? extends Activity>) input.getSerializableExtra(EXTRA_ACTIVITY_TO_START);
      if (type != null)
        intent = new Intent(this, type);
      intent.putExtra(EXTRA_INTENT, input);
    }
    startActivity(intent);
    finish();
  }
}
