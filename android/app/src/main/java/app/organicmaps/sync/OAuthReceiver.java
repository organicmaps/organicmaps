package app.organicmaps.sync;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Base64;
import android.widget.Toast;
import app.organicmaps.R;
import app.organicmaps.sdk.util.log.Logger;
import java.nio.charset.StandardCharsets;
import java.util.Objects;
import net.openid.appauth.AuthorizationResponse;
import net.openid.appauth.AuthorizationService;
import org.json.JSONObject;

public class OAuthReceiver extends BroadcastReceiver
{
  private static final String TAG = OAuthReceiver.class.getSimpleName();

  public static PendingIntent getPendingIntent(Context context, int requestCode)
  {
    Intent intent = new Intent();
    intent.setComponent(new ComponentName(context, OAuthReceiver.class));
    return PendingIntent.getBroadcast(context, requestCode, intent, PendingIntent.FLAG_MUTABLE);
  }

  private static void showFailureToast(Context context)
  {
    Toast.makeText(context, R.string.error_adding_account, Toast.LENGTH_LONG).show();
  }

  @Override
  public void onReceive(Context context, Intent intent)
  {
    Uri responseUri = intent.getData();
    if (responseUri == null || responseUri.getPath() == null)
    {
      Logger.d(TAG, "Unexpected intent received. The intent data is " + responseUri);
      return;
    }
    if (responseUri.getPath().startsWith(GoogleLoginHelper.GOOGLE_OAUTH_REDIRECT_PATH))
    {
      googleAuth(context.getApplicationContext(), AuthorizationResponse.fromIntent(intent));
    }
    else
    {
      Logger.e(TAG, "Unsupported redirect: " + responseUri);
    }
  }

  private void googleAuth(Context appContext, final AuthorizationResponse response)
  {
    if (response == null)
    {
      showFailureToast(appContext);
      return;
    }

    final AuthorizationService service = new AuthorizationService(appContext.getApplicationContext());
    service.performTokenRequest(response.createTokenExchangeRequest(), (tokenResponse, exception) -> {
      if (exception != null || tokenResponse == null)
      {
        showFailureToast(appContext);
        return;
      }
      String idToken =
          tokenResponse.idToken; // The "email" (or a similar) scope is necessary for the response to have an id_token
      try
      {
        // the idToken is a JWT with the payload being of the form: { ..., "email": string, ...}
        String payload = new String(Base64.decode(Objects.requireNonNull(idToken).split("\\.")[1], Base64.DEFAULT),
                                    StandardCharsets.UTF_8);
        String email = new JSONObject(payload).getString("email");
        GoogleDriveAuth authState = GoogleDriveAuth.fromTokenAndEmail(tokenResponse.refreshToken, email);
        SyncPrefs.AddAccountResult result =
            SyncPrefs.getInstance(appContext).addAccount(BackendType.GoogleDrive, authState);

        switch (result)
        {
          case Success -> Toast.makeText(appContext, R.string.account_connection_success, Toast.LENGTH_SHORT).show();
          case UnexpectedError -> showFailureToast(appContext); // should be fairly impossible
          case AlreadyExists -> Toast.makeText(appContext, R.string.account_already_exists, Toast.LENGTH_LONG).show();
        }
      }
      catch (Exception e)
      {
        showFailureToast(appContext);
        Logger.e(TAG, "Error trying to parse token request for Google OAuth", e);
      }
    });
    service.dispose();
  }
}
