package app.organicmaps.settings;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
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
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.view.ViewCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.BuildConfig;
import app.organicmaps.R;
import app.organicmaps.sdk.sync.BackendType;
import app.organicmaps.sdk.sync.SyncAccount;
import app.organicmaps.sdk.sync.SyncManager;
import app.organicmaps.sdk.sync.SyncOpException;
import app.organicmaps.sdk.sync.preferences.SyncCallback;
import app.organicmaps.sdk.sync.preferences.SyncPrefs;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.sync.BackendUtils;
import app.organicmaps.sync.nextcloud.NextcloudLoginFlow;
import app.organicmaps.sync.nextcloud.PollParams;
import app.organicmaps.util.WindowInsetUtils;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.slider.Slider;
import com.google.common.primitives.Longs;
import java.util.ArrayList;
import java.util.List;

public class SyncSettingsFragment
    extends BaseSettingsFragment implements SyncAccountAdapter.OnAccountInteractionListener
{
  private static final String TAG = SyncSettingsFragment.class.getSimpleName();

  private static final long[] SYNC_INTERVALS_MS = {
      60 * 1000L, // 1 minute
      15 * 60 * 1000L, // 15 minutes
      3600 * 1000L, // 1 hour
      3 * 3600 * 1000L, // 3 hours
      12 * 3600 * 1000L, // 12 hours
      24 * 3600 * 1000L, // 1 day
      3 * 24 * 3600 * 1000L, // 3 days
      7 * 24 * 3600 * 1000L, // 1 week
      14 * 24 * 3600 * 1000L, // 2 weeks
  }; // For debug builds, an additional '5 seconds' interval is used.

  private SyncAccountAdapter mAdapter;
  private TextView mSyncIntervalTv;
  private View mAddAccountHint;
  private View mSetIntervalOption;

  private final SyncCallback.LastSync mLastSyncCallback =
      (accountId,
       timestamp) -> new Handler(Looper.getMainLooper()).post(() -> mAdapter.updateLastSynced(accountId, timestamp));

  private final SyncCallback.ErrorInfo mErrorInfoCallback =
      (accountId,
       exception) -> new Handler(Looper.getMainLooper()).post(() -> mAdapter.updateErrorInfo(accountId, exception));

  private final SyncCallback.AccountsChanged mAccountsChangedCallback =
      newAccounts -> new Handler(Looper.getMainLooper()).post(() -> refreshState(newAccounts));

  private final SyncCallback.FrequencyChange mFrequencyChangeCallback =
      syncIntervalMs -> mSyncIntervalTv.setText(formatDuration(syncIntervalMs, mSyncIntervalTv.getContext()));

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    ViewCompat.setOnApplyWindowInsetsListener(view, WindowInsetUtils.PaddingInsetsListener.excludeTop());
    ViewCompat.requestApplyInsets(view);

    Context context = view.getContext();
    mSyncIntervalTv = view.findViewById(R.id.tv_summary_interval);
    mAddAccountHint = view.findViewById(R.id.tv_add_sync_account_hint);
    mSetIntervalOption = view.findViewById(R.id.option_set_interval);

    view.findViewById(R.id.btn_add_sync_account).setOnClickListener(this::onAddAccountClick);
    mAddAccountHint.setOnClickListener(this::onAddAccountClick);
    mSetIntervalOption.setOnClickListener(this::showIntervalDialog);
    mSyncIntervalTv.setText(formatDuration(SyncManager.INSTANCE.getPrefs().getSyncIntervalMs(), context));

    RecyclerView mRecyclerView = view.findViewById(R.id.rv_sync_accounts);
    mRecyclerView.setLayoutManager(new LinearLayoutManager(context));
    mAdapter = new SyncAccountAdapter(new ArrayList<>(), this);
    mRecyclerView.setAdapter(mAdapter);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    SyncPrefs prefs = SyncManager.INSTANCE.getPrefs();
    refreshState(prefs.getAccounts());
    prefs.registerAccountsChangedCallback(mAccountsChangedCallback);
    prefs.registerLastSyncedCallback(mLastSyncCallback);
    prefs.registerFrequencyChangeCallback(mFrequencyChangeCallback);
    prefs.registerErrorInfoCallback(mErrorInfoCallback);
    pollNextcloudAuth();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    SyncPrefs prefs = SyncManager.INSTANCE.getPrefs();
    prefs.unregisterErrorInfoCallback(mErrorInfoCallback);
    prefs.unregisterFrequencyChangeCallback(mFrequencyChangeCallback);
    prefs.unregisterLastSyncedCallback(mLastSyncCallback);
    prefs.unregisterAccountsChangedCallback(mAccountsChangedCallback);
  }

  private void refreshState(List<SyncAccount> accounts)
  {
    mAdapter.updateAccounts(accounts);
    mAddAccountHint.setVisibility(accounts.isEmpty() ? View.VISIBLE : View.GONE);
    mSetIntervalOption.setVisibility(accounts.isEmpty() ? View.GONE : View.VISIBLE);
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
        holder.label.setText(BackendUtils.getDisplayName(getItem(i)));
        holder.icon.setImageDrawable(AppCompatResources.getDrawable(context, BackendUtils.getIcon(getItem(i))));
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
      BackendUtils.login(context, backendTypes[position]);
    });
    dialog.show();
  }

  @Override
  public void onSwitchToggled(SyncAccount account, boolean isChecked)
  {
    SyncPrefs prefs = SyncManager.INSTANCE.getPrefs();
    prefs.setEnabled(account, isChecked);
    mAdapter.updateEnabled(account.getAccountId(), isChecked);
  }

  @Override
  public void onAccountClicked(Context context, SyncAccount account)
  {
    // Show login UI if auth expired for clicked account card.
    SyncOpException accountError = SyncManager.INSTANCE.getPrefs().getErrorInfo(account.getAccountId());
    if (accountError instanceof SyncOpException.AuthExpiredException)
      BackendUtils.login(context, account.getBackendType());
  }

  @Override
  public boolean onAccountLongClicked(SyncAccount account)
  {
    new MaterialAlertDialogBuilder(requireContext(), R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.logout)
        .setMessage(getString(R.string.sync_log_out_confirmation,
                              requireContext().getString(BackendUtils.getDisplayName(account.getBackendType()))))
        .setPositiveButton(R.string.logout, (d, i) -> SyncManager.INSTANCE.getPrefs().removeAccount(account))
        .setNegativeButton(R.string.cancel, null)
        .show();
    return true;
  }

  private void showIntervalDialog(View view)
  {
    Context context = view.getContext();
    @SuppressLint("InflateParams")
    View sliderView = LayoutInflater.from(context).inflate(R.layout.dialog_slider, null);

    final long[] intervals =
        BuildConfig.DEBUG ? Longs.concat(new long[] {5_000}, SYNC_INTERVALS_MS) : SYNC_INTERVALS_MS;
    final SyncPrefs prefs = SyncManager.INSTANCE.getPrefs();
    final Slider slider = sliderView.findViewById(R.id.slider);
    slider.setValueFrom(0f);
    slider.setValueTo(intervals.length - 1);
    slider.setStepSize(1f);
    slider.setLabelFormatter(value -> formatDuration(intervals[Math.round(slider.getValue())], context));
    long currentInterval = prefs.getSyncIntervalMs();
    int currentIndex = intervals.length - 1;
    while (currentIndex > 0 && intervals[currentIndex] > currentInterval)
      --currentIndex;
    slider.setValue(currentIndex);

    new MaterialAlertDialogBuilder(context, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.set_sync_interval)
        .setView(sliderView)
        .setCancelable(false)
        .setNegativeButton(R.string.cancel, null)
        .setPositiveButton(R.string.ok,
                           (dialogInterface, i) -> {
                             long chosenInterval = intervals[Math.round(slider.getValue())];
                             prefs.setSyncFrequency(chosenInterval);
                           })
        .create()
        .show();
  }

  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_prefs_sync;
  }

  private void pollNextcloudAuth()
  {
    final Context context = getContext();
    SyncPrefs syncPrefs = SyncManager.INSTANCE.getPrefs();
    String pollParamsStr = syncPrefs.getNextcloudPollParams();
    if (pollParamsStr == null)
      return;
    try
    {
      PollParams pollParams = PollParams.fromString(pollParamsStr);
      ThreadPool.getWorker().execute(() -> {
        if (!NextcloudLoginFlow.storeAuthStateIfAvailable(context, pollParams))
          syncPrefs.setNextcloudPollParams(null);
      });
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Error trying to parse stored Nextcloud auth poll parameters", e);
    }
  }

  private static String formatDuration(long millis, Context context)
  {
    Resources res = context.getResources();
    if (millis < 60_000)
    {
      int seconds = (int) (millis / 1000);
      return res.getQuantityString(R.plurals.duration_seconds, seconds, seconds);
    }
    else if (millis < 3_600_000)
    {
      int minutes = (int) (millis / 60_000);
      return res.getQuantityString(R.plurals.duration_minutes, minutes, minutes);
    }
    else if (millis < 86_400_000)
    {
      int hours = (int) (millis / 3_600_000);
      return res.getQuantityString(R.plurals.duration_hours, hours, hours);
    }
    else if (millis < 604_800_000)
    {
      int days = (int) (millis / 86_400_000);
      return res.getQuantityString(R.plurals.duration_days, days, days);
    }
    else
    {
      int weeks = (int) (millis / 604_800_000);
      return res.getQuantityString(R.plurals.duration_weeks, weeks, weeks);
    }
  }
}
