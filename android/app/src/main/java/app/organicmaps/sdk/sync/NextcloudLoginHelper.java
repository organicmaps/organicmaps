package app.organicmaps.sdk.sync;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.TypedValue;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import androidx.appcompat.app.AlertDialog;
import androidx.browser.customtabs.CustomTabsCallback;
import androidx.browser.customtabs.CustomTabsClient;
import androidx.browser.customtabs.CustomTabsIntent;
import androidx.browser.customtabs.CustomTabsServiceConnection;
import androidx.browser.customtabs.CustomTabsSession;
import app.organicmaps.R;
import app.organicmaps.sdk.util.InsecureHttpsHelper;
import app.organicmaps.sdk.util.Utils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.log.Logger;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import org.json.JSONException;
import org.json.JSONObject;

public class NextcloudLoginHelper
{
  public static final String TAG = NextcloudLoginHelper.class.getSimpleName();

  /// @param context must be an Activity context.
  @MainThread
  public static void login(Context context)
  {
    if (!(context instanceof Activity))
      throw new IllegalArgumentException("context argument must be an Activity context");

    // This theme is needed because using R.style.MwmTheme_AlertDialog causes a blank
    // underlay to appear below the copy/paste/select menu on the EditText, probably due to having
    // a background defined.
    TypedValue resolvedId = new TypedValue();
    boolean foundTheme = context.getTheme().resolveAttribute(R.attr.alertDialogThemeWide, resolvedId, true);
    int themeResId = foundTheme ? resolvedId.data : R.style.MwmTheme_AlertDialog;

    final AlertDialog dialog =
        new MaterialAlertDialogBuilder(context, themeResId)
            .setTitle(context.getString(R.string.enter_nextcloud_url))
            .setNegativeButton(R.string.cancel, (dialogInterface, i) -> dialogInterface.dismiss())
            .setPositiveButton(R.string.next_button, null)
            .create();

    dialog.setView(View.inflate(context, R.layout.item_url_input, null));
    dialog.setOnShowListener(dialogInterface -> {
      Button positiveButton = dialog.getButton(AlertDialog.BUTTON_POSITIVE);
      positiveButton.setOnClickListener(view -> {
        final EditText editText = dialog.findViewById(R.id.et_url_input);
        final ProgressBar progressBar = dialog.findViewById(R.id.pb_url_input);
        final TextView errorTv = dialog.findViewById(R.id.tv_url_input_error);
        final Button btnNext = dialog.getButton(AlertDialog.BUTTON_POSITIVE);
        final String input = editText.getText().toString();

        final String initLoginUrl;
        try
        {
          // Prepend "https://" if scheme (https://datatracker.ietf.org/doc/html/rfc3986#section-3.1) not specified
          String inputUrl = input.matches("^[a-zA-Z][a-zA-Z0-9+.-]*://.*") ? input : "https://" + input;
          final URL url = new URL(inputUrl);
          String baseUrl = url.getProtocol() + "://" + url.getHost();
          int port = url.getPort();
          if (port != -1)
            baseUrl += ":" + port;
          initLoginUrl = baseUrl + "/index.php/login/v2";
        }
        catch (MalformedURLException e)
        {
          errorTv.setText(R.string.hint_enter_valid_url);
          errorTv.setVisibility(View.VISIBLE);
          return;
        }

        errorTv.setVisibility(View.GONE);
        progressBar.setVisibility(View.VISIBLE);
        editText.setEnabled(false);
        btnNext.setEnabled(false);

        ThreadPool.getWorker().execute(() -> {
          HttpURLConnection connection = null;
          try
          {
            connection = InsecureHttpsHelper.openInsecureConnection(initLoginUrl);
            connection.setInstanceFollowRedirects(true);
            connection.setRequestMethod("POST");
            connection.setRequestProperty("Content-Type", "application/json");
            connection.setRequestProperty("Accept", "application/json");
            connection.setRequestProperty("User-Agent", context.getString(R.string.app_name));
            connection.setDoOutput(true);
            final int responseCode = connection.getResponseCode();
            if (responseCode / 100 != 2)
              throw new Exception(context.getString(responseCode == 404 ? R.string.make_sure_url_is_nextcloud
                                                                        : R.string.error_connecting_to_server));
            try (BufferedReader in = new BufferedReader(new InputStreamReader(connection.getInputStream())))
            {
              String inputLine;
              StringBuilder response = new StringBuilder();
              while ((inputLine = in.readLine()) != null)
                response.append(inputLine);
              try
              {
                // success response format: { "poll": {"token": string, "endpoint": string }, "login": string }
                final JSONObject responseJson = new JSONObject(response.toString());
                PollParams params = PollParams.fromJson(responseJson.getJSONObject("poll"));
                String loginUrl = responseJson.getString("login");

                new Handler(Looper.getMainLooper()).post(() -> {
                  dialog.dismiss();
                  loginFlowStepTwo(context, params, loginUrl);
                });
              }
              catch (JSONException e)
              {
                throw new Exception(context.getString(R.string.unrecognized_server_response) + response);
              }
            }
          }
          catch (Exception e)
          {
            Logger.e(TAG, "Error trying to initiate connection with the server.", e);
            new Handler(Looper.getMainLooper()).post(() -> {
              errorTv.setText(e.getLocalizedMessage());
              errorTv.setVisibility(View.VISIBLE);
            });
          }
          finally
          {
            new Handler(Looper.getMainLooper()).post(() -> {
              progressBar.setVisibility(View.GONE);
              editText.setEnabled(true);
              btnNext.setEnabled(true);
            });
            if (connection != null)
              connection.disconnect();
          }
        });
      });
    });

    dialog.show();
  }

  @MainThread
  private static void loginFlowStepTwo(Context context, PollParams pollParams, String loginUrl)
  {
    try
    {
      // CustomTabsCallback cannot be relied upon for some browsers,
      // Firefox being a notable example (https://bugzilla.mozilla.org/show_bug.cgi?id=1972417)
      // So this is needed as a fallback even in the case of Custom Tabs being available.
      SyncPrefs.getInstance(context).setNextcloudPollParams(pollParams.asString());
    }
    catch (JSONException e)
    {
      Logger.e(TAG, "Impossible error stringifying Nextcloud PollParams", e);
    }
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
                  .setInitialActivityHeightPx(((Activity) context).getWindow().getDecorView().getHeight() * 9 / 10)
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

  private static void showErrorDialog(Context context, String errorMessage)
  {
    new Handler(Looper.getMainLooper())
        .post(()
                  -> new MaterialAlertDialogBuilder(context, R.style.MwmTheme_AlertDialog)
                         .setTitle(R.string.error_adding_account)
                         .setMessage(errorMessage)
                         .setNegativeButton(R.string.ok, null)
                         .show());
  }

  /**
   * @return {@code true} iff the the poll was consumed, i.e. the server sent authentication state.
   * Does not guarantee that the account will get added.
   */
  @WorkerThread
  public static boolean storeAuthStateIfAvailable(Context context, PollParams pollParams)
  {
    try
    {
      HttpURLConnection connection = InsecureHttpsHelper.openInsecureConnection(pollParams.endpoint);
      connection.setRequestMethod("POST");
      connection.setRequestProperty("Accept", "application/json");
      connection.setDoOutput(true);
      try (OutputStream os = connection.getOutputStream())
      {
        byte[] body = ("token=" + pollParams.token).getBytes(StandardCharsets.UTF_8);
        os.write(body, 0, body.length);
      }
      final int responseCode = connection.getResponseCode();
      if (responseCode / 100 != 2)
        return false;
      StringBuilder response = new StringBuilder();
      try (BufferedReader in = new BufferedReader(new InputStreamReader(connection.getInputStream())))
      {
        String inputLine;
        while ((inputLine = in.readLine()) != null)
          response.append(inputLine);
      }
      if (response.toString().isEmpty())
        return false;
      // success response format: { "server": string, "loginName": string, "appPassword": string }
      JSONObject responseJson = new JSONObject(response.toString());
      NextcloudAuth authState = new NextcloudAuth(responseJson);
      SyncPrefs syncPrefs = SyncPrefs.getInstance(context);
      SyncPrefs.AddAccountResult result = syncPrefs.addAccount(BackendType.Nextcloud, authState);
      syncPrefs.setNextcloudPollParams(null);
      if (context instanceof Activity)
      {
        Intent returnIntent = new Intent(context, context.getClass());
        context.startActivity(returnIntent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT));
      }
      switch (result)
      {
      case Success ->
        new Handler(Looper.getMainLooper())
            .post(() -> Toast.makeText(context, R.string.account_connection_success, Toast.LENGTH_SHORT).show());
      case UnexpectedError -> showErrorDialog(context, ""); // should be fairly impossible
      case AlreadyExists -> showErrorDialog(context, context.getString(R.string.account_already_exists));
      }
      return true;
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to poll Nextcloud auth status", e);
      return false;
    }
  }

  public static class PollParams
  {
    private static final String KEY_TOKEN = "token";
    private static final String KEY_ENDPOINT = "endpoint";
    private final String token;
    private final String endpoint;

    private PollParams(String token, String endpoint)
    {
      this.token = token;
      this.endpoint = endpoint;
    }

    /// {"token": string, "endpoint": string }
    public static PollParams fromJson(JSONObject json) throws JSONException
    {
      return new PollParams(json.getString(KEY_TOKEN), json.getString(KEY_ENDPOINT));
    }

    public static PollParams fromString(String jsonString) throws JSONException
    {
      return fromJson(new JSONObject(jsonString));
    }

    @NonNull
    public String asString() throws JSONException
    {
      JSONObject json = new JSONObject();
      json.put(KEY_TOKEN, token);
      json.put(KEY_ENDPOINT, endpoint);
      return json.toString();
    }
  }
}
