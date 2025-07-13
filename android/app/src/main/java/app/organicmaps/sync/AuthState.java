package app.organicmaps.sync;

import android.content.Context;
import androidx.annotation.Nullable;
import org.json.JSONException;
import org.json.JSONObject;

public abstract class AuthState
{
  public AuthState(JSONObject authStateJson) throws JSONException {}

  public abstract JSONObject toJson() throws JSONException;

  /**
   * @param other The AuthState to compare with.
   * @return whether the two AuthStates refer to the same account (point to the same cloud directory).
   */
  public abstract boolean equals(AuthState other);

  /**
   * @return a user-facing String representing the username
   */
  public abstract String getUsername();

  /**
   * @return a user-facing String to describe the backend type
   */
  public abstract String getBackendInfo(Context context);

  /**
   * @return Auth expiry timestamp in seconds; null if not applicable.
   */
  public abstract @Nullable Long getExpiryTimestamp();
}
