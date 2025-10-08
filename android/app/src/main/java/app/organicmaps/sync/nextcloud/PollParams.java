package app.organicmaps.sync.nextcloud;

import androidx.annotation.NonNull;
import org.json.JSONException;
import org.json.JSONObject;

public record PollParams(String token, String endpoint)
{
  private static final String KEY_TOKEN = "token";
  private static final String KEY_ENDPOINT = "endpoint";

  /// {"token": string, "endpoint": string }
  public static PollParams fromJson(JSONObject json) throws JSONException
  {
    return new PollParams(json.getString(KEY_TOKEN), json.getString(KEY_ENDPOINT));
  }

  public static PollParams fromString(String jsonString) throws JSONException
  {
    return fromJson(new JSONObject(jsonString));
  }

  @NonNull
  public String asString() throws JSONException
  {
    JSONObject json = new JSONObject();
    json.put(KEY_TOKEN, token);
    json.put(KEY_ENDPOINT, endpoint);
    return json.toString();
  }
}
