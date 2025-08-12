package app.organicmaps.sdk.sync;

import androidx.annotation.NonNull;
import java.util.Objects;
import org.json.JSONException;
import org.json.JSONObject;

public class SyncAccount
{
  private static final String KEY_ACCOUNT_ID = "id";
  private static final String KEY_BACKEND_ID = "back";
  private static final String KEY_AUTH_STATE = "auth";

  private final long mAccountId;
  private final int mBackendTypeId;
  private final AuthState mAuthState;

  public SyncAccount(long accountId, int backendTypeId, AuthState authState)
  {
    mAccountId = accountId;
    mBackendTypeId = backendTypeId;
    mAuthState = authState;
  }

  public long getAccountId()
  {
    return mAccountId;
  }

  @NonNull
  public BackendType getBackendType()
  {
    return Objects.requireNonNull(BackendType.idToBackendType.get(mBackendTypeId));
  }

  public AuthState getAuthState()
  {
    return mAuthState;
  }

  public SyncClient createSyncClient()
  {
    return switch (Objects.requireNonNull(BackendType.idToBackendType.get(mBackendTypeId)))
    {
      case Nextcloud -> new NextcloudSyncClient((NextcloudAuth) mAuthState);
    };
  }

  public static SyncAccount fromJson(JSONObject json) throws JSONException
  {
    int backendType = json.getInt(KEY_BACKEND_ID);
    AuthState authState = switch (Objects.requireNonNull(BackendType.idToBackendType.get(backendType)))
    {
      case Nextcloud -> new NextcloudAuth(json.getJSONObject(KEY_AUTH_STATE));
    };
    return new SyncAccount(json.getLong(KEY_ACCOUNT_ID), backendType, authState);
  }

  public JSONObject toJson() throws JSONException
  {
    JSONObject json = new JSONObject();
    json.put(KEY_ACCOUNT_ID, mAccountId);
    json.put(KEY_BACKEND_ID, mBackendTypeId);
    json.put(KEY_AUTH_STATE, mAuthState.toJson());
    return json;
  }

  @Override
  public boolean equals(Object obj)
  {
    if (this == obj)
      return true;
    if (obj == null || getClass() != obj.getClass())
      return false;
    return getAccountId() == ((SyncAccount) obj).getAccountId();
  }

  @Override
  public int hashCode()
  {
    return Objects.hash(getAccountId());
  }
}
