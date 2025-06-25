package app.organicmaps.sync;

import android.app.Activity;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.widget.Toast;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.IntentSenderRequest;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import app.organicmaps.R;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.log.Logger;
import com.google.android.gms.auth.api.identity.AuthorizationRequest;
import com.google.android.gms.auth.api.identity.AuthorizationResult;
import com.google.android.gms.auth.api.identity.Identity;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.common.Scopes;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.api.Scope;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Arrays;
import java.util.List;
import org.json.JSONObject;

public class GoogleLoginHelper
{
  public static final String TAG = GoogleLoginHelper.class.getSimpleName();

  public static void attemptPlayServicesAuth(final AppCompatActivity activity,
                                             final GoogleLoginActivity.GmsAuthFailedCallback gmsAuthFailedCallback)
  {
    if (GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(activity) != ConnectionResult.SUCCESS)
    {
      gmsAuthFailedCallback.onGmsAuthFailed();
      return;
    }

    // TODO(savsch) figure out how to enable adding multiple Google accounts on the same device using this flow without
    //  the GET_ACCOUNTS runtime permission or requiring the user to input tne email manually.
    //  until then, just fall back to web login flow in case there's already >=1 Google account signed in
    if (SyncPrefs.getInstance(activity).getAccounts().stream().anyMatch(
            account -> account.getBackendType() == BackendType.GoogleDrive))
    {
      gmsAuthFailedCallback.onGmsAuthFailed();
      return;
    }

    ActivityResultLauncher<IntentSenderRequest> authLauncher =
        activity.registerForActivityResult(new ActivityResultContracts.StartIntentSenderForResult(), result -> {
          if (result.getResultCode() == Activity.RESULT_OK)
          {
            try
            {
              AuthorizationResult authResult =
                  Identity.getAuthorizationClient(activity).getAuthorizationResultFromIntent(result.getData());
              fetchProfile(authResult.getAccessToken(), activity.getApplicationContext());
              activity.finish();
            }
            catch (ApiException e)
            {
              throw new RuntimeException(e); // TODO(savsch)
            }
          }
          else
          {
            Toast.makeText(activity, R.string.error_adding_account, Toast.LENGTH_LONG).show();
            activity.finish();
          }
        });

    List<Scope> requestedScopes = Arrays.asList(new Scope(Scopes.DRIVE_FILE), new Scope(Scopes.EMAIL));
    AuthorizationRequest authorizationRequest =
        AuthorizationRequest.builder().setRequestedScopes(requestedScopes).build();
    Identity.getAuthorizationClient(activity)
        .authorize(authorizationRequest)
        .addOnSuccessListener(authResult -> {
          if (authResult.hasResolution())
          {
            // Access needs to be granted by the user
            IntentSenderRequest request =
                new IntentSenderRequest.Builder(authResult.getPendingIntent().getIntentSender()).build();
            authLauncher.launch(request);
          }
          else
          {
            fetchProfile(authResult.getAccessToken(), activity.getApplicationContext()); // Access already granted
            activity.finish();
          }
        })
        .addOnFailureListener(e -> {
          Logger.e(TAG, "Failed to authorize", e);
          // TODO investigate when this happens (other than when using .setAccount with a non-existing account)
          activity.finish();
        });
  }

  private static void fetchProfile(final String accessToken, final Context appContext)
  {
    ThreadPool.getWorker().execute(() -> {
      HttpURLConnection connection = null;
      String responseBody;
      try
      {
        connection = (HttpURLConnection) new URL("https://www.googleapis.com/oauth2/v3/userinfo").openConnection();
        connection.setRequestProperty("Authorization", "Bearer " + accessToken);
        connection.setConnectTimeout(10000);
        connection.setReadTimeout(10000);
        try (InputStream is = connection.getInputStream(); ByteArrayOutputStream result = new ByteArrayOutputStream())
        {
          byte[] buffer = new byte[4096];
          int length;
          while ((length = is.read(buffer)) != -1)
          {
            result.write(buffer, 0, length);
          }
          responseBody = result.toString("UTF-8");
        }
        JSONObject userinfo = new JSONObject(responseBody);
        GoogleDriveAuth authState = GoogleDriveAuth.from(null, userinfo.getString("email"), userinfo.getString("sub"));
        final SyncPrefs.AddAccountResult result =
            SyncPrefs.getInstance(appContext).addAccount(BackendType.GoogleDrive, authState);
        new Handler(Looper.getMainLooper()).post(() -> {
          switch (result)
          {
            case Success -> Toast.makeText(appContext, R.string.account_connection_success, Toast.LENGTH_SHORT).show();
            case UnexpectedError ->
              Toast.makeText(appContext, R.string.error_adding_account, Toast.LENGTH_SHORT).show();
            case AlreadyExists -> Toast.makeText(appContext, R.string.account_already_exists, Toast.LENGTH_LONG).show();
          }
        });
      }
      catch (Exception e)
      {
        Logger.e(TAG, "Error trying to fetch profile info from access token", e);
        new Handler(Looper.getMainLooper())
            .post(()
                      -> Toast
                             .makeText(appContext, R.string.error_adding_account + " : " + e.getLocalizedMessage(),
                                       Toast.LENGTH_SHORT)
                             .show());
      }
      finally
      {
        if (connection != null)
          connection.disconnect();
      }
    });
  }
}
