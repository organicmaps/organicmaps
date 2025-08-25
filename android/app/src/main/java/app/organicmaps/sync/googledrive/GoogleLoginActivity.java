package app.organicmaps.sync.googledrive;

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
import app.organicmaps.sdk.sync.BackendType;
import app.organicmaps.sdk.sync.SyncManager;
import app.organicmaps.sdk.sync.googledrive.GoogleDriveAuth;
import app.organicmaps.sdk.sync.googledrive.OAuthHelper;
import app.organicmaps.sdk.sync.preferences.AddAccountResult;
import app.organicmaps.sdk.sync.preferences.SyncPrefs;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;

public class GoogleLoginActivity extends AppCompatActivity
{
  private static final String TAG = GoogleLoginActivity.class.getSimpleName();
  public static final String GOOGLE_OAUTH_REDIRECT_PATH = "/GoogleOAuthRedirect";
  public static final String EXTRA_PERFORM_LOGIN = "login";

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
    if (intent.getBooleanExtra(EXTRA_PERFORM_LOGIN, false))
      authenticate();
    else if (intent.getData() != null && intent.getData().getScheme() != null
             && intent.getData().getScheme().equals(BuildConfig.APPLICATION_ID))
    {
      Uri redirectUri = intent.getData();
      if (redirectUri.getPath() != null && redirectUri.getPath().startsWith(GOOGLE_OAUTH_REDIRECT_PATH))
      {
        handleOAuthRedirect(redirectUri);
      }
      else
      {
        Logger.d(TAG, "Unrecognized redirect uri received: " + redirectUri);
      }
    }
    else
      finish();
  }

  private void handleOAuthRedirect(Uri redirectUri)
  {
    SyncPrefs prefs = SyncManager.INSTANCE.getPrefs();
    String storedParamsStr = prefs.getGoogleOauthParams();
    if (storedParamsStr == null)
    {
      Logger.e(TAG, "Uninitiated redirect. No stored state or code verifier found.");
      return;
    }
    StoredOAuthParams storedParams = StoredOAuthParams.deserialize(storedParamsStr);
    ThreadPool.getWorker().execute(() -> {
      try
      {
        GoogleDriveAuth authState = OAuthTokenRequest.make(redirectUri, storedParams);
        AddAccountResult result = prefs.addAccount(BackendType.GoogleDrive, authState);
        runOnUiThread(() -> {
          switch (result)
          {
          case Success ->
            Toast.makeText(GoogleLoginActivity.this, R.string.account_connection_success, Toast.LENGTH_SHORT).show();
          case UnexpectedError ->
            Toast.makeText(GoogleLoginActivity.this, R.string.error_adding_account, Toast.LENGTH_LONG).show();
          case ReplacedExisting ->
            Toast.makeText(GoogleLoginActivity.this, R.string.relogin_message, Toast.LENGTH_LONG).show();
          }
        });
      }
      catch (Exception e)
      {
        Logger.e(TAG, "Error making token request", e);
        runOnUiThread(
            () -> Toast.makeText(GoogleLoginActivity.this, R.string.error_adding_account, Toast.LENGTH_SHORT).show());
      }
      finally
      {
        finish();
      }
    });
  }

  @Override
  public void finish()
  {
    super.finish();
    overridePendingTransition(0, 0);
  }

  private void authenticate()
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
      // Unlikely: https://developer.android.com/reference/java/security/MessageDigest
      Logger.e(TAG, "An error that should be impossible", e);
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
            .appendQueryParameter("client_id", OAuthHelper.getClientId())
            .appendQueryParameter("response_type", "code")
            .appendQueryParameter("state", state)
            .appendQueryParameter("nonce", nonce)
            // having email (openid scope) ensures we get the sub (unique user identifier) as well
            // as the email in the idToken.
            // There exist scopes narrower than "drive", but they would deprive OM of the ability to see
            // user-uploaded files. See https://developers.google.com/workspace/drive/api/guides/api-specific-auth
            // Besides, the auth tokens aren't stored on OM servers (and just stored on the user's device).
            .appendQueryParameter("scope", "https://www.googleapis.com/auth/drive email")
            .appendQueryParameter("code_challenge", codeChallenge)
            .appendQueryParameter("code_challenge_method", "S256")
            .build();

    String customTabsPackage = Utils.getCustomTabsPackage(this);
    if (customTabsPackage != null)
      CustomTabsClient.bindCustomTabsService(this, customTabsPackage,
                                             getCustomTabsServiceConnection(customTabsPackage, oAuthUri));
    else
      startActivity(new Intent(Intent.ACTION_VIEW, oAuthUri));
    SyncManager.INSTANCE.getPrefs().setGoogleOauthParams(new StoredOAuthParams(state, codeVerifier).serialize());
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

  static String getRedirectUri()
  {
    return BuildConfig.APPLICATION_ID + ":" + GOOGLE_OAUTH_REDIRECT_PATH;
  }
}
