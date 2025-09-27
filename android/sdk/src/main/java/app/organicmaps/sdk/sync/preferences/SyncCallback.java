package app.organicmaps.sdk.sync.preferences;

import app.organicmaps.sdk.sync.SyncAccount;
import app.organicmaps.sdk.sync.SyncOpException;
import java.util.List;

public class SyncCallback
{
  public interface LastSync
  {
    void onLastSyncChanged(int accountId, long timestamp);
  }

  public interface ErrorInfo
  {
    void onSyncException(int accountId, SyncOpException exception);
  }

  public interface AccountsChanged
  {
    void onAccountsChanged(List<SyncAccount> newAccounts);
  }

  public interface AccountToggled
  {
    void onAccountEnabled(SyncAccount account);

    void onAccountDisabled(SyncAccount account);
  }

  public interface FrequencyChange
  {
    void onFrequencyChange(long syncIntervalMs);
  }
}
