package app.organicmaps.sdk.sync.preferences;

import androidx.annotation.Nullable;
import app.organicmaps.sdk.sync.AuthState;
import app.organicmaps.sdk.sync.BackendType;
import app.organicmaps.sdk.sync.SyncAccount;
import app.organicmaps.sdk.sync.SyncOpException;
import java.util.List;

public interface SyncPrefs
{
  boolean isEnabled(int accountId);

  void setEnabled(SyncAccount account, boolean enabled);

  List<SyncAccount> getAccounts();

  List<SyncAccount> getEnabledAccounts();

  /**
   * @return timestamp in milliseconds
   */
  @Nullable
  Long getLastSynced(int accountId);

  /**
   * @param timestamp milliseconds
   */
  void setLastSynced(int accountId, long timestamp);

  @Nullable
  SyncOpException getErrorInfo(int accountId);

  void setErrorInfo(int accountId, SyncOpException exception);

  /**
   * @see #setLastRun(long)
   * @return Time elapsed in ms.
   */
  long getTimeSinceLastRun();

  /**
   * Unlike {@link #setLastSynced(int, long)}, this is not account-specific.
   * It stores only the time when the sync routine was last run, regardless
   * of whether it succeeded or not.
   * @param timestamp in milliseconds
   */
  void setLastRun(long timestamp);

  AddAccountResult addAccount(BackendType backendType, AuthState authState);

  void removeAccount(SyncAccount account);

  void setSyncFrequency(long syncIntervalMs);

  long getSyncIntervalMs();

  void registerLastSyncedCallback(SyncCallback.LastSync callback);

  void unregisterLastSyncedCallback(SyncCallback.LastSync callback);

  void registerErrorInfoCallback(SyncCallback.ErrorInfo callback);

  void unregisterErrorInfoCallback(SyncCallback.ErrorInfo callback);

  void registerAccountsChangedCallback(SyncCallback.AccountsChanged callback);

  void unregisterAccountsChangedCallback(SyncCallback.AccountsChanged callback);

  void registerAccountToggledCallback(SyncCallback.AccountToggled callback);

  void registerFrequencyChangeCallback(SyncCallback.FrequencyChange callback);

  void unregisterFrequencyChangeCallback(SyncCallback.FrequencyChange callback);

  void setNextcloudPollParams(@Nullable String params);

  @Nullable
  String getNextcloudPollParams();
}
