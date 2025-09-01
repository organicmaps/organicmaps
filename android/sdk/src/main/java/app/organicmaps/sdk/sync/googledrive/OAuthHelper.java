package app.organicmaps.sdk.sync.googledrive;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.sync.SyncOpException;
import app.organicmaps.sdk.util.log.Logger;
import java.io.IOException;
import okhttp3.FormBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import org.json.JSONException;
import org.json.JSONObject;
public class OAuthHelper
{
  private static final String TAG = OAuthHelper.class.getSimpleName();

  public static String getClientId()
  {
    // TODO use the real one, based on applicationId, instead.
    return "215362850630-l9j0opuu675mu14ri7b9mnsi28nroh2e.apps.googleusercontent.com";
  }

  static TokenResponse generateAccessToken(OkHttpClient client, String refreshToken) throws SyncOpException
  {
    RequestBody requestBody = new FormBody.Builder()
                                  .add("client_id", getClientId())
                                  .add("refresh_token", refreshToken)
                                  .add("grant_type", "refresh_token")
                                  .build();

    Request request = new Request.Builder()
                          .url("https://oauth2.googleapis.com/token")
                          .post(requestBody)
                          .addHeader("Content-Type", "application/x-www-form-urlencoded")
                          .build();

    try (Response response = client.newCall(request).execute())
    {
      if (response.code() >= 400 && response.code() < 404)
      {
        Logger.e(TAG, "OAuthHelper.generateAccessToken failed with HTTP " + response.code());
        throw new SyncOpException.AuthExpiredException();
      }
      else if (!response.isSuccessful())
      {
        Logger.e(TAG, "OAuthHelper.generateAccessToken failed with HTTP " + response.code());
        throw new SyncOpException.UnexpectedException("HTTP " + response.code());
      }

      JSONObject resp = new JSONObject(response.body().string());
      String authHeader = resp.getString("token_type") + " " + resp.getString("access_token");
      return new TokenResponse(authHeader, resp.getInt("expires_in"));
    }
    catch (IOException e)
    {
      Logger.e(TAG, "OAuth token request failed.", e);
      throw new SyncOpException.NetworkException();
    }
    catch (JSONException e)
    {
      Logger.e(TAG, "Unable to parse access token response.", e);
      throw new SyncOpException.UnexpectedException(e.getLocalizedMessage());
    }
  }

  /**
   * @param expiresIn seconds (usually 3599)
   */
  record TokenResponse(@NonNull String authHeader, int expiresIn) {}
}
