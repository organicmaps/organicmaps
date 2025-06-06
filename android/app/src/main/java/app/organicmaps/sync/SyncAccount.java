package app.organicmaps.sync;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.InvocationTargetException;
import java.util.Objects;
import org.json.JSONException;
import org.json.JSONObject;

public class SyncAccount
{
  private static final String TAG = SyncAccount.class.getSimpleName();
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
    return BackendType.idToBackendType.get(mBackendTypeId);
  }

  public AuthState getAuthState()
  {
    return mAuthState;
  }

  public static SyncAccount fromJson(JSONObject json) throws JSONException, IllegalArgumentException
  {
    int backendType = json.getInt(KEY_BACKEND_ID);
    AuthState authState;
    try
    {
      authState = Objects.requireNonNull(BackendType.idToBackendType.get(backendType))
                      .getAuthStateClass()
                      .getConstructor(JSONObject.class)
                      .newInstance(json.getJSONObject(KEY_AUTH_STATE));
    }
    catch (IllegalAccessException | InstantiationException | InvocationTargetException | NoSuchMethodException
           | NullPointerException e)
    {
      Logger.e(TAG, "Error creating SyncAccount object", e);
      throw new IllegalArgumentException(e);
    }
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
}
