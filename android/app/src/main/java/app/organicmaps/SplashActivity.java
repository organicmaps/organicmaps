package app.organicmaps;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.OnApplyWindowInsetsListener;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import app.organicmaps.downloader.DownloaderActivity;
import app.organicmaps.intent.Factory;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.LocationUtils;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.util.SharingUtils;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.Utils;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.io.IOException;
import java.util.Objects;

public class SplashActivity extends AppCompatActivity
{
  private static final String TAG = SplashActivity.class.getSimpleName();

  private static final long DELAY = 100;

  private boolean mCanceled = false;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private ActivityResultLauncher<Intent> mApiRequest;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private ActivityResultLauncher<String[]> mPermissionRequest;
  @NonNull
  private ActivityResultLauncher<SharingUtils.SharingIntent> mShareLauncher;

  @NonNull
  private final Runnable mInitCoreDelayedTask = this::init;

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    final String theme = Config.UiTheme.getCurrent();
    if (Config.UiTheme.isDefault(theme))
      setTheme(R.style.MwmTheme_Splash);
    else if (Config.UiTheme.isNight(theme))
      setTheme(R.style.MwmTheme_Night_Splash);
    else
      throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);

    UiThread.cancelDelayedTasks(mInitCoreDelayedTask);
    setContentView(R.layout.activity_splash);

    ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.root_view), new OnApplyWindowInsetsListener() {
      @NonNull
      @Override
      public WindowInsetsCompat onApplyWindowInsets(@NonNull View v, @NonNull WindowInsetsCompat insets)
      {
        Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
        v.setPadding(0, 0, 0, systemBars.bottom);
        return insets;
      }
    });
    mPermissionRequest = registerForActivityResult(new ActivityResultContracts.RequestMultiplePermissions(),
                                                   result -> Config.setLocationRequested());
    mApiRequest = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
      setResult(result.getResultCode(), result.getData());
      finish();
    });
    mShareLauncher = SharingUtils.RegisterLauncher(this);

    if (MwmApplication.from(this).getDisplayManager().isCarDisplayUsed())
    {
      startActivity(new Intent(this, MapPlaceholderActivity.class));
      finish();
    }
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    if (mCanceled)
      return;
    if (!Config.isLocationRequested() && !LocationUtils.checkLocationPermission(this))
    {
      Logger.d(TAG, "Requesting location permissions");
      mPermissionRequest.launch(new String[] {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION});
      return;
    }

    UiThread.runLater(mInitCoreDelayedTask, DELAY);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    UiThread.cancelDelayedTasks(mInitCoreDelayedTask);
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();
    mPermissionRequest.unregister();
    mPermissionRequest = null;
    mApiRequest.unregister();
    mApiRequest = null;
  }

  private void showFatalErrorDialog(@StringRes int titleId, @StringRes int messageId, Exception error)
  {
    mCanceled = true;
    new MaterialAlertDialogBuilder(this, R.style.MwmTheme_AlertDialog)
        .setTitle(titleId)
        .setMessage(messageId)
        .setPositiveButton(
            R.string.report_a_bug,
            (dialog, which) -> Utils.sendBugReport(mShareLauncher, this, "Fatal Error", Log.getStackTraceString(error)))
        .setCancelable(false)
        .show();
  }

  private void init()
  {
    MwmApplication app = MwmApplication.from(this);
    boolean asyncContinue = false;
    try
    {
      asyncContinue = app.initOrganicMaps(this::processNavigation);
    }
    catch (IOException error)
    {
      showFatalErrorDialog(R.string.dialog_error_storage_title, R.string.dialog_error_storage_message, error);
      return;
    }

    if (Config.isFirstLaunch(this) && LocationUtils.checkLocationPermission(this))
    {
      final LocationHelper locationHelper = app.getLocationHelper();
      locationHelper.onEnteredIntoFirstRun();
      if (!locationHelper.isActive())
        locationHelper.start();
    }

    if (!asyncContinue)
      processNavigation();
  }

  // Called from MwmApplication::nativeInitFramework like callback.
  @Keep
  @SuppressWarnings({"unused", "unchecked"})
  public void processNavigation()
  {
    if (isDestroyed())
    {
      Logger.w(TAG, "Ignore late callback from core because activity is already destroyed");
      return;
    }

    // Re-use original intent with the known safe subset of flags to retain security permissions.
    // https://github.com/organicmaps/organicmaps/issues/6944
    final Intent intent = Objects.requireNonNull(getIntent());

    if (isManageSpaceActivity(intent))
    {
      intent.setComponent(new ComponentName(this, DownloaderActivity.class));
    }
    else
    {
      intent.setComponent(new ComponentName(this, DownloadResourcesLegacyActivity.class));
    }

    // FLAG_ACTIVITY_NEW_TASK and FLAG_ACTIVITY_RESET_TASK_IF_NEEDED break the cold start.
    // https://github.com/organicmaps/organicmaps/pull/7287
    // FORWARD_RESULT_FLAG conflicts with the ActivityResultLauncher.
    // https://github.com/organicmaps/organicmaps/issues/8984
    intent.setFlags(intent.getFlags() & Intent.FLAG_GRANT_READ_URI_PERMISSION);

    if (Factory.isStartedForApiResult(intent))
    {
      // Wait for the result from MwmActivity for API callers.
      mApiRequest.launch(intent);
      return;
    }

    Config.setFirstStartDialogSeen(this);
    startActivity(intent);
    finish();
  }

  private boolean isManageSpaceActivity(Intent intent)
  {
    var component = intent.getComponent();

    if (!Intent.ACTION_VIEW.equals(intent.getAction()))
      return false;
    if (component == null)
      return false;

    var manageSpaceActivityName = BuildConfig.APPLICATION_ID + ".ManageSpaceActivity";

    return manageSpaceActivityName.equals(component.getClassName());
  }
}
