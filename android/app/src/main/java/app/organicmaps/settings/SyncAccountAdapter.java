package app.organicmaps.settings;

import android.content.Context;
import android.text.format.DateUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.appcompat.widget.SwitchCompat;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.sync.SyncAccount;
import app.organicmaps.sdk.sync.SyncManager;
import app.organicmaps.sdk.sync.SyncOpException;
import app.organicmaps.sdk.sync.preferences.SyncPrefs;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.sync.BackendUtils;
import java.text.DateFormat;
import java.util.Date;
import java.util.List;

public class SyncAccountAdapter extends RecyclerView.Adapter<SyncAccountAdapter.AccountViewHolder>
{
  private final OnAccountInteractionListener mListener;
  private List<SyncAccount> mAccountList;
  private RecyclerView mRecyclerView;

  public SyncAccountAdapter(List<SyncAccount> accountList, OnAccountInteractionListener listener)
  {
    this.mAccountList = accountList;
    this.mListener = listener;
  }

  @Override
  public void onAttachedToRecyclerView(@NonNull RecyclerView recyclerView)
  {
    super.onAttachedToRecyclerView(recyclerView);
    mRecyclerView = recyclerView;
  }

  @Override
  public void onDetachedFromRecyclerView(@NonNull RecyclerView recyclerView)
  {
    super.onDetachedFromRecyclerView(recyclerView);
    mRecyclerView = null;
  }

  @NonNull
  @Override
  public AccountViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_sync_account, parent, false);
    return new AccountViewHolder(view);
  }

  @Override
  public void onBindViewHolder(@NonNull AccountViewHolder holder, int position)
  {
    SyncAccount account = mAccountList.get(position);
    Context context = holder.itemView.getContext();
    SyncPrefs syncPrefs = SyncManager.INSTANCE.getPrefs();
    Long lastSynced = syncPrefs.getLastSynced(account.getAccountId());
    boolean syncEnabled = syncPrefs.isEnabled(account.getAccountId());

    holder.backendIcon.setImageDrawable(
        AppCompatResources.getDrawable(context, BackendUtils.getIcon(account.getBackendType())));
    holder.usernameText.setText(account.getAuthState().getUsername());
    holder.backendServerText.setText(account.getAuthState().getBackendInfo(context));
    holder.setState(syncEnabled, lastSynced, syncPrefs.getErrorInfo(account.getAccountId()));
    holder.accountEnabledSwitch.setOnCheckedChangeListener(null);
    holder.accountEnabledSwitch.setChecked(syncEnabled);

    // Set switch listener
    holder.accountEnabledSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
      if (mListener != null)
        mListener.onSwitchToggled(mAccountList.get(holder.getBindingAdapterPosition()), isChecked);
    });
    holder.accountInfo.setOnClickListener(v -> holder.accountEnabledSwitch.performClick());

    // Set click listener for the entire card
    holder.itemView.setOnClickListener(v -> {
      if (mListener != null)
        mListener.onAccountClicked(v.getContext(), mAccountList.get(holder.getBindingAdapterPosition()));
    });

    // Set long click listener for the entire card
    View.OnLongClickListener longClickListener = v ->
    {
      if (mListener != null)
        return mListener.onAccountLongClicked(mAccountList.get(holder.getBindingAdapterPosition()));
      return false;
    };
    holder.accountInfo.setOnLongClickListener(longClickListener);
    holder.itemView.setOnLongClickListener(longClickListener);
  }

  @Override
  public int getItemCount()
  {
    return mAccountList == null ? 0 : mAccountList.size();
  }

  public void updateAccounts(List<SyncAccount> accounts)
  {
    this.mAccountList = accounts;
    notifyDataSetChanged();
  }

  public void updateEnabled(int accountId, boolean enabled)
  {
    AccountViewHolder holder = getViewHolderForAccount(accountId);
    if (holder != null)
      holder.setEnabled(enabled);
  }

  public void updateLastSynced(int accountId, long timestamp)
  {
    AccountViewHolder holder = getViewHolderForAccount(accountId);
    if (holder != null)
      holder.setLastSynced(timestamp);
  }

  public void updateErrorInfo(int accountId, @Nullable SyncOpException exception)
  {
    AccountViewHolder holder = getViewHolderForAccount(accountId);
    if (holder != null)
      holder.setErrorInfo(exception);
  }

  @Nullable
  private AccountViewHolder getViewHolderForAccount(int accountId)
  {
    if (mRecyclerView == null)
      return null;
    for (int i = 0; i < mAccountList.size(); ++i)
    {
      if (mAccountList.get(i).getAccountId() == accountId)
      {
        RecyclerView.ViewHolder holder = mRecyclerView.findViewHolderForAdapterPosition(i);
        if (holder instanceof AccountViewHolder)
          return (AccountViewHolder) holder;
      }
    }
    return null;
  }

  public interface OnAccountInteractionListener
  {
    void onSwitchToggled(SyncAccount account, boolean isChecked);

    void onAccountClicked(Context context, SyncAccount account);

    boolean onAccountLongClicked(SyncAccount account);
  }

  public static class AccountViewHolder extends RecyclerView.ViewHolder
  {
    private static final String TAG = AccountViewHolder.class.getSimpleName();
    final ImageView backendIcon;
    final TextView usernameText;
    final TextView backendServerText;
    final SwitchCompat accountEnabledSwitch;
    final View accountInfo;
    final TextView syncStatusText;
    final TextView errorStatusText;

    boolean mSyncEnabled;
    @Nullable
    Long mLastSynced = null; // milliseconds, null if never synced
    @Nullable
    SyncOpException mErrorInfo = null;

    public AccountViewHolder(@NonNull View itemView)
    {
      super(itemView);

      backendIcon = itemView.findViewById(R.id.iv_account);
      usernameText = itemView.findViewById(R.id.tv_username);
      backendServerText = itemView.findViewById(R.id.tv_backend);
      accountEnabledSwitch = itemView.findViewById(R.id.switch_enable_sync);
      syncStatusText = itemView.findViewById(R.id.tv_sync_status);
      errorStatusText = itemView.findViewById(R.id.tv_error_status);
      accountInfo = itemView.findViewById(R.id.account_info);
    }

    public void refreshViews()
    {
      if (mSyncEnabled)
      {
        String lastSyncedStr;
        if (mLastSynced == null)
        {
          lastSyncedStr = syncStatusText.getContext().getString(R.string.power_managment_setting_never);
          if (mErrorInfo != null)
            showErrorInfo();
          else
            hideErrorInfo();
        }
        else
        {
          // To avoid having to periodically refresh the relative time in case of recent sync,
          //   show relative time only if last sync occurred more than an hour ago,
          //   else show the absolute time
          long currentTime = System.currentTimeMillis();
          if (currentTime - mLastSynced >= DateUtils.HOUR_IN_MILLIS)
            lastSyncedStr = DateUtils
                                .getRelativeTimeSpanString(mLastSynced, currentTime, DateUtils.MINUTE_IN_MILLIS,
                                                           DateUtils.FORMAT_ABBREV_RELATIVE)
                                .toString();
          else
            lastSyncedStr = app.organicmaps.sdk.util.DateUtils.getMediumTimeFormatter().format(new Date(mLastSynced));

          if (mErrorInfo != null && mErrorInfo.getTimestampMs() > mLastSynced)
            showErrorInfo();
          else
            hideErrorInfo();
        }
        syncStatusText.setText(syncStatusText.getContext().getString(R.string.last_synced_time, lastSyncedStr));
      }
      else
      {
        syncStatusText.setText(R.string.sync_disabled);
        hideErrorInfo();
      }
    }

    private void showErrorInfo()
    {
      if (mErrorInfo == null)
      {
        hideErrorInfo();
        return;
      }

      Context context = errorStatusText.getContext();
      String errorMessage = DateFormat.getDateTimeInstance().format(new Date(mErrorInfo.getTimestampMs())) + " : ";

      if (mErrorInfo instanceof SyncOpException.NetworkException)
        errorMessage += context.getString(R.string.error_connecting_to_server);
      else if (mErrorInfo instanceof SyncOpException.AuthExpiredException)
        errorMessage += context.getString(R.string.authorization_expired);
      else if (mErrorInfo instanceof SyncOpException.UnexpectedException)
      {
        errorMessage += context.getString(R.string.unexpected_error);
        if (mErrorInfo.getMessage() != null)
          errorMessage += " – " + mErrorInfo.getMessage(); // Such exceptions are supposed to be rare
      }
      else
      {
        errorMessage += context.getString(R.string.unexpected_error);
        Logger.e(TAG, "Error type formatting unimplemented for " + mErrorInfo.getClass().getSimpleName());
      }
      errorStatusText.setText(errorMessage);
      errorStatusText.setVisibility(View.VISIBLE);
    }

    private void hideErrorInfo()
    {
      errorStatusText.setVisibility(View.GONE);
    }

    public void setState(boolean syncEnabled, Long lastSynced, SyncOpException errorInfo)
    {
      mErrorInfo = errorInfo;
      mSyncEnabled = syncEnabled;
      mLastSynced = lastSynced;
      refreshViews();
    }

    public void setErrorInfo(@Nullable SyncOpException exception)
    {
      mErrorInfo = exception;
      refreshViews();
    }

    public void setEnabled(boolean enabled)
    {
      mSyncEnabled = enabled;
      refreshViews();
    }

    public void setLastSynced(long timestamp)
    {
      mLastSynced = timestamp;
      refreshViews();
    }
  }
}
