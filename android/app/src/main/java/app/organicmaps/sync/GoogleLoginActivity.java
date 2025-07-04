package app.organicmaps.sync;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Base64;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.browser.customtabs.CustomTabsClient;
import androidx.browser.customtabs.CustomTabsIntent;
import androidx.browser.customtabs.CustomTabsServiceConnection;
import androidx.browser.customtabs.CustomTabsSession;
import app.organicmaps.BuildConfig;
import app.organicmaps.R;
import app.organicmaps.sdk.util.UiUtils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.util.Utils;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.Objects;
import org.json.JSONObject;

public class GoogleLoginActivity extends AppCompatActivity
{
  private static final String TAG = GoogleLoginActivity.class.getSimpleName();
  public static final String GOOGLE_OAUTH_REDIRECT_PATH = "/GoogleOAuthRedirect";
  public static final String EXTRA_DO_AUTH = "auth";

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    handleIntent(getIntent());
  }

  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);
    handleIntent(intent);
  }

  private void handleIntent(Intent intent)
  {
    if (intent.getBooleanExtra(EXTRA_DO_AUTH, false))
      GoogleLoginHelper.attemptPlayServicesAuth(this, this::webLogin);
    else if (intent.getData() != null && intent.getData().getScheme() != null
             && intent.getData().getScheme().equals(BuildConfig.APPLICATION_ID))
    {
      Uri redirectUri = intent.getData();
      if (redirectUri.getPath() != null && redirectUri.getPath().startsWith(GOOGLE_OAUTH_REDIRECT_PATH))
      {
        ThreadPool.getWorker().execute(() -> {
          HttpURLConnection conn = null;
          try
          {
            SyncPrefs prefs = SyncPrefs.getInstance(GoogleLoginActivity.this);
            String storedAuthParams = prefs.getGoogleOauthParams();
            if (storedAuthParams == null)
            {
              Logger.e(TAG, "Uninitiated redirect. No stored state or code verifier found.");
              return;
            }
            String expectedState = storedAuthParams.split(":")[0];
            String codeVerifier = storedAuthParams.split(":")[1];
            String receivedState = redirectUri.getQueryParameter("state");
            if (expectedState == null || !expectedState.equals(receivedState))
            {
              Logger.e(TAG, "State mismatch in Google OAuth redirect: expected " + expectedState + ", received "
                                + receivedState);
              return;
            }
            String code = redirectUri.getQueryParameter("code");
            if (code == null)
            {
              Logger.e(TAG, "No authorization code found in Google OAuth redirect.");
              return;
            }
            conn = (HttpURLConnection) new URL("https://oauth2.googleapis.com/token").openConnection();
            conn.setRequestMethod("POST");
            conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
            conn.setDoOutput(true);
            String postData = "client_id=" + Uri.encode(getClientId()) + "&code=" + Uri.encode(code)
                            + "&redirect_uri=" + Uri.encode(getRedirectUri()) + "&grant_type=authorization_code"
                            + "&code_verifier=" + Uri.encode(codeVerifier);
            conn.getOutputStream().write(postData.getBytes(StandardCharsets.UTF_8));
            conn.setConnectTimeout(10000);
            conn.setReadTimeout(10000);
            int responseCode = conn.getResponseCode();
            boolean isError = responseCode / 100 != 2;
            StringBuilder response = new StringBuilder();
            try (InputStream is = isError ? conn.getErrorStream() : conn.getInputStream();
                 BufferedReader reader = new BufferedReader(new InputStreamReader(is)))
            {
              String line;
              while ((line = reader.readLine()) != null)
              {
                response.append(line);
              }
            }
            if (isError)
              throw new Exception("Error response from Google OAuth token endpoint: " + responseCode + " - "
                                  + response.toString());

            // response format: {
            //                     "refresh_token": string (persisted),
            //                     "id_token": jwt string (contains email and sub),
            //                     ... other unused fields
            //                  }
            JSONObject resp = new JSONObject(response.toString());
            String idToken = resp.getString("id_token");
            String refreshToken = resp.getString("refresh_token");
            String payload = new String(Base64.decode(Objects.requireNonNull(idToken).split("\\.")[1], Base64.DEFAULT),
                                        StandardCharsets.UTF_8);
            String email = new JSONObject(payload).getString("email");
            String sub = new JSONObject(payload).getString("sub");
            GoogleDriveAuth authState = GoogleDriveAuth.from(refreshToken, email, sub);
            SyncPrefs.AddAccountResult result = prefs.addAccount(BackendType.GoogleDrive, authState);
            runOnUiThread(() -> {
              switch (result)
              {
                case Success ->
                  Toast.makeText(GoogleLoginActivity.this, R.string.account_connection_success, Toast.LENGTH_SHORT)
                      .show();
                case UnexpectedError ->
                  Toast.makeText(GoogleLoginActivity.this, R.string.error_adding_account, Toast.LENGTH_LONG).show();
                case AlreadyExists ->
                  Toast.makeText(GoogleLoginActivity.this, R.string.account_already_exists, Toast.LENGTH_LONG).show();
              }
            });
          }
          catch (Exception e)
          {
            Logger.e(TAG, "Error making token request", e);
            runOnUiThread(
                ()
                    -> Toast.makeText(GoogleLoginActivity.this, R.string.error_adding_account, Toast.LENGTH_SHORT)
                           .show());
          }
          finally
          {
            if (conn != null)
              conn.disconnect();
            finish();
          }
        });
      }
      else
      {
        Logger.d(TAG, "Unrecognized redirect uri received: " + redirectUri);
      }
    }
    else
      finish();
  }

  @Override
  public void finish()
  {
    super.finish();
    overridePendingTransition(0, 0);
  }

  private void webLogin()
  {
    SecureRandom secureRandom = new SecureRandom();
    byte[] codeVerifierBytes = new byte[64];
    secureRandom.nextBytes(codeVerifierBytes);
    int encoding = Base64.URL_SAFE | Base64.NO_PADDING | Base64.NO_WRAP;
    String codeVerifier = Base64.encodeToString(codeVerifierBytes, encoding);
    MessageDigest digest;
    try
    {
      digest = MessageDigest.getInstance("SHA-256");
    }
    catch (NoSuchAlgorithmException e)
    {
      // TODO(savsch) check if this catch branch is reachable on any Android device
      return;
    }
    byte[] hash = digest.digest(codeVerifier.getBytes());
    String codeChallenge = Base64.encodeToString(hash, encoding);
    byte[] stateBytes = new byte[16];
    secureRandom.nextBytes(stateBytes);
    String state = Base64.encodeToString(hash, encoding);
    byte[] nonceBytes = new byte[16];
    secureRandom.nextBytes(nonceBytes);
    String nonce = Base64.encodeToString(hash, encoding);

    Uri oAuthUri =
        new Uri.Builder()
            .scheme("https")
            .authority("accounts.google.com")
            .path("o/oauth2/v2/auth")
            .appendQueryParameter("redirect_uri", getRedirectUri())
            .appendQueryParameter("client_id", getClientId())
            .appendQueryParameter("response_type", "code")
            .appendQueryParameter("state", state)
            .appendQueryParameter("nonce", nonce)
            // having email (openid scope) ensures we get the sub (unique user identifier) as well as the email in the
            // idToken for drive.file see https://developers.google.com/workspace/drive/api/guides/api-specific-auth
            .appendQueryParameter("scope", "https://www.googleapis.com/auth/drive.file email")
            .appendQueryParameter("code_challenge", codeChallenge)
            .appendQueryParameter("code_challenge_method", "S256")
            .build();

    String customTabsPackage = Utils.getCustomTabsPackage(this);
    if (customTabsPackage != null)
      CustomTabsClient.bindCustomTabsService(this, customTabsPackage,
                                             getCustomTabsServiceConnection(customTabsPackage, oAuthUri));
    else
      startActivity(new Intent(Intent.ACTION_VIEW, oAuthUri));
    SyncPrefs.getInstance(this).setGoogleOauthParams(state + ":" + codeVerifier);
    finish();
  }

  @NonNull
  private CustomTabsServiceConnection getCustomTabsServiceConnection(String customTabsPackage, Uri oAuthUri)
  {
    final Context appContext = getApplicationContext();
    return new CustomTabsServiceConnection() {
      @Override
      public void onCustomTabsServiceConnected(@NonNull ComponentName name, @NonNull CustomTabsClient client)
      {
        client.warmup(0L);
        CustomTabsSession session = client.newSession(null);
        CustomTabsIntent customTabsIntent =
            new CustomTabsIntent.Builder(session)
                .setInitialActivityHeightPx(UiUtils.getDisplayTotalHeight(appContext) * 9 / 10)
                .build();
        customTabsIntent.intent.setPackage(customTabsPackage);
        customTabsIntent.intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
        customTabsIntent.launchUrl(appContext, oAuthUri);
      }

      @Override
      public void onServiceDisconnected(ComponentName name)
      {}
    };
  }

  private String getClientId()
  {
    return "215362850630-l9j0opuu675mu14ri7b9mnsi28nroh2e.apps.googleusercontent.com"; // TODO use the real one, based
                                                                                       // on applicationId, instead.
  }

  private String getRedirectUri()
  {
    return BuildConfig.APPLICATION_ID + ":" + GOOGLE_OAUTH_REDIRECT_PATH;
  }

  public interface GmsAuthFailedCallback
  {
    void onGmsAuthFailed();
  }
}
