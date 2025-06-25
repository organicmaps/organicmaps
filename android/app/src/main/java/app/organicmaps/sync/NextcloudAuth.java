package app.organicmaps.sync;

import android.content.Context;
import androidx.annotation.Nullable;
import java.util.Objects;
import org.json.JSONException;
import org.json.JSONObject;

// {"server": string, "loginName": string, "appPassword": string}

public class NextcloudAuth extends AuthState
{
  private static final String KEY_SERVER = "server";
  private static final String KEY_LOGIN_NAME = "loginName";
  private static final String KEY_APP_PASSWORD = "appPassword";

  private final String mServer;
  private final String mLoginName;
  private final String mAppPassword;

  public NextcloudAuth(JSONObject authStateJson) throws JSONException
  {
    super(authStateJson);
    mServer = authStateJson.getString(KEY_SERVER);
    mLoginName = authStateJson.getString(KEY_LOGIN_NAME);
    mAppPassword = authStateJson.getString(KEY_APP_PASSWORD);
  }

  @Override
  public JSONObject toJson() throws JSONException
  {
    JSONObject json = new JSONObject();
    json.put(KEY_SERVER, mServer);
    json.put(KEY_LOGIN_NAME, mLoginName);
    json.put(KEY_APP_PASSWORD, mAppPassword);
    return json;
  }

  @Override
  public boolean equals(AuthState other)
  {
    if (other instanceof NextcloudAuth)
      return Objects.equals(mServer, ((NextcloudAuth) other).mServer)
   && Objects.equals(mLoginName, ((NextcloudAuth) other).mLoginName);
    return false;
  }

  @Override
  public String getUsername()
  {
    return mLoginName;
  }

  @Override
  public String getBackendInfo(Context context)
  {
    return mServer;
  }

  @Nullable
  @Override
  public Long getExpiryTimestamp()
  {
    return null;
  }

  String getLoginName()
  {
    return mLoginName;
  }

  String getAppPassword()
  {
    return mAppPassword;
  }

  String getServer()
  {
    return mServer;
  }
}
