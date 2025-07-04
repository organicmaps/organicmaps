package app.organicmaps.sync;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.browser.customtabs.CustomTabsCallback;
import androidx.browser.customtabs.CustomTabsClient;
import androidx.browser.customtabs.CustomTabsIntent;
import androidx.browser.customtabs.CustomTabsServiceConnection;
import androidx.browser.customtabs.CustomTabsSession;
import app.organicmaps.R;
import app.organicmaps.sdk.util.UiUtils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.util.InsecureHttpsHelper;
import app.organicmaps.util.Utils;
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

  @MainThread
  public static void login(Context context)
  {
    final AlertDialog dialog =
        new MaterialAlertDialogBuilder(context, R.style.MwmTheme_AlertDialog)
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
          final URL url = new URL(input);
          String baseUrl = url.getProtocol() + "://" + url.getHost();
          int port = url.getPort();
          if (port != -1)
          {
            baseUrl += ":" + port;
          }
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
                // example success response: { "poll": {"token": string, "endpoint": string }, "login": string }
                LoginParams params = new LoginParams(response.toString());
                new Handler(Looper.getMainLooper()).post(() -> {
                  dialog.dismiss();
                  loginFlowStepTwo(context, params);
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
  private static void loginFlowStepTwo(Context context, LoginParams loginParams)
  {
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
              if (mLoginComplete)
                return;
              if (navigationEvent == CustomTabsCallback.NAVIGATION_FINISHED)
              {
                // The user might have finished authentication. Poll to check the same.
                ThreadPool.getWorker().execute(() -> {
                  try
                  {
                    HttpURLConnection connection = InsecureHttpsHelper.openInsecureConnection(loginParams.pollEndpoint);
                    connection.setRequestMethod("POST");
                    connection.setRequestProperty("Accept", "application/json");
                    connection.setDoOutput(true);
                    try (OutputStream os = connection.getOutputStream())
                    {
                      byte[] body = ("token=" + loginParams.pollToken).getBytes(StandardCharsets.UTF_8);
                      os.write(body, 0, body.length);
                    }
                    final int responseCode = connection.getResponseCode();
                    if (responseCode / 100 != 2)
                      return;
                    StringBuilder response = new StringBuilder();
                    try (BufferedReader in = new BufferedReader(new InputStreamReader(connection.getInputStream())))
                    {
                      String inputLine;
                      while ((inputLine = in.readLine()) != null)
                        response.append(inputLine);
                    }
                    if (response.toString().isEmpty())
                      return;
                    // success response format: { "poll": {"token": string, "endpoint": string }, "login": string }
                    JSONObject responseJson = new JSONObject(response.toString());
                    NextcloudAuth authState = new NextcloudAuth(responseJson);
                    SyncPrefs.AddAccountResult result =
                        SyncPrefs.getInstance(context).addAccount(BackendType.Nextcloud, authState);
                    if (context instanceof Activity)
                      context.startActivity(
                          new Intent(context, context.getClass()).addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT));
                    mLoginComplete = true;
                    switch (result)
                    {
                      case Success ->
                        Toast.makeText(context, R.string.account_connection_success, Toast.LENGTH_SHORT).show();
                      case UnexpectedError -> showErrorDialog(context, ""); // should be fairly impossible
                      case AlreadyExists ->
                        showErrorDialog(context, context.getString(R.string.account_already_exists));
                    }
                  }
                  catch (Exception e)
                  {
                    Logger.e(TAG, "Failed to poll Nextcloud auth status", e);
                  }
                });
              }
            }
          };

          CustomTabsSession session = client.newSession(customTabsCallback);

          CustomTabsIntent customTabsIntent =
              new CustomTabsIntent
                  .Builder(session)
                  // Set initial custom tabs height to 90% of total display height
                  .setInitialActivityHeightPx(UiUtils.getDisplayTotalHeight(context) * 9 / 10)
                  .build();

          customTabsIntent.intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
          customTabsIntent.launchUrl(context, Uri.parse(loginParams.loginUrl));
        }

        @Override
        public void onServiceDisconnected(ComponentName name)
        {}
      };

      CustomTabsClient.bindCustomTabsService(context, customTabsPackage, connection);
    }
    else
    {
      throw new RuntimeException("TODO implement");
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

  private static class LoginParams
  {
    private final String pollToken;
    private final String pollEndpoint;
    private final String loginUrl;

    /**
     * @param rawServerJsonResponse Raw string of a json of the form { "poll": {"token": string, "endpoint": string },
     *     "login": string }
     */
    private LoginParams(String rawServerJsonResponse) throws JSONException
    {
      JSONObject jsonResponse = new JSONObject(rawServerJsonResponse);
      JSONObject pollObject = jsonResponse.getJSONObject("poll");
      this.pollToken = pollObject.getString("token");
      this.pollEndpoint = pollObject.getString("endpoint");
      this.loginUrl = jsonResponse.getString("login");
    }
  }
}
