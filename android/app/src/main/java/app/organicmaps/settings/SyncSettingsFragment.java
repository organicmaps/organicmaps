package app.organicmaps.settings;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import androidx.appcompat.app.AlertDialog;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sync.BackendType;
import app.organicmaps.sync.SyncAccount;
import app.organicmaps.sync.SyncPrefs;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.util.ArrayList;
import java.util.List;

public class SyncSettingsFragment
    extends BaseSettingsFragment implements SyncAccountAdapter.OnAccountInteractionListener
{
  private SyncAccountAdapter mAdapter;
  private View mAddAccountHint;

  private final SyncPrefs.LastSyncCallback mLastSyncCallback = new SyncPrefs.LastSyncCallback() {
    @Override
    public void onLastSyncChanged(long accountId, long timestamp)
    {
      new Handler(Looper.getMainLooper()).post(() -> mAdapter.updateSyncStatusText(accountId, true, timestamp));
    }
  };

  private final SyncPrefs.AccountsChangedCallback mAccountsChangedCallback = new SyncPrefs.AccountsChangedCallback() {
    @Override
    public void onAccountsChanged(List<SyncAccount> newAccounts)
    {
      new Handler(Looper.getMainLooper()).post(() -> refreshState(newAccounts));
    }
  };

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    view.findViewById(R.id.btn_add_sync_account).setOnClickListener(this::onAddAccountClick);
    mAddAccountHint = view.findViewById(R.id.tv_add_sync_account_hint);

    RecyclerView mRecyclerView = view.findViewById(R.id.rv_sync_accounts);
    mRecyclerView.setLayoutManager(new LinearLayoutManager(view.getContext()));
    mAdapter = new SyncAccountAdapter(new ArrayList<>(), this);
    mRecyclerView.setAdapter(mAdapter);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    SyncPrefs prefs = SyncPrefs.getInstance(getContext());
    refreshState(prefs.getAccounts());
    prefs.registerAccountsChangedCallback(mAccountsChangedCallback);
    prefs.registerLastSyncedCallback(mLastSyncCallback);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    SyncPrefs prefs = SyncPrefs.getInstance(getContext());
    prefs.unregisterAccountsChangedCallback(mAccountsChangedCallback);
    prefs.unregisterLastSyncedCallback(mLastSyncCallback);
  }

  private void refreshState(List<SyncAccount> accounts)
  {
    mAdapter.updateAccounts(accounts);
    mAddAccountHint.setVisibility(accounts.isEmpty() ? View.VISIBLE : View.GONE);
  }

  private void onAddAccountClick(View view)
  {
    Context context = view.getContext();

    MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(context, R.style.MwmTheme_AlertDialog);
    ListView listView = new ListView(context);
    BackendType[] backendTypes = BackendType.values();
    listView.setAdapter(new BaseAdapter() {
      private final LayoutInflater inflater = LayoutInflater.from(context);

      @Override
      public int getCount()
      {
        return backendTypes.length;
      }

      @Override
      public BackendType getItem(int i)
      {
        return backendTypes[i];
      }

      @Override
      public long getItemId(int i)
      {
        return i;
      }

      @Override
      public View getView(int i, View convertView, ViewGroup parent)
      {
        View view;
        ViewHolder holder;
        if (convertView == null)
        {
          view = inflater.inflate(R.layout.item_labelled_icon, parent, false);
          holder = new ViewHolder();
          holder.icon = view.findViewById(R.id.iv_icon);
          holder.label = view.findViewById(R.id.tv_label);
          view.setTag(holder);
        }
        else
        {
          view = convertView;
          holder = (ViewHolder) view.getTag();
        }
        holder.label.setText(getItem(i).getDisplayName(context));
        holder.icon.setImageDrawable(getItem(i).getIcon(context));
        return view;
      }

      private static class ViewHolder
      {
        ImageView icon;
        TextView label;
      }
    });
    AlertDialog dialog = builder.setView(listView).create();
    listView.setOnItemClickListener((parent, clickedView, position, id) -> {
      dialog.dismiss();
      backendTypes[position].login(context);
    });
    dialog.show();
  }

  @Override
  public void onSwitchToggled(SyncAccount account, boolean isChecked)
  {
    SyncPrefs prefs = SyncPrefs.getInstance(getContext());
    prefs.setEnabled(account, isChecked);
    mAdapter.updateSyncStatusText(account.getAccountId(), isChecked, prefs.getLastSynced(account.getAccountId()));
  }

  @Override
  public void onAccountClicked(long accountId)
  {
    // TODO
  }

  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_prefs_sync;
  }
}
