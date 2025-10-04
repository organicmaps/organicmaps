package app.organicmaps.sync.nextcloud;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.widget.Toast;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import androidx.browser.customtabs.CustomTabsCallback;
import androidx.browser.customtabs.CustomTabsClient;
import androidx.browser.customtabs.CustomTabsIntent;
import androidx.browser.customtabs.CustomTabsServiceConnection;
import androidx.browser.customtabs.CustomTabsSession;
import app.organicmaps.R;
import app.organicmaps.sdk.sync.BackendType;
import app.organicmaps.sdk.sync.SyncManager;
import app.organicmaps.sdk.sync.nextcloud.NextcloudAuth;
import app.organicmaps.sdk.sync.preferences.AddAccountResult;
import app.organicmaps.sdk.sync.preferences.SyncPrefs;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import org.json.JSONException;
import org.json.JSONObject;

public class NextcloudLoginFlow
{
  public static final String TAG = NextcloudLoginFlow.class.getSimpleName();

  @MainThread
  public static void login(Context context)
  {
    new UrlInputDialog(context).show((loginUrl, pollParams) -> onReceiveParams(context, pollParams, loginUrl));
  }

  /**
   * @param pollParams The token and endpoint as specified in <a
   *     href="https://docs.nextcloud.com/server/latest/developer_manual/client_apis/LoginFlow/index.html">Nextcloud
   *     docs</a>
   * @param loginUrl The login url obtained in the initial login request.
   */
  @MainThread
  private static void onReceiveParams(Context context, PollParams pollParams, String loginUrl)
  {
    try
    {
      // CustomTabsCallback cannot be relied upon for some browsers,
      // Firefox being a notable example (https://bugzilla.mozilla.org/show_bug.cgi?id=1972417)
      // So this is needed as a fallback even in the case of Custom Tabs being available.
      SyncManager.INSTANCE.getPrefs().setNextcloudPollParams(pollParams.asString());
    }
    catch (JSONException e)
    {
      Logger.e(TAG, "Impossible error stringifying Nextcloud PollParams", e);
    }

    // Show the web login UI to the user (use Custom Tabs if available, else any installed Browser)
    String customTabsPackage = Utils.getCustomTabsPackage(context);
    if (customTabsPackage != null)
    {
      CustomTabsServiceConnection connection = new CustomTabsServiceConnection() {
        @Override
        public void onCustomTabsServiceConnected(@NonNull ComponentName name, CustomTabsClient client)
        {
          client.warmup(0L);

          CustomTabsCallback customTabsCallback = new CustomTabsCallback() {
            boolean mLoginComplete = false;
            @Override
            public void onNavigationEvent(int navigationEvent, @Nullable Bundle extras)
            {
              super.onNavigationEvent(navigationEvent, extras);
              if (!mLoginComplete && navigationEvent == CustomTabsCallback.NAVIGATION_FINISHED)
              {
                // The user might have finished authentication. Poll to check the same.
                ThreadPool.getWorker().execute(() -> {
                  if (storeAuthStateIfAvailable(context, pollParams))
                    mLoginComplete = true; // prevents unnecessary checks
                });
              }
            }
          };

          CustomTabsSession session = client.newSession(customTabsCallback);

          CustomTabsIntent customTabsIntent =
              new CustomTabsIntent
                  .Builder(session)
                  // Set initial custom tabs height to 90% of total display height
                  // Certain browsers like Firefox don't respect this though
                  .setInitialActivityHeightPx(UiUtils.getDisplayTotalHeight(context) * 9 / 10)
                  .build();

          customTabsIntent.intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
          customTabsIntent.launchUrl(context, Uri.parse(loginUrl));
        }

        @Override
        public void onServiceDisconnected(ComponentName name)
        {}
      };

      CustomTabsClient.bindCustomTabsService(context, customTabsPackage, connection);
    }
    else
    {
      Intent intent = new Intent(Intent.ACTION_VIEW)
                          .setData(Uri.parse(loginUrl))
                          .addCategory(Intent.CATEGORY_BROWSABLE)
                          .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      if (intent.resolveActivity(context.getPackageManager()) != null)
        context.startActivity(intent);
      else
      {
        Logger.w(TAG, "No app found to handle Nextcloud login url: " + loginUrl);
        Toast.makeText(context, R.string.browser_not_available, Toast.LENGTH_SHORT).show();
      }
    }
  }

  /**
   * Checks to see if authentication data is available on specified poll parameters.
   * @return {@code true} iff the the poll parameters was consumed, i.e. the server sent authentication state.
   */
  @WorkerThread
  public static boolean storeAuthStateIfAvailable(Context context, PollParams pollParams)
  {
    try
    {
      String pollResponse = PollRequest.make(pollParams);
      if (pollResponse == null)
        return false;

      JSONObject responseJson = new JSONObject(pollResponse);
      NextcloudAuth authState = new NextcloudAuth(responseJson);
      SyncPrefs syncPrefs = SyncManager.INSTANCE.getPrefs();
      AddAccountResult result = syncPrefs.addAccount(BackendType.Nextcloud, authState);
      syncPrefs.setNextcloudPollParams(null);
      if (context instanceof Activity)
      {
        Intent returnIntent = new Intent(context, context.getClass());
        context.startActivity(returnIntent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT));
      }
      UiThread.run(() -> {
        switch (result)
        {
        case Success -> Toast.makeText(context, R.string.account_connection_success, Toast.LENGTH_SHORT).show();
        case ReplacedExisting -> Toast.makeText(context, R.string.relogin_message, Toast.LENGTH_LONG).show();
        case UnexpectedError -> Toast.makeText(context, R.string.unexpected_error, Toast.LENGTH_LONG).show();
        }
      });
      return true;
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to poll Nextcloud auth status", e);
      return false;
    }
  }
}
