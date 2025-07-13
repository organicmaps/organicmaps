package app.organicmaps.sync;

import android.content.Context;
import androidx.annotation.Nullable;
import app.organicmaps.R;
import java.util.Objects;
import org.json.JSONException;
import org.json.JSONObject;

// {"rt": null|string (refresh token, nullable), "em": string (email), "sub": string (user identifier)}

public class GoogleDriveAuth extends AuthState
{
  private static final String KEY_REFRESH_TOKEN = "rt";
  private static final String KEY_EMAIL = "em";
  private static final String KEY_SUB = "sub";

  @Nullable
  private final String mRefreshToken;
  private final String mEmail;
  private final String mSub;

  public GoogleDriveAuth(JSONObject authStateJson) throws JSONException
  {
    super(authStateJson);
    mRefreshToken = authStateJson.isNull(KEY_REFRESH_TOKEN) ? null : authStateJson.getString(KEY_REFRESH_TOKEN);
    mEmail = authStateJson.getString(KEY_EMAIL);
    mSub = authStateJson.getString(KEY_SUB);
  }

  @Override
  public JSONObject toJson() throws JSONException
  {
    JSONObject json = new JSONObject();
    json.put(KEY_REFRESH_TOKEN, mRefreshToken == null ? JSONObject.NULL : mRefreshToken);
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
    return context.getString(R.string.google_drive);
  }

  @Nullable
  @Override
  public Long getExpiryTimestamp()
  {
    return null;
  }

  public static GoogleDriveAuth from(@Nullable String refreshToken, String email, String sub) throws JSONException
  {
    JSONObject json = new JSONObject();
    json.put(KEY_REFRESH_TOKEN, refreshToken == null ? JSONObject.NULL : refreshToken);
    json.put(KEY_EMAIL, email);
    json.put(KEY_SUB, sub);
    return new GoogleDriveAuth(json);
  }
}
