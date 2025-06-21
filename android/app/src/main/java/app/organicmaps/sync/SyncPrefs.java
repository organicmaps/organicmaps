package app.organicmaps.sync;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.stream.Collectors;
import org.json.JSONException;
import org.json.JSONObject;

public class SyncPrefs
{
  private static final String TAG = SyncPrefs.class.getSimpleName();

  private static final String PREF_NAME_ACCOUNTS = "SyncAccounts";
  private static final String PREF_NAME_STATE = "SyncState";

  private static final String PREF_KEY_ACCOUNTS = "ac";
  private static final String PREF_KEY_GOOGLE_OAUTH_PARAMS = "googleOAuthUri";
  private static final String PREF_KEY_LAST_ACCOUNT_ID = "lastId";
  private static final String PREF_KEY_PREFIX_ENABLED = "enabled-";
  private static final String PREF_KEY_PREFIX_LAST_SYNCED = "lastSynced-";
  @Nullable
  private static SyncPrefs instance;
  @NonNull
  private final SharedPreferences prefsAccounts;
  @NonNull
  private final SharedPreferences prefsState;

  private final CopyOnWriteArrayList<SyncAccount> mAccounts;
  private final Set<LastSyncCallback> mLastSyncCallbacks = new HashSet<>();
  private final Set<AccountsChangedCallback> mAccountsChangedCallbacks = new HashSet<>();

  private SyncPrefs(Context context)
  {
    prefsAccounts = context.getSharedPreferences(PREF_NAME_ACCOUNTS, Context.MODE_PRIVATE);
    prefsState = context.getSharedPreferences(PREF_NAME_STATE, Context.MODE_PRIVATE);

    Set<String> storedAccounts = prefsAccounts.getStringSet(PREF_KEY_ACCOUNTS, Collections.emptySet());
    mAccounts = new CopyOnWriteArrayList<>(storedAccounts.stream()
                                               .map(accountStr -> {
                                                 try
                                                 {
                                                   return SyncAccount.fromJson(new JSONObject(accountStr));
                                                 }
                                                 catch (JSONException e)
                                                 {
                                                   Logger.e(TAG, "Invalid stored account JSON: " + accountStr, e);
                                                   return null;
                                                 }
                                               })
                                               .filter(Objects::nonNull)
                                               .collect(Collectors.toList()));
  }

  public static SyncPrefs getInstance(Context context)
  {
    if (instance == null)
      instance = new SyncPrefs(context.getApplicationContext());
    return instance;
  }

  private String getPrefKeyEnabled(long accountId)
  {
    return PREF_KEY_PREFIX_ENABLED + accountId;
  }

  private String getPrefKeyLastSynced(long accountId)
  {
    return PREF_KEY_PREFIX_LAST_SYNCED + accountId;
  }

  public boolean isEnabled(long accountId)
  {
    return prefsAccounts.getBoolean(getPrefKeyEnabled(accountId), true); // sync is enabled on an account by default
  }

  public void setEnabled(long accountId, boolean enabled)
  {
    prefsAccounts.edit()
        .putBoolean(getPrefKeyEnabled(accountId), enabled)
        .apply(); // TODO add auth expiry checks if needed
  }

  public List<SyncAccount> getAccounts()
  {
    return Collections.unmodifiableList(mAccounts);
  }

  /**
   * @return timestamp in milliseconds
   */
  public @Nullable Long getLastSynced(long accountId)
  {
    long lastSynced = prefsAccounts.getLong(getPrefKeyLastSynced(accountId), 0);
    return lastSynced != 0 ? lastSynced : null;
  }

  /**
   * @param timestamp milliseconds
   */
  public void setLastSynced(long accountId, long timestamp)
  {
    prefsAccounts.edit().putLong(getPrefKeyLastSynced(accountId), timestamp).apply();
    for (LastSyncCallback callback : mLastSyncCallbacks)
    {
      try
      {
        callback.onLastSyncChanged(accountId, timestamp);
      }
      catch (Exception ignored)
      {}
    }
  }

  public AddAccountResult addAccount(BackendType backendType, AuthState authState)
  {
    try
    {
      for (SyncAccount account : mAccounts)
      {
        if (account.getAuthState().equals(authState))
          return AddAccountResult.AlreadyExists; // TODO consider updating tokens to reset session expiry
      }

      SyncAccount newAccount = new SyncAccount(generateNextAccountId(), backendType.getId(), authState);
      mAccounts.add(newAccount);
      Set<String> prefsSet = new HashSet<>(prefsAccounts.getStringSet(PREF_KEY_ACCOUNTS, Collections.emptySet()));
      prefsSet.add(newAccount.toJson().toString());
      prefsAccounts.edit().putStringSet(PREF_KEY_ACCOUNTS, prefsSet).apply();
      for (AccountsChangedCallback callback : mAccountsChangedCallbacks)
      {
        try
        {
          callback.onAccountsChanged(Collections.unmodifiableList(mAccounts));
        }
        catch (Exception ignored)
        {}
      }
      return AddAccountResult.Success;
    }
    catch (JSONException e)
    {
      Logger.e(TAG, "Unexpected error adding a sync account to shared preferences", e);
      return AddAccountResult.UnexpectedError;
    }
  }

  private long generateNextAccountId()
  {
    long nextAccountId = prefsAccounts.getLong(PREF_KEY_LAST_ACCOUNT_ID, 0L);
    prefsAccounts.edit().putLong(PREF_KEY_LAST_ACCOUNT_ID, nextAccountId + 1).apply();
    return nextAccountId;
  }

  public void registerLastSyncedCallback(LastSyncCallback callback)
  {
    mLastSyncCallbacks.add(callback);
  }

  public void unregisterLastSyncedCallback(LastSyncCallback callback)
  {
    mLastSyncCallbacks.remove(callback);
  }

  public void registerAccountsChangedCallback(AccountsChangedCallback callback)
  {
    mAccountsChangedCallbacks.add(callback);
  }

  public void unregisterAccountsChangedCallback(AccountsChangedCallback callback)
  {
    mAccountsChangedCallbacks.remove(callback);
  }

  public void setGoogleOauthParams(String params)
  {
    prefsAccounts.edit().putString(PREF_KEY_GOOGLE_OAUTH_PARAMS, params).apply();
  }

  public @Nullable String getGoogleOauthParams()
  {
    return prefsAccounts.getString(PREF_KEY_GOOGLE_OAUTH_PARAMS, null);
  }

  public interface LastSyncCallback
  {
    void onLastSyncChanged(long accountId, long timestamp);
  }

  public interface AccountsChangedCallback
  {
    void onAccountsChanged(List<SyncAccount> newAccounts);
  }

  public enum AddAccountResult
  {
    Success,
    AlreadyExists,
    UnexpectedError
  }
}
