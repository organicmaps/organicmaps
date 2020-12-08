package com.mapswithme.maps;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentManager;
import com.mapswithme.maps.ads.Banner;
import com.mapswithme.maps.analytics.AdvertisingObserver;
import com.mapswithme.maps.analytics.ExternalLibrariesMediator;
import com.mapswithme.maps.base.BaseActivity;
import com.mapswithme.maps.base.BaseActivityDelegate;
import com.mapswithme.maps.base.Detachable;
import com.mapswithme.maps.downloader.UpdaterDialogFragment;
import com.mapswithme.maps.editor.ViralFragment;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.news.OnboardingStep;
import com.mapswithme.maps.onboarding.BaseNewsFragment;
import com.mapswithme.maps.onboarding.NewsFragment;
import com.mapswithme.maps.onboarding.WelcomeDialogFragment;
import com.mapswithme.maps.permissions.PermissionsDialogFragment;
import com.mapswithme.maps.permissions.StoragePermissionsDialogFragment;
import com.mapswithme.util.Config;
import com.mapswithme.util.Counters;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.PushwooshHelper;

public class SplashActivity extends AppCompatActivity
    implements BaseNewsFragment.NewsDialogListener, BaseActivity,
               WelcomeDialogFragment.PolicyAgreementListener,
               WelcomeDialogFragment.OnboardingStepPassedListener
{
  private static final String EXTRA_CURRENT_ONBOARDING_STEP = "extra_current_onboarding_step";
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = SplashActivity.class.getSimpleName();
  private static final String EXTRA_ACTIVITY_TO_START = "extra_activity_to_start";
  public static final String EXTRA_INITIAL_INTENT = "extra_initial_intent";
  private static final int REQUEST_PERMISSIONS = 1;
  private static final long FIRST_START_DELAY = 300;
  private static final long DELAY = 100;

  private View mIvLogo;
  private View mAppName;

  private boolean mPermissionsGranted;
  private boolean mNeedStoragePermission;
  private boolean mCanceled;
  private boolean mWaitForAdvertisingInfo;

  @NonNull
  private final Runnable mUserAgreementDelayedTask = new Runnable()
  {
    @Override
    public void run()
    {
      WelcomeDialogFragment.show(SplashActivity.this);
    }
  };

  @NonNull
  private Runnable mOnboardingStepsTask = new Runnable()
  {
    @Override
    public void run()
    {
      if (mCurrentOnboardingStep != null)
      {
        WelcomeDialogFragment.showOnboardinStepsStartWith(SplashActivity.this,
                                                          mCurrentOnboardingStep);
        return;
      }

      WelcomeDialogFragment.showOnboardinSteps(SplashActivity.this);
    }
  };

  @NonNull
  private final Runnable mPermissionsDelayedTask = new Runnable()
  {
    @Override
    public void run()
    {
      PermissionsDialogFragment.show(SplashActivity.this, REQUEST_PERMISSIONS);
    }
  };

  @NonNull
  private final Runnable mInitCoreDelayedTask = new Runnable()
  {
    @Override
    public void run()
    {
      MwmApplication app = (MwmApplication) getApplication();
      if (app.arePlatformAndCoreInitialized())
      {
        UiThread.runLater(mFinalDelayedTask);
        return;
      }

      ExternalLibrariesMediator mediator = MwmApplication.from(getApplicationContext()).getMediator();
      if (!mediator.isAdvertisingInfoObtained())
      {
        LOGGER.i(TAG, "Advertising info not obtained yet, wait...");
        mWaitForAdvertisingInfo = true;
        return;
      }

      mWaitForAdvertisingInfo = false;

      if (!mediator.isLimitAdTrackingEnabled())
      {
        LOGGER.i(TAG, "Limit ad tracking disabled, sensitive tracking initialized");
        mediator.initSensitiveData();
      }
      else
      {
        LOGGER.i(TAG, "Limit ad tracking enabled, sensitive tracking not initialized");
      }

      init();
      LOGGER.i(TAG, "Core initialized: " + app.arePlatformAndCoreInitialized());
      if (app.arePlatformAndCoreInitialized() && mediator.isLimitAdTrackingEnabled())
      {
        LOGGER.i(TAG, "Limit ad tracking enabled, rb banners disabled.");
        mediator.disableAdProvider(Banner.Type.TYPE_RB);
      }

//    Run delayed task because resumeDialogs() must see the actual value of mCanceled flag,
//    since onPause() callback can be blocked because of UI thread is busy with framework
//    initialization.
      UiThread.runLater(mFinalDelayedTask);
    }
  };

  @NonNull
  private final Runnable mFinalDelayedTask = new Runnable()
  {
    @Override
    public void run()
    {
      resumeDialogs();
    }
  };

  @NonNull
  private final BaseActivityDelegate mBaseDelegate = new BaseActivityDelegate(this);

  @NonNull
  private final AdvertisingInfoObserver mAdvertisingObserver = new AdvertisingInfoObserver();
  @Nullable
  private OnboardingStep mCurrentOnboardingStep;

  public static void start(@NonNull Context context,
                           @Nullable Class<? extends Activity> activityToStart,
                           @Nullable Intent initialIntent)
  {
    Intent intent = new Intent(context, SplashActivity.class);
    if (activityToStart != null)
      intent.putExtra(EXTRA_ACTIVITY_TO_START, activityToStart);
    if (initialIntent != null)
      intent.putExtra(EXTRA_INITIAL_INTENT, initialIntent);
    context.startActivity(intent);
  }

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mBaseDelegate.onCreate();
    handleOnboardingStep(savedInstanceState);
    handleUpdateMapsFragmentCorrectly(savedInstanceState);
    UiThread.cancelDelayedTasks(mUserAgreementDelayedTask);
    UiThread.cancelDelayedTasks(mOnboardingStepsTask);
    UiThread.cancelDelayedTasks(mPermissionsDelayedTask);
    UiThread.cancelDelayedTasks(mInitCoreDelayedTask);
    UiThread.cancelDelayedTasks(mFinalDelayedTask);
    Counters.initCounters(this);
    initView();
  }

  private void handleOnboardingStep(@Nullable Bundle savedInstanceState)
  {
    if (savedInstanceState == null)
      return;

    if (!savedInstanceState.containsKey(EXTRA_CURRENT_ONBOARDING_STEP))
      return;

    int step = savedInstanceState.getInt(EXTRA_CURRENT_ONBOARDING_STEP);
    mCurrentOnboardingStep = OnboardingStep.values()[step];
  }

  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);
    mBaseDelegate.onNewIntent(intent);
  }

  private void handleUpdateMapsFragmentCorrectly(@Nullable Bundle savedInstanceState)
  {
    if (savedInstanceState == null)
      return;

    FragmentManager fm = getSupportFragmentManager();
    DialogFragment updaterFragment = (DialogFragment) fm
        .findFragmentByTag(UpdaterDialogFragment.class.getName());

    if (updaterFragment == null)
      return;

    // If the user revoked the external storage permission while the app was killed
    // we can't update maps automatically during recovering process, so just dismiss updater fragment
    // and ask the user to grant the permission.
    if (!PermissionsUtils.isExternalStorageGranted(this))
    {
      fm.beginTransaction().remove(updaterFragment).commitAllowingStateLoss();
      fm.executePendingTransactions();
      StoragePermissionsDialogFragment.show(this);
    }
    else
    {
      // If external permissions are still granted we just need to check platform
      // and core initialization, because we may be in the recovering process,
      // i.e. method onResume() may not be invoked in that case.
      if (!MwmApplication.from(getApplicationContext()).arePlatformAndCoreInitialized())
      {
        init();
      }
    }
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    mBaseDelegate.onStart();
    mAdvertisingObserver.attach(this);
    ExternalLibrariesMediator mediator = MwmApplication.from(this).getMediator();
    LOGGER.d(TAG, "Add advertising observer");
    mediator.addAdvertisingObserver(mAdvertisingObserver);
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    mBaseDelegate.onResume();
    mCanceled = false;

    Context context = getApplicationContext();
    if (Counters.isMigrationNeeded(context))
    {
      Config.migrateCountersToSharedPrefs(context);
      Counters.setMigrationExecuted(context);
    }
    
    final boolean isFirstLaunch = WelcomeDialogFragment.isFirstLaunch(this);
    if (isFirstLaunch)
      MwmApplication.from(this).setFirstLaunch(true);

    boolean isWelcomeFragmentOnScreen = false;
    DialogFragment welcomeFragment = WelcomeDialogFragment.find(this);
    if (welcomeFragment != null)
    {
      isWelcomeFragmentOnScreen = true;
      welcomeFragment.dismissAllowingStateLoss();
    }

    if (isFirstLaunch || isWelcomeFragmentOnScreen)
    {
      if (WelcomeDialogFragment.isAgreementDeclined(this))
      {
        UiThread.runLater(mUserAgreementDelayedTask, FIRST_START_DELAY);
        return;
      }
      else
      {
        if (processPermissionGranting())
        {
          UiThread.runLater(mOnboardingStepsTask, DELAY);
          return;
        }
      }
    }

    if (processPermissionGranting())
      runInitCoreTask();
  }

  private boolean processPermissionGranting()
  {
    mPermissionsGranted = PermissionsUtils.isExternalStorageGranted(this);
    DialogFragment storagePermissionsDialog = StoragePermissionsDialogFragment.find(this);
    DialogFragment permissionsDialog = PermissionsDialogFragment.find(this);
    if (!mPermissionsGranted)
    {
      if (mNeedStoragePermission || storagePermissionsDialog != null)
      {
        if (permissionsDialog != null)
          permissionsDialog.dismiss();
        if (storagePermissionsDialog == null)
          StoragePermissionsDialogFragment.show(this);
        return false;
      }
      permissionsDialog = PermissionsDialogFragment.find(this);
      if (permissionsDialog == null)
        UiThread.runLater(mPermissionsDelayedTask, DELAY);

      return false;
    }

    if (permissionsDialog != null)
      permissionsDialog.dismiss();

    if (storagePermissionsDialog != null)
      storagePermissionsDialog.dismiss();

    return true;
  }

  private void runInitCoreTask()
  {
    UiThread.runLater(mInitCoreDelayedTask, DELAY);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    mBaseDelegate.onPause();
    mCanceled = true;
    UiThread.cancelDelayedTasks(mUserAgreementDelayedTask);
    UiThread.cancelDelayedTasks(mOnboardingStepsTask);
    UiThread.cancelDelayedTasks(mPermissionsDelayedTask);
    UiThread.cancelDelayedTasks(mInitCoreDelayedTask);
    UiThread.cancelDelayedTasks(mFinalDelayedTask);
  }

  @Override
  protected void onStop()
  {
    super.onStop();
    mBaseDelegate.onStop();
    mAdvertisingObserver.detach();
    ExternalLibrariesMediator mediator = MwmApplication.from(this).getMediator();
    LOGGER.d(TAG, "Remove advertising observer");
    mediator.removeAdvertisingObserver(mAdvertisingObserver);
  }

  @Override
  protected void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    if (mCurrentOnboardingStep != null)
      outState.putInt(EXTRA_CURRENT_ONBOARDING_STEP, mCurrentOnboardingStep.ordinal());
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();
    mBaseDelegate.onDestroy();
  }

  private void resumeDialogs()
  {
    if (mCanceled)
      return;

    MwmApplication app = (MwmApplication) getApplication();
    if (!app.arePlatformAndCoreInitialized())
    {
      showExternalStorageErrorDialog();
      return;
    }

    boolean showNews = NewsFragment.showOn(this, this);
    if (!showNews)
    {
      if (ViralFragment.shouldDisplay(getApplicationContext()))
      {
        UiUtils.hide(mIvLogo, mAppName);
        ViralFragment dialog = new ViralFragment();
        dialog.onDismissListener(new Runnable()
        {
          @Override
          public void run()
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
      UiUtils.hide(mIvLogo, mAppName);
    }
  }

  private void showExternalStorageErrorDialog()
  {
    AlertDialog dialog = new AlertDialog.Builder(this)
        .setTitle(R.string.dialog_error_storage_title)
        .setMessage(R.string.dialog_error_storage_message)
        .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            SplashActivity.this.finish();
          }
        })
        .setCancelable(false)
        .create();
    dialog.show();
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

  @Override
  public void onPolicyAgreementApplied()
  {
    boolean permissionsGranted = processPermissionGranting();
    if (!permissionsGranted)
      return;

    boolean isFirstLaunch = WelcomeDialogFragment.isFirstLaunch(this);
    if (isFirstLaunch)
    {
      UiThread.runLater(mOnboardingStepsTask, DELAY);
      return;
    }

    runInitCoreTask();
  }

  @Override
  public void onLastOnboardingStepPassed()
  {
    runInitCoreTask();
  }

  @Override
  public void onOnboardingStepCancelled()
  {
    finish();
  }

  @Override
  public void onOnboardingStepPassed(@NonNull OnboardingStep step)
  {
    mCurrentOnboardingStep = step;
  }

  private void initView()
  {
    UiUtils.setupStatusBar(this);
    setContentView(R.layout.activity_splash);
    mIvLogo = findViewById(R.id.iv__logo);
    mAppName = findViewById(R.id.tv__app_name);
  }

  private void init()
  {
    MwmApplication app = MwmApplication.from(this);
    boolean success = app.initCore();
    if (!success || !app.isFirstLaunch())
      return;

    PushwooshHelper.nativeProcessFirstLaunch();
    LocationHelper.INSTANCE.onEnteredIntoFirstRun();
    if (!LocationHelper.INSTANCE.isActive())
      LocationHelper.INSTANCE.start();
  }

  @SuppressWarnings("unchecked")
  private void processNavigation()
  {
    Intent input = getIntent();
    Intent result = new Intent(this, DownloadResourcesLegacyActivity.class);
    if (input != null)
    {
      if (input.hasExtra(EXTRA_ACTIVITY_TO_START))
      {
        result = new Intent(this,
                            (Class<? extends Activity>) input.getSerializableExtra(EXTRA_ACTIVITY_TO_START));
      }

      Intent initialIntent = input.hasExtra(EXTRA_INITIAL_INTENT) ?
                           input.getParcelableExtra(EXTRA_INITIAL_INTENT) :
                           input;
      result.putExtra(EXTRA_INITIAL_INTENT, initialIntent);
    }
    startActivity(result);
    finish();
  }

  @Override
  @NonNull
  public Activity get()
  {
    return this;
  }

  @Override
  public int getThemeResourceId(@NonNull String theme)
  {
    Context context = getApplicationContext();
    if (ThemeUtils.isDefaultTheme(context, theme))
      return R.style.MwmTheme;

    if (ThemeUtils.isNightTheme(context, theme))
      return R.style.MwmTheme_Night;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }

  boolean isWaitForAdvertisingInfo()
  {
    return mWaitForAdvertisingInfo;
  }

  private static class AdvertisingInfoObserver implements AdvertisingObserver,
                                                          Detachable<SplashActivity>
  {
    @Nullable
    private SplashActivity mActivity;

    @Override
    public void onAdvertisingInfoObtained()
    {
      LOGGER.i(TAG, "Advertising info obtained");
      if (mActivity == null)
        return;

      if (!mActivity.isWaitForAdvertisingInfo())
      {
        LOGGER.i(TAG, "Advertising info not waited");
        return;
      }

      mActivity.runInitCoreTask();
    }

    @Override
    public void attach(@NonNull SplashActivity object)
    {
      mActivity = object;
    }

    @Override
    public void detach()
    {
      mActivity = null;
    }
  }
}
