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
import androidx.appcompat.widget.SwitchCompat;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sync.SyncAccount;
import app.organicmaps.sync.SyncPrefs;
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
    SyncPrefs syncPrefs = SyncPrefs.getInstance(context);
    Long lastSynced = syncPrefs.getLastSynced(account.getAccountId());
    boolean syncEnabled = syncPrefs.isEnabled(account.getAccountId());

    holder.backendIcon.setImageDrawable(account.getBackendType().getIcon(context));
    holder.usernameText.setText(account.getAuthState().getUsername());
    holder.backendServerText.setText(account.getAuthState().getBackendInfo(context));
    holder.setSyncStatus(syncEnabled, lastSynced);
    holder.accountEnabledSwitch.setOnCheckedChangeListener(null);
    holder.accountEnabledSwitch.setChecked(syncEnabled);
    holder.authExpiryStatusText.setVisibility(View.GONE); // TODO decide when to show this

    // Set switch listener
    holder.accountEnabledSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
      if (mListener != null)
      {
        mListener.onSwitchToggled(mAccountList.get(holder.getBindingAdapterPosition()).getAccountId(), isChecked);
      }
    });

    // Set click listener for the entire item
    holder.itemView.setOnClickListener(v -> {
      if (mListener != null)
      {
        mListener.onAccountClicked(mAccountList.get(holder.getBindingAdapterPosition()).getAccountId());
      }
    });
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

  public void updateSyncStatusText(long accountId, boolean syncEnabled, @Nullable Long lastSynced)
  {
    if (mRecyclerView == null)
      return;
    for (int i = 0; i < mAccountList.size(); ++i)
    {
      if (mAccountList.get(i).getAccountId() == accountId)
      {
        RecyclerView.ViewHolder holder = mRecyclerView.findViewHolderForAdapterPosition(i);
        if (holder instanceof AccountViewHolder)
          ((AccountViewHolder) holder).setSyncStatus(syncEnabled, lastSynced);
      }
    }
  }

  public interface OnAccountInteractionListener
  {
    void onSwitchToggled(long accountId, boolean isChecked);

    void onAccountClicked(long accountId);
  }

  public static class AccountViewHolder extends RecyclerView.ViewHolder
  {
    ImageView backendIcon;
    TextView usernameText;
    TextView backendServerText;
    SwitchCompat accountEnabledSwitch;
    TextView syncStatusText;
    TextView authExpiryStatusText;

    public AccountViewHolder(@NonNull View itemView)
    {
      super(itemView);

      backendIcon = itemView.findViewById(R.id.iv_account);
      usernameText = itemView.findViewById(R.id.tv_username);
      backendServerText = itemView.findViewById(R.id.tv_backend);
      accountEnabledSwitch = itemView.findViewById(R.id.switch_enable_sync);
      syncStatusText = itemView.findViewById(R.id.tv_sync_status);
      authExpiryStatusText = itemView.findViewById(R.id.tv_auth_status);
    }

    public void setSyncStatus(boolean syncEnabled, @Nullable Long lastSynced)
    {
      if (syncEnabled)
        syncStatusText.setText(syncStatusText.getContext().getString(
            R.string.last_synced_relative_time,
            lastSynced == null
                ? syncStatusText.getContext().getString(R.string.power_managment_setting_never)
                : DateUtils
                      .getRelativeTimeSpanString(lastSynced, System.currentTimeMillis(), DateUtils.MINUTE_IN_MILLIS,
                                                 DateUtils.FORMAT_ABBREV_RELATIVE)
                      .toString()));
      else
        syncStatusText.setText(R.string.sync_disabled);
    }
  }
}
