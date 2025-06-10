package app.organicmaps.sync;

import android.content.Context;
import androidx.annotation.Nullable;
import app.organicmaps.R;
import java.util.Objects;
import org.json.JSONException;
import org.json.JSONObject;

// {"rt": string, "em": string}

public class GoogleDriveAuth extends AuthState
{
  private static final String KEY_REFRESH_TOKEN = "rt";
  private static final String KEY_EMAIL = "em";

  private final String mRefreshToken;
  private final String mEmail;

  public GoogleDriveAuth(JSONObject authStateJson) throws JSONException
  {
    super(authStateJson);
    mRefreshToken = authStateJson.getString(KEY_REFRESH_TOKEN);
    mEmail = authStateJson.getString(KEY_EMAIL);
  }

  @Override
  public JSONObject toJson() throws JSONException
  {
    JSONObject json = new JSONObject();
    json.put(KEY_REFRESH_TOKEN, mRefreshToken);
    json.put(KEY_EMAIL, mEmail);
    return json;
  }

  @Override
  public boolean equals(AuthState other)
  {
    if (other instanceof GoogleDriveAuth)
      return Objects.equals(mEmail, ((GoogleDriveAuth) other).mEmail);
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

  public static GoogleDriveAuth fromTokenAndEmail(String refreshToken, String email) throws JSONException
  {
    JSONObject json = new JSONObject();
    json.put(KEY_REFRESH_TOKEN, refreshToken);
    json.put(KEY_EMAIL, email);
    return new GoogleDriveAuth(json);
  }
}
