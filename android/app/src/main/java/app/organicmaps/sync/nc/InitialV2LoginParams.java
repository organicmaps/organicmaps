package app.organicmaps.sync.nc;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class InitialV2LoginParams
{
  private final String pollToken;
  private final String pollEndpoint;
  private final String loginUrl;

  /**
   * @param rawServerJsonResponse Raw string of a json of the form { "poll": {"token": string, "endpoint": string }, "login": string }
   */
  public InitialV2LoginParams(String rawServerJsonResponse) throws JSONException
  {
    JSONObject jsonResponse = new JSONObject(rawServerJsonResponse);
    JSONObject pollObject = jsonResponse.getJSONObject("poll");
    this.pollToken = pollObject.getString("token");
    this.pollEndpoint = pollObject.getString("endpoint");
    this.loginUrl = jsonResponse.getString("login");
  }

  public String getPollToken()
  {
    return pollToken;
  }

  public String getPollEndpoint()
  {
    return pollEndpoint;
  }

  public String getLoginUrl()
  {
    return loginUrl;
  }
}
