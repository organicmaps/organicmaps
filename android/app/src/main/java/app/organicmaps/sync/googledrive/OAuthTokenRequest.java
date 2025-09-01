package app.organicmaps.sync.googledrive;

import android.net.Uri;
import android.util.Base64;
import androidx.annotation.NonNull;
import androidx.annotation.WorkerThread;
import app.organicmaps.sdk.sync.googledrive.GoogleDriveAuth;
import app.organicmaps.sdk.sync.googledrive.OAuthHelper;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.Objects;
import org.json.JSONObject;

public class OAuthTokenRequest
{
  @WorkerThread
  public static GoogleDriveAuth make(Uri redirectUri, StoredOAuthParams expectedParams) throws Exception
  {
    String receivedState = redirectUri.getQueryParameter("state");
    if (!Objects.equals(expectedParams.state(), receivedState))
      throw new Exception("State mismatch in Google OAuth redirect: expected " + expectedParams.state() + ", received "
                          + receivedState);
    String code = Objects.requireNonNull(redirectUri.getQueryParameter("code"));

    HttpURLConnection conn = null;
    try
    {
      conn = (HttpURLConnection) new URL("https://oauth2.googleapis.com/token").openConnection();
      conn.setRequestMethod("POST");
      conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
      conn.setDoOutput(true);
      String postData = "client_id=" + Uri.encode(OAuthHelper.getClientId()) + "&code=" + Uri.encode(code)
                      + "&redirect_uri=" + Uri.encode(GoogleLoginActivity.getRedirectUri())
                      + "&grant_type=authorization_code"
                      + "&code_verifier=" + Uri.encode(expectedParams.codeVerifier());
      conn.getOutputStream().write(postData.getBytes(StandardCharsets.UTF_8));
      conn.setConnectTimeout(10000);
      conn.setReadTimeout(10000);
      JSONObject resp = readJsonObject(conn);
      String idToken = resp.getString("id_token");
      String refreshToken = resp.getString("refresh_token");
      String payload = new String(Base64.decode(Objects.requireNonNull(idToken).split("\\.")[1], Base64.DEFAULT),
                                  StandardCharsets.UTF_8);
      String email = new JSONObject(payload).getString("email");
      String sub = new JSONObject(payload).getString("sub");

      return GoogleDriveAuth.from(refreshToken, email, sub);
    }
    finally
    {
      if (conn != null)
        conn.disconnect();
    }
  }

  @NonNull
  private static JSONObject readJsonObject(HttpURLConnection conn) throws Exception
  {
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
    return resp;
  }
}
