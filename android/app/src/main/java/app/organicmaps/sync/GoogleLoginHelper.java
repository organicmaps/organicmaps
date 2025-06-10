package app.organicmaps.sync;

import android.content.Context;
import android.net.Uri;
import android.util.Base64;
import app.organicmaps.BuildConfig;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import net.openid.appauth.AppAuthConfiguration;
import net.openid.appauth.AuthorizationRequest;
import net.openid.appauth.AuthorizationService;
import net.openid.appauth.AuthorizationServiceConfiguration;
import net.openid.appauth.ResponseTypeValues;

public class GoogleLoginHelper
{
  public static final String GOOGLE_OAUTH_REDIRECT_PATH = "/GoogleOAuthRedirect";

  public static void login(Context context)
  {
    SecureRandom secureRandom = new SecureRandom();
    byte[] bytes = new byte[64];
    secureRandom.nextBytes(bytes);
    int encoding = Base64.URL_SAFE | Base64.NO_PADDING | Base64.NO_WRAP;
    String codeVerifier = Base64.encodeToString(bytes, encoding);
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
    AuthorizationRequest.Builder builder =
        new AuthorizationRequest
            .Builder(
                new AuthorizationServiceConfiguration(Uri.parse("https://accounts.google.com/o/oauth2/v2/auth"),
                                                      Uri.parse("https://www.googleapis.com/oauth2/v4/token"), null,
                                                      Uri.parse("https://accounts.google.com/o/oauth2/revoke?token=")),
                "215362850630-l9j0opuu675mu14ri7b9mnsi28nroh2e.apps.googleusercontent.com", // TODO use the real one
                                                                                            // instead
                ResponseTypeValues.CODE, Uri.parse(BuildConfig.APPLICATION_ID + ":" + GOOGLE_OAUTH_REDIRECT_PATH))
            .setCodeVerifier(codeVerifier, codeChallenge, "S256");
    // Two scopes in use:
    //   1. drive.file (see https://developers.google.com/workspace/drive/api/guides/api-specific-auth)
    //   2. email (used for showing user-facing profile name and for recognizing attempts to connect the same account
    //   twice)
    builder.setScopes("https://www.googleapis.com/auth/drive.file", "email");
    AuthorizationRequest request = builder.build();
    AppAuthConfiguration appAuthConfiguration = new AppAuthConfiguration.Builder().build();
    AuthorizationService service =
        new AuthorizationService(context, appAuthConfiguration); // this must be manually disposed
    service.performAuthorizationRequest(request, OAuthReceiver.getPendingIntent(context, request.hashCode()));
    service.dispose();
  }
}
