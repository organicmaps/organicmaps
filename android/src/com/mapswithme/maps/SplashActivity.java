package com.mapswithme.maps;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import com.mapswithme.maps.base.BaseActivity;
import com.mapswithme.maps.base.BaseActivityDelegate;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.Config;
import com.mapswithme.util.Counters;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.UiThread;

import java.io.IOException;

public class SplashActivity extends AppCompatActivity implements BaseActivity
{
  private static final String EXTRA_ACTIVITY_TO_START = "extra_activity_to_start";
  public static final String EXTRA_INITIAL_INTENT = "extra_initial_intent";
  private static final int REQUEST_PERMISSIONS = 1;
  private static final int REQ_CODE_API_RESULT = 10;

  private static final long DELAY = 100;

  private boolean mCanceled = false;

  @NonNull
  private final Runnable mInitCoreDelayedTask = new Runnable()
  {
    @Override
    public void run()
    {
      init();
    }
  };

  @NonNull
  private final BaseActivityDelegate mBaseDelegate = new BaseActivityDelegate(this);

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
    UiThread.cancelDelayedTasks(mInitCoreDelayedTask);
    Counters.initCounters(this);
    setContentView(R.layout.activity_splash);
  }

  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);
    mBaseDelegate.onNewIntent(intent);
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    mBaseDelegate.onStart();
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    mBaseDelegate.onResume();
    if (mCanceled)
      return;
    if (!Config.isLocationRequested() && !LocationUtils.isLocationGranted(this))
    {
      ActivityCompat.requestPermissions(this, new String[]{
          ACCESS_COARSE_LOCATION,
          ACCESS_FINE_LOCATION
      }, REQUEST_PERMISSIONS);
      return;
    }

    UiThread.runLater(mInitCoreDelayedTask, DELAY);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    mBaseDelegate.onPause();
    UiThread.cancelDelayedTasks(mInitCoreDelayedTask);
  }

  @Override
  protected void onStop()
  {
    super.onStop();
    mBaseDelegate.onStop();
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();
    mBaseDelegate.onDestroy();
  }

  private void showFatalErrorDialog(@StringRes int titleId, @StringRes int messageId)
  {
    mCanceled = true;
    new AlertDialog.Builder(this, R.style.MwmTheme_AlertDialog)
        .setTitle(titleId)
        .setMessage(messageId)
        .setNegativeButton(R.string.ok, (dialog, which) -> SplashActivity.this.finish())
        .setCancelable(false)
        .show();
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                         @NonNull int[] grantResults)
  {
    if (requestCode != REQUEST_PERMISSIONS)
      throw new AssertionError("Unexpected requestCode");
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    Config.setLocationRequested();
    // No-op here - onResume() calls init();
  }

  private void init()
  {
    MwmApplication app = MwmApplication.from(this);
    try
    {
      app.init();
    } catch (IOException e)
    {
      showFatalErrorDialog(R.string.dialog_error_storage_title, R.string.dialog_error_storage_message);
      return;
    }

    if (Counters.isFirstLaunch(this) && LocationUtils.isLocationGranted(this))
    {
      LocationHelper.INSTANCE.onEnteredIntoFirstRun();
      if (!LocationHelper.INSTANCE.isActive())
        LocationHelper.INSTANCE.start();
    }

    processNavigation();
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
      if (!initialIntent.hasCategory(Intent.CATEGORY_LAUNCHER))
      {
        // Wait for the result from MwmActivity for API callers.
        startActivityForResult(result, REQ_CODE_API_RESULT);
        return;
      }
    }
    Counters.setFirstStartDialogSeen(this);
    startActivity(result);
    finish();
  }

  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    if (requestCode == REQ_CODE_API_RESULT)
    {
      setResult(resultCode, data);
      finish();
    }
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
      return R.style.MwmTheme_Splash;

    if (ThemeUtils.isNightTheme(context, theme))
      return R.style.MwmTheme_Night_Splash;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }


}
