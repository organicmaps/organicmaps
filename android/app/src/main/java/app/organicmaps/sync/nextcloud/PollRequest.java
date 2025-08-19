package app.organicmaps.sync.nextcloud;

import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import app.organicmaps.sdk.util.InsecureHttpsHelper;
import app.organicmaps.sdk.util.log.Logger;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.nio.charset.StandardCharsets;

public class PollRequest
{
  private static final String TAG = PollRequest.class.getSimpleName();

  /**
   * Polls to see whether the user has finished authentication or not. Returns .
   * Returns the response body if the poll parameters were consumed (i.e. user finished authenticated)
   * Successful response format: { "server": string, "loginName": string, "appPassword": string }
   * @return null if not or if errored
   */
  @Nullable
  @WorkerThread
  static String make(PollParams params)
  {
    try
    {
      HttpURLConnection connection = InsecureHttpsHelper.openInsecureConnection(params.endpoint());
      connection.setRequestMethod("POST");
      connection.setRequestProperty("Accept", "application/json");
      connection.setDoOutput(true);
      try (OutputStream os = connection.getOutputStream())
      {
        byte[] body = ("token=" + params.token()).getBytes(StandardCharsets.UTF_8);
        os.write(body, 0, body.length);
      }
      final int responseCode = connection.getResponseCode();
      Logger.i(TAG, "Poll endpoint returned response code " + responseCode);
      if (responseCode / 100 != 2)
        return null;
      StringBuilder response = new StringBuilder();
      try (BufferedReader in = new BufferedReader(new InputStreamReader(connection.getInputStream())))
      {
        String inputLine;
        while ((inputLine = in.readLine()) != null)
          response.append(inputLine);
      }
      String responseBody = response.toString();
      if (responseBody.isEmpty())
        return null;
      return responseBody;
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to poll Nextcloud auth status.", e);
      return null;
    }
  }
}
