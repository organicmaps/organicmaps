package app.organicmaps.sync;

import android.app.Application;
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

  private static final String PREF_KEY_ACCOUNTS = "ac";
  private static final String PREF_KEY_LAST_ACCOUNT_ID = "lastId";
  private static final String PREF_KEY_SYNC_INTERVAL = "interval";
  private static final String PREF_KEY_LAST_RUN = "lastRun";
  private static final String PREF_KEY_NC_POLL = "ncPollParams";
  private static final String PREF_KEY_PREFIX_ENABLED = "enabled-";
  private static final String PREF_KEY_PREFIX_LAST_SYNCED = "lastSynced-";
  private static final long DEFAULT_SYNC_INTERVAL_MS = 24 * 3600_000;
  @Nullable
  private static SyncPrefs instance;
  @NonNull
  private final SharedPreferences prefsAccounts;

  private final CopyOnWriteArrayList<SyncAccount> mAccounts;
  private final Set<LastSyncCallback> mLastSyncCallbacks = new HashSet<>();
  private final Set<AccountsChangedCallback> mAccountsChangedCallbacks = new HashSet<>();
  private final Set<AccountToggledCallback> mAccountToggledCallbacks = new HashSet<>();
  private final Set<FrequencyChangeCallback> mFrequencyChangeCallbacks = new HashSet<>();

  private SyncPrefs(Application context)
  {
    prefsAccounts = context.getSharedPreferences(PREF_NAME_ACCOUNTS, Context.MODE_PRIVATE);
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
      instance = new SyncPrefs((Application) context.getApplicationContext());
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
    return prefsAccounts.getBoolean(getPrefKeyEnabled(accountId), false);
  }

  public void setEnabled(SyncAccount account, boolean enabled)
  {
    prefsAccounts.edit()
        .putBoolean(getPrefKeyEnabled(account.getAccountId()), enabled)
        .apply(); // TODO add auth expiry checks if needed
    for (AccountToggledCallback callback : mAccountToggledCallbacks)
    {
      try
      {
        if (enabled)
          callback.onAccountEnabled(account);
        else
          callback.onAccountDisabled(account);
      }
      catch (Exception ignored)
      {}
    }
  }

  public List<SyncAccount> getAccounts()
  {
    return Collections.unmodifiableList(mAccounts);
  }

  public List<SyncAccount> getEnabledAccounts()
  {
    return mAccounts.stream()
        .filter(account -> isEnabled(account.getAccountId()))
        .collect(Collectors.toUnmodifiableList());
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

  /**
   * @see #setLastRun(long)
   * @return Time elapsed in ms.
   */
  public long getTimeSinceLastRun()
  {
    long timeElapsed = System.currentTimeMillis() - prefsAccounts.getLong(PREF_KEY_LAST_RUN, 0L);
    if (timeElapsed < 0) // The user's system date is in the past for some reason
      return Long.MAX_VALUE / 4;
    return timeElapsed;
  }

  /**
   * Unlike {@link #setLastSynced(long, long)}, this is not account-specific.
   * It stores only the time when the sync routine was last run, regardless
   * of whether it succeeded or not.
   * @param timestamp in milliseconds
   */
  public void setLastRun(long timestamp)
  {
    prefsAccounts.edit().putLong(PREF_KEY_LAST_RUN, timestamp).apply();
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

      long accountId = generateNextAccountId();
      SyncAccount newAccount = new SyncAccount(accountId, backendType.getId(), authState);
      mAccounts.add(newAccount);
      Set<String> prefsSet = new HashSet<>(prefsAccounts.getStringSet(PREF_KEY_ACCOUNTS, Collections.emptySet()));
      prefsSet.add(newAccount.toJson().toString());
      prefsAccounts.edit().putStringSet(PREF_KEY_ACCOUNTS, prefsSet).apply();
      setEnabled(newAccount, false); // Newly added accounts are disabled by default.
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

  public void setSyncFrequency(long syncIntervalMs)
  {
    prefsAccounts.edit().putLong(PREF_KEY_SYNC_INTERVAL, syncIntervalMs).apply();
    for (FrequencyChangeCallback callback : mFrequencyChangeCallbacks)
    {
      try
      {
        callback.onFrequencyChange(syncIntervalMs);
      }
      catch (Exception ignored)
      {}
    }
  }

  public long getSyncIntervalMs()
  {
    return prefsAccounts.getLong(PREF_KEY_SYNC_INTERVAL, DEFAULT_SYNC_INTERVAL_MS);
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

  public void registerAccountToggledCallback(AccountToggledCallback callback)
  {
    mAccountToggledCallbacks.add(callback);
  }

  public void registerFrequencyChangeCallback(FrequencyChangeCallback callback)
  {
    mFrequencyChangeCallbacks.add(callback);
  }

  public void unregisterFrequencyChangeCallback(FrequencyChangeCallback callback)
  {
    mFrequencyChangeCallbacks.remove(callback);
  }

  public void setNextcloudPollParams(@Nullable String params)
  {
    prefsAccounts.edit().putString(PREF_KEY_NC_POLL, params).apply();
  }

  public @Nullable String getNextcloudPollParams()
  {
    return prefsAccounts.getString(PREF_KEY_NC_POLL, null);
  }

  public interface LastSyncCallback
  {
    void onLastSyncChanged(long accountId, long timestamp);
  }

  public interface AccountsChangedCallback
  {
    void onAccountsChanged(List<SyncAccount> newAccounts);
  }

  public interface AccountToggledCallback
  {
    void onAccountEnabled(SyncAccount account);

    void onAccountDisabled(SyncAccount account);
  }

  public interface FrequencyChangeCallback
  {
    void onFrequencyChange(long syncIntervalMs);
  }

  public enum AddAccountResult
  {
    Success,
    AlreadyExists,
    UnexpectedError
  }
}
