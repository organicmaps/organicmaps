package app.organicmaps.sync.nextcloud;

import android.content.Context;
import androidx.annotation.WorkerThread;
import app.organicmaps.R;
import app.organicmaps.sdk.util.InsecureHttpsHelper;
import app.organicmaps.sdk.util.log.Logger;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import org.json.JSONException;
import org.json.JSONObject;

class ConnectionRequest
{
  private static final String TAG = ConnectionRequest.class.getSimpleName();

  /**
   * Makes the initial <a
   * href="https://docs.nextcloud.com/server/31/developer_manual/client_apis/LoginFlow/index.html">"anonymous POST
   * request"</a> synchronously.
   * @param context Needed to get the user facing error message.
   */
  @WorkerThread
  static void make(final Context context, final String connectUrl, final ConnectionRequestCallback callback)
  {
    HttpURLConnection connection = null;
    try
    {
      connection = InsecureHttpsHelper.openInsecureConnection(connectUrl);
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
        String responseBody = response.toString();

        try
        {
          // success response format: { "poll": {"token": string, "endpoint": string }, "login": string }
          final JSONObject responseJson = new JSONObject(responseBody);
          PollParams params = PollParams.fromJson(responseJson.getJSONObject("poll"));
          String loginUrl = responseJson.getString("login");

          callback.onSuccess(loginUrl, params);
        }
        catch (JSONException e)
        {
          throw new Exception(context.getString(R.string.unrecognized_server_response) + responseBody);
        }
      }
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Error trying to initiate connection with the server.", e);
      callback.onError(e.getLocalizedMessage());
    }
    finally
    {
      if (connection != null)
        connection.disconnect();
    }
  }

  interface ConnectionRequestCallback
  {
    void onSuccess(String loginUrl, PollParams pollParams);
    /**
     * @param message User facing error string
     */
    void onError(String message);
  }
}
