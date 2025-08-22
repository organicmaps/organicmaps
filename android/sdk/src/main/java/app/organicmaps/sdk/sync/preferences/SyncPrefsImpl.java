package app.organicmaps.sdk.sync.preferences;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.sync.AuthState;
import app.organicmaps.sdk.sync.BackendType;
import app.organicmaps.sdk.sync.SyncAccount;
import app.organicmaps.sdk.sync.SyncOpException;
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

public class SyncPrefsImpl implements SyncPrefs
{
  private static final String TAG = SyncPrefsImpl.class.getSimpleName();

  private static final String PREF_NAME_ACCOUNTS = "SyncAccounts";

  private static final String PREF_KEY_ACCOUNTS = "ac";
  private static final String PREF_KEY_LAST_ACCOUNT_ID = "lastId";
  private static final String PREF_KEY_SYNC_INTERVAL = "interval";
  private static final String PREF_KEY_LAST_RUN = "lastRun";
  private static final String PREF_KEY_NC_POLL = "ncPollParams";

  private static final String PREF_KEY_PREFIX_ENABLED = "enabled-";
  private static final String PREF_KEY_PREFIX_LAST_SYNCED = "lastSynced-";
  private static final String PREF_KEY_PREFIX_ERROR_INFO = "syncError-";

  private static final long DEFAULT_SYNC_INTERVAL_MS = 24 * 3600_000; // 1 day
  @NonNull
  private final SharedPreferences prefsAccounts;

  private final CopyOnWriteArrayList<SyncAccount> mAccounts;
  private final CallbackCollection<SyncCallback.LastSync> mLastSyncCallbacks = new CallbackCollection<>();
  private final CallbackCollection<SyncCallback.ErrorInfo> mErrorInfoCallbacks = new CallbackCollection<>();
  private final CallbackCollection<SyncCallback.AccountsChanged> mAccountsChangedCallbacks = new CallbackCollection<>();
  private final CallbackCollection<SyncCallback.AccountToggled> mAccountToggledCallbacks = new CallbackCollection<>();
  private final CallbackCollection<SyncCallback.FrequencyChange> mFrequencyChangeCallbacks = new CallbackCollection<>();

  public SyncPrefsImpl(Context context)
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

  private String getPrefKeyEnabled(int accountId)
  {
    return PREF_KEY_PREFIX_ENABLED + accountId;
  }

  private String getPrefKeyLastSynced(int accountId)
  {
    return PREF_KEY_PREFIX_LAST_SYNCED + accountId;
  }

  private String getPrefKeyErrorInfo(int accountId)
  {
    return PREF_KEY_PREFIX_ERROR_INFO + accountId;
  }

  @Override
  public boolean isEnabled(int accountId)
  {
    return prefsAccounts.getBoolean(getPrefKeyEnabled(accountId), false);
  }

  @Override
  public void setEnabled(SyncAccount account, boolean enabled)
  {
    prefsAccounts.edit().putBoolean(getPrefKeyEnabled(account.getAccountId()), enabled).apply();
    mAccountToggledCallbacks.notifyAll(cb -> {
      if (enabled)
        cb.onAccountEnabled(account);
      else
        cb.onAccountDisabled(account);
    });
  }

  @Override
  public List<SyncAccount> getAccounts()
  {
    return Collections.unmodifiableList(mAccounts);
  }

  @Override
  public List<SyncAccount> getEnabledAccounts()
  {
    return mAccounts.stream()
        .filter(account -> isEnabled(account.getAccountId()))
        .collect(Collectors.toUnmodifiableList());
  }

  @Nullable
  @Override
  public Long getLastSynced(int accountId)
  {
    long lastSynced = prefsAccounts.getLong(getPrefKeyLastSynced(accountId), 0);
    return lastSynced != 0 ? lastSynced : null;
  }

  @Override
  public void setLastSynced(int accountId, long timestamp)
  {
    prefsAccounts.edit().putLong(getPrefKeyLastSynced(accountId), timestamp).apply();
    mLastSyncCallbacks.notifyAll(cb -> cb.onLastSyncChanged(accountId, timestamp));
  }

  @Nullable
  @Override
  public SyncOpException getErrorInfo(int accountId)
  {
    String errorString = prefsAccounts.getString(getPrefKeyErrorInfo(accountId), null);
    if (errorString == null)
      return null;
    try
    {
      return SyncOpException.fromJson(new JSONObject(errorString));
    }
    catch (JSONException e)
    {
      // Should be impossible
      Logger.e(TAG, "Error deserializing stored SyncOpException json string: " + errorString, e);
      return new SyncOpException.UnexpectedException();
    }
  }

  @Override
  public void setErrorInfo(int accountId, SyncOpException exception)
  {
    try
    {
      String jsonStr = exception.toJson().toString();
      prefsAccounts.edit().putString(getPrefKeyErrorInfo(accountId), jsonStr).apply();
    }
    catch (JSONException e)
    {
      // Should be impossible
      Logger.e(TAG,
               "Error serializing stored SyncOpException " + exception.getClass().getSimpleName() + ", "
                   + exception.getMessage(),
               e);
    }
    mErrorInfoCallbacks.notifyAll(cb -> cb.onSyncException(accountId, exception));
  }

  @Override
  public long getTimeSinceLastRun()
  {
    long timeElapsed = System.currentTimeMillis() - prefsAccounts.getLong(PREF_KEY_LAST_RUN, 0L);
    if (timeElapsed < 0) // The user's system date is in the past for some reason
      return Long.MAX_VALUE / 4;
    return timeElapsed;
  }

  @Override
  public void setLastRun(long timestamp)
  {
    prefsAccounts.edit().putLong(PREF_KEY_LAST_RUN, timestamp).apply();
  }

  @Override
  public AddAccountResult addAccount(BackendType backendType, AuthState authState)
  {
    try
    {
      for (SyncAccount account : mAccounts)
      {
        if (account.getAuthState().equals(authState))
          // TODO consider updating tokens to reset session expiry. Not needed for nextcloud.
          return AddAccountResult.AlreadyExists;
      }

      int accountId = generateNextAccountId();
      SyncAccount newAccount = new SyncAccount(accountId, backendType.getId(), authState);
      mAccounts.add(newAccount);
      Set<String> prefsSet = new HashSet<>(prefsAccounts.getStringSet(PREF_KEY_ACCOUNTS, Collections.emptySet()));
      prefsSet.add(newAccount.toJson().toString());
      prefsAccounts.edit().putStringSet(PREF_KEY_ACCOUNTS, prefsSet).apply();
      setEnabled(newAccount, false); // Newly added accounts are disabled by default.

      mAccountsChangedCallbacks.notifyAll(cb -> cb.onAccountsChanged(Collections.unmodifiableList(mAccounts)));
      return AddAccountResult.Success;
    }
    catch (JSONException e)
    {
      Logger.e(TAG, "Unexpected error adding a sync account to shared preferences", e);
      return AddAccountResult.UnexpectedError;
    }
  }

  @Override
  public void removeAccount(SyncAccount account)
  {
    setEnabled(account, false); // Must be called explicitly
    Set<String> prefsSet = new HashSet<>(prefsAccounts.getStringSet(PREF_KEY_ACCOUNTS, Collections.emptySet()));
    for (String accountStr : prefsSet)
    {
      SyncAccount storedAccount = null;
      try
      {
        storedAccount = SyncAccount.fromJson(new JSONObject(accountStr));
      }
      catch (JSONException e)
      {
        Logger.e(TAG, "Unable to deserialize an existing account from json string " + accountStr, e);
      }
      if (account.equals(storedAccount))
      {
        prefsSet.remove(accountStr);
        break;
      }
    }
    prefsAccounts.edit().putStringSet(PREF_KEY_ACCOUNTS, prefsSet).apply();
    mAccounts.remove(account);

    mAccountsChangedCallbacks.notifyAll(cb -> cb.onAccountsChanged(Collections.unmodifiableList(mAccounts)));

    // SyncState for this accountId should also be removed, but it could possibly
    // be in use by some ongoing sync operation.
    // That is low priority because it'd require adding quite a bit of code
    // only to save a couple of KBs on disk for every removed account. TODO(savsch)
  }

  @Override
  public void setSyncFrequency(long syncIntervalMs)
  {
    prefsAccounts.edit().putLong(PREF_KEY_SYNC_INTERVAL, syncIntervalMs).apply();
    mFrequencyChangeCallbacks.notifyAll(cb -> cb.onFrequencyChange(syncIntervalMs));
  }

  @Override
  public long getSyncIntervalMs()
  {
    return prefsAccounts.getLong(PREF_KEY_SYNC_INTERVAL, DEFAULT_SYNC_INTERVAL_MS);
  }

  private int generateNextAccountId()
  {
    int nextAccountId = prefsAccounts.getInt(PREF_KEY_LAST_ACCOUNT_ID, 0);
    prefsAccounts.edit().putInt(PREF_KEY_LAST_ACCOUNT_ID, nextAccountId + 1).apply();
    return nextAccountId;
  }

  @Override
  public void registerLastSyncedCallback(SyncCallback.LastSync callback)
  {
    mLastSyncCallbacks.register(callback);
  }

  @Override
  public void unregisterLastSyncedCallback(SyncCallback.LastSync callback)
  {
    mLastSyncCallbacks.unregister(callback);
  }

  @Override
  public void registerErrorInfoCallback(SyncCallback.ErrorInfo callback)
  {
    mErrorInfoCallbacks.register(callback);
  }

  @Override
  public void unregisterErrorInfoCallback(SyncCallback.ErrorInfo callback)
  {
    mErrorInfoCallbacks.unregister(callback);
  }

  @Override
  public void registerAccountsChangedCallback(SyncCallback.AccountsChanged callback)
  {
    mAccountsChangedCallbacks.register(callback);
  }

  @Override
  public void unregisterAccountsChangedCallback(SyncCallback.AccountsChanged callback)
  {
    mAccountsChangedCallbacks.unregister(callback);
  }

  @Override
  public void registerAccountToggledCallback(SyncCallback.AccountToggled callback)
  {
    mAccountToggledCallbacks.register(callback);
  }

  @Override
  public void registerFrequencyChangeCallback(SyncCallback.FrequencyChange callback)
  {
    mFrequencyChangeCallbacks.register(callback);
  }

  @Override
  public void unregisterFrequencyChangeCallback(SyncCallback.FrequencyChange callback)
  {
    mFrequencyChangeCallbacks.unregister(callback);
  }

  @Override
  public void setNextcloudPollParams(@Nullable String params)
  {
    prefsAccounts.edit().putString(PREF_KEY_NC_POLL, params).apply();
  }

  @Nullable
  @Override
  public String getNextcloudPollParams()
  {
    return prefsAccounts.getString(PREF_KEY_NC_POLL, null);
  }
}
