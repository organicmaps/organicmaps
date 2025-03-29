package app.organicmaps.settings;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.widget.Toast;

import androidx.annotation.MainThread;
import androidx.appcompat.app.AlertDialog;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleEventObserver;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.SwitchPreferenceCompat;

import org.json.JSONException;
import org.json.JSONObject;

import app.organicmaps.R;
import app.organicmaps.sync.nc.InitialV2LoginParams;
import app.organicmaps.sync.nc.NextcloudPreferences;
import app.organicmaps.sync.nc.PollHelper;
import app.organicmaps.util.log.Logger;
import app.organicmaps.widget.BetterEditTextPreference;
import experiment.InsecureHttpsHelper;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.Objects;

public class NcSyncSettingsFragment extends BaseXmlSettingsFragment
{
  private static final String PREF_SERVER_URL = "nextcloud_server_url";
  private static final String PREF_CONNECT = "nextcloud_connect";
  private static final String PREF_LOGOUT = "nextcloud_logout";
  private static final String PREF_SYNC_OPTIONS = "category_nextcloud_sync_options";
  private static final String PREF_SYNC_ENABLED = "nextcloud_sync_enabled";
  private static final String PREF_PROGRESS = "nextcloud_progress";

  private static final int CONNECT_TIMEOUT_MS = 5_000;
  private static final long NC_LOGIN_URL_EXPIRATION_MS = 60 * 20_000;
  private static final long NC_AUTH_POLL_DELAY_MS = 5000;

  private final Handler mHandler = new Handler(Looper.getMainLooper());
  private BetterEditTextPreference mServerUrlPreference;
  private Preference mConnectPreference;
  private Preference mLogoutPreference;
  private PreferenceCategory mSyncOptionsCategory;
  private SwitchPreferenceCompat mSyncEnabledPreference;
  private Preference mProgressPreference;

  private InitialV2LoginParams mLoginParams;
  private long lastAuthRequestTime = Long.MIN_VALUE;

  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_nextcloud_sync;
  }

  @Override
  public void onCreatePreferences(Bundle savedInstanceState, String rootKey)
  {
    super.onCreatePreferences(savedInstanceState, rootKey);

    mServerUrlPreference = findPreference(PREF_SERVER_URL);
    mConnectPreference = findPreference(PREF_CONNECT);
    mLogoutPreference = findPreference(PREF_LOGOUT);
    mSyncOptionsCategory = findPreference(PREF_SYNC_OPTIONS);
    mSyncEnabledPreference = findPreference(PREF_SYNC_ENABLED);
    mProgressPreference = findPreference(PREF_PROGRESS);
    setConnectionInProgress(false);

    setupAuthStatusPolling();
    setupListeners();
    updateUI();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    updateUI();
  }

  private void setupAuthStatusPolling()
  {
    HandlerThread handlerThread = new HandlerThread("AuthStatusPollingThread");
    handlerThread.start();
    PollHelper pollHelper = new PollHelper(NC_AUTH_POLL_DELAY_MS, new Handler(handlerThread.getLooper()), () -> {
      // The nextcloud initial login params expire after 20 minutes of creation
      if (mLoginParams == null || System.currentTimeMillis() - NC_LOGIN_URL_EXPIRATION_MS > lastAuthRequestTime)
        return;

      try
      {
        HttpURLConnection connection = InsecureHttpsHelper.openInsecureConnection(mLoginParams.getPollEndpoint());
        connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
        connection.setRequestMethod("POST");
        connection.setRequestProperty("Accept", "application/json");
        connection.setDoOutput(true);

        try (OutputStream os = connection.getOutputStream())
        {
          byte[] body = ("token=" + mLoginParams.getPollToken()).getBytes(StandardCharsets.UTF_8);
          os.write(body, 0, body.length);
        }
        // example success response: { "poll": {"token": string, "endpoint": string }, "login": string }
        final int responseCode = connection.getResponseCode();
        if (responseCode / 100 != 2)
          return;
        try (BufferedReader in = new BufferedReader(new InputStreamReader(connection.getInputStream())))
        {
          String inputLine;
          StringBuilder response = new StringBuilder();
          while ((inputLine = in.readLine()) != null)
          {
            response.append(inputLine);
          }

          // by now, mLoginParams could have become null due to verification by another runnable, so another null check (tfw no kotlin)
          if (mLoginParams == null)
            return;

          // expected format: {"server": string, "loginName": string, "appPassword": string }
          JSONObject responseJson = new JSONObject(response.toString());
          String server = Objects.requireNonNull(responseJson.getString("server"));
          String loginName = Objects.requireNonNull(responseJson.getString("loginName"));
          String appPassword = Objects.requireNonNull(responseJson.getString("appPassword"));

          new Handler(Looper.getMainLooper()).post(() -> {
            NextcloudPreferences.setAuthCredentials(requireContext(), server, loginName, appPassword);
            Toast.makeText(requireContext(), "Logged in successfully as " + loginName, Toast.LENGTH_SHORT).show();
            updateUI();
          });
        }
      } catch (Exception e)
      {
        Logger.e("pocstuff", "failed to poll auth status", e);
      }

    });
    pollHelper.setNoInitialDelay(true);
    getLifecycle().addObserver((LifecycleEventObserver) (lifecycleOwner, event) -> {
      if (event == Lifecycle.Event.ON_STOP)
        pollHelper.stop();
      else if (event == Lifecycle.Event.ON_START)
        pollHelper.start();
      else if (event == Lifecycle.Event.ON_DESTROY)
        handlerThread.quitSafely();
    });
  }


  private void setupListeners()
  {
    if (mServerUrlPreference != null)
    {
      mServerUrlPreference.setOnPreferenceChangeListener((preference, newValue) -> {
        // wait for the value to be commited and then update other UI elements
        mHandler.post(this::updateUI);
        return true;
      });
    }

    if (mConnectPreference != null)
    {
      mConnectPreference.setOnPreferenceClickListener(preference -> {
        if (NextcloudPreferences.isAuthenticated(requireContext()))
        {
          showLogoutWarningDialog();
        }
        else
        {
          attemptToConnect();
        }
        return true;
      });
    }

    if (mLogoutPreference != null)
    {
      mLogoutPreference.setOnPreferenceClickListener(preference -> {
        NextcloudPreferences.clearAuthCredentials(requireContext());
        updateUI();
        // TODO also tell the server to destroy the token by calling DELETE on /ocs/v2.php/core/apppassword
        return true;
      });
    }

    if (mSyncEnabledPreference != null)
    {
      mSyncEnabledPreference.setOnPreferenceChangeListener((preference, newValue) -> {
        boolean enabled = (Boolean) newValue;
        NextcloudPreferences.setSyncEnabled(requireContext(), enabled);
        return true;
      });
    }
  }

  private void updateUI()
  {
    Context context = getContext();
    if (context == null)
      return;

    String configuredUrl = mServerUrlPreference != null ? mServerUrlPreference.getText() : "";
    String authenticatedUrl = NextcloudPreferences.getAuthenticatedServerUrl(context);
    boolean isAuthenticated = NextcloudPreferences.isAuthenticated(context);

    if (mConnectPreference != null)
    {
      boolean shouldShowConnect = configuredUrl != null && !configuredUrl.isEmpty();

      if (isAuthenticated)
      {
        shouldShowConnect = shouldShowConnect && !configuredUrl.equals(authenticatedUrl);
      }

      mConnectPreference.setVisible(shouldShowConnect);
      if (shouldShowConnect)
      {
        mConnectPreference.setSummary("Tap to login to " + configuredUrl);
      }
    }

    if (mLogoutPreference != null)
    {
      mLogoutPreference.setVisible(isAuthenticated);
      if (isAuthenticated)
      {
        mLogoutPreference.setTitle("Logged in as " + NextcloudPreferences.getLoginName(requireContext()));
        mLogoutPreference.setSummary("Tap to log out of " + authenticatedUrl);
      }
    }

    if (mSyncOptionsCategory != null)
    {
      mSyncOptionsCategory.setVisible(isAuthenticated);
    }
  }

  @MainThread
  private void showLogoutWarningDialog()
  {
    new AlertDialog.Builder(requireContext()).setTitle("Warning").setMessage("Doing this will log you out of your currently configured server.").setPositiveButton("Continue", (dialog, which) -> attemptToConnect()).setNegativeButton("Cancel", null).show();
  }

  @MainThread
  private void showFailedToConnectToServerDialog(String attemptedUrl, String errorMessage)
  {
    new AlertDialog.Builder(requireContext()).setTitle("Connection Failed").setMessage("POST request to " + attemptedUrl + " unsuccessful.\n\nError message: " + errorMessage).setNegativeButton("Ok", null).show();
  }

  private void attemptToConnect()
  {
    String configuredUrl = mServerUrlPreference.getText();
    String requestUrl = configuredUrl;
    try
    {
      URL url = new URL(configuredUrl);
      String baseUrl = url.getProtocol() + "://" + url.getHost();
      int port = url.getPort();
      if (port != -1)
      {
        baseUrl += ":" + port;
      }
      requestUrl = baseUrl + "/index.php/login/v2";
    } catch (MalformedURLException e)
    {
      showFailedToConnectToServerDialog(requestUrl, "Malformed URL provided");
      return;
    }
    final String connectionUrl = requestUrl;
    setConnectionInProgress(true);
    new Thread(() -> {
      try
      {
        HttpURLConnection connection = InsecureHttpsHelper.openInsecureConnection(connectionUrl);
        connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
        connection.setRequestMethod("POST");
        connection.setRequestProperty("Content-Type", "application/json");
        connection.setRequestProperty("Accept", "application/json");
        connection.setRequestProperty("User-Agent", "some Organic Maps fork made for a GSoC proposal");
        connection.setDoOutput(true);
        // example success response: { "poll": {"token": string, "endpoint": string }, "login": string }
        final int responseCode = connection.getResponseCode();
        if (responseCode / 100 != 2)
          throw new Exception("Http error code " + responseCode);
        try (BufferedReader in = new BufferedReader(new InputStreamReader(connection.getInputStream())))
        {
          String inputLine;
          StringBuilder response = new StringBuilder();
          while ((inputLine = in.readLine()) != null)
          {
            response.append(inputLine);
          }
          try
          {
            final InitialV2LoginParams params = new InitialV2LoginParams(response.toString());
            mHandler.post(() -> {
              setConnectionInProgress(false);
              loginFlowStepTwo(params);
            });
          } catch (JSONException e)
          {
            throw new IllegalStateException("Unrecognized server response: " + response);
          }
        }
      } catch (Exception e)
      {
        mHandler.post(() -> {
          setConnectionInProgress(false);
          showFailedToConnectToServerDialog(connectionUrl, e.getLocalizedMessage());
        });
      }
    }).start();
  }

  public void setConnectionInProgress(boolean inProgress)
  {
    if (mProgressPreference != null)
    {
      mProgressPreference.setVisible(inProgress);
    }

    if (mConnectPreference != null)
    {
      mConnectPreference.setEnabled(!inProgress);
    }
  }

  @MainThread
  private void loginFlowStepTwo(InitialV2LoginParams params)
  {
    mLoginParams = params;
    lastAuthRequestTime = System.currentTimeMillis();
    Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(params.getLoginUrl()));
    if (intent.resolveActivity(requireContext().getPackageManager()) != null)
    {
      startActivity(intent);
    }
    else
    {
      Toast.makeText(requireContext(), "No browser application installed on device.", Toast.LENGTH_SHORT).show();
      // TODO consider using a WebView
    }
  }
}