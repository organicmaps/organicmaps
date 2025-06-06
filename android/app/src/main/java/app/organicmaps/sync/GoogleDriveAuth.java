package app.organicmaps.sync;

import android.content.Context;
import androidx.annotation.Nullable;
import org.json.JSONException;
import org.json.JSONObject;

public class GoogleDriveAuth extends AuthState
{
  public GoogleDriveAuth(JSONObject authStateJson) throws JSONException
  {
    super(authStateJson);
    throw new RuntimeException("TODO implement");
  }

  @Override
  public JSONObject toJson() throws JSONException
  {
    throw new RuntimeException("TODO implement");
  }

  @Override
  public boolean equals(AuthState other)
  {
    throw new RuntimeException("TODO implement");
  }

  @Override
  public String getUsername()
  {
    throw new RuntimeException("TODO implement");
  }

  @Override
  public String getBackendInfo(Context context)
  {
    throw new RuntimeException("TODO implement");
  }

  @Nullable
  @Override
  public Long getExpiryTimestamp()
  {
    throw new RuntimeException("TODO implement");
  }
}
