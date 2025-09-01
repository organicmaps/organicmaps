package app.organicmaps.sdk.sync.googledrive;

import android.content.Context;
import app.organicmaps.sdk.sync.AuthState;
import java.util.Objects;
import org.json.JSONException;
import org.json.JSONObject;

// {"rt": string (refresh token), "em": string (email), "sb": string (user identifier)}

public class GoogleDriveAuth extends AuthState
{
  private static final String KEY_REFRESH_TOKEN = "rt";
  private static final String KEY_EMAIL = "em";
  private static final String KEY_SUB = "sb";

  private final String mRefreshToken;
  private final String mEmail;
  private final String mSub;

  public GoogleDriveAuth(JSONObject authStateJson) throws JSONException
  {
    super(authStateJson);
    mRefreshToken = authStateJson.getString(KEY_REFRESH_TOKEN);
    mEmail = authStateJson.getString(KEY_EMAIL);
    mSub = authStateJson.getString(KEY_SUB);
  }

  @Override
  public JSONObject toJson() throws JSONException
  {
    JSONObject json = new JSONObject();
    json.put(KEY_REFRESH_TOKEN, mRefreshToken);
    json.put(KEY_EMAIL, mEmail);
    json.put(KEY_SUB, mSub);
    return json;
  }

  @Override
  public boolean equals(AuthState other)
  {
    if (other instanceof GoogleDriveAuth)
      return Objects.equals(mSub, ((GoogleDriveAuth) other).mSub);
    return false;
  }

  @Override
  public String getUsername()
  {
    return mEmail;
  }

  @Override
  public String getBackendInfo(Context context)
  {
    return mEmail.substring(mEmail.indexOf("@") + 1);
  }

  public String getRefreshToken()
  {
    return mRefreshToken;
  }

  public String getSubject()
  {
    return mSub;
  }

  public static GoogleDriveAuth from(String refreshToken, String email, String sub) throws JSONException
  {
    JSONObject json = new JSONObject();
    json.put(KEY_REFRESH_TOKEN, refreshToken);
    json.put(KEY_EMAIL, email);
    json.put(KEY_SUB, sub);
    return new GoogleDriveAuth(json);
  }
}
