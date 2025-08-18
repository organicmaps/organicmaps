package app.organicmaps.sdk.sync;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.sync.engine.SyncClient;
import java.util.Objects;
import org.json.JSONException;
import org.json.JSONObject;

public class SyncAccount
{
  private static final String KEY_ACCOUNT_ID = "id";
  private static final String KEY_BACKEND_ID = "back";
  private static final String KEY_AUTH_STATE = "auth";

  private final int mAccountId;
  private final int mBackendTypeId;
  private final AuthState mAuthState;

  public SyncAccount(int accountId, int backendTypeId, AuthState authState)
  {
    mAccountId = accountId;
    mBackendTypeId = backendTypeId;
    mAuthState = authState;
  }

  public int getAccountId()
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
      // TODO (PR #10651)
      default -> null;
    };
  }

  public static SyncAccount fromJson(JSONObject json) throws JSONException
  {
    int backendType = json.getInt(KEY_BACKEND_ID);
    AuthState authState = switch (Objects.requireNonNull(BackendType.idToBackendType.get(backendType)))
    {
      // TODO (PR #10651)
      default -> null;
    };
    return new SyncAccount(json.getInt(KEY_ACCOUNT_ID), backendType, authState);
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
