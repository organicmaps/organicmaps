package com.mapswithme.maps.downloader;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.view.View;
import android.view.Window;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.news.BaseNewsFragment;
import com.mapswithme.util.Constants;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

import static com.mapswithme.util.statistics.Statistics.EventName.DOWNLOADER_DIALOG_CANCEL;
import static com.mapswithme.util.statistics.Statistics.EventName.DOWNLOADER_DIALOG_DOWNLOAD;
import static com.mapswithme.util.statistics.Statistics.EventName.DOWNLOADER_DIALOG_LATER;
import static com.mapswithme.util.statistics.Statistics.EventName.DOWNLOADER_DIALOG_MANUAL_DOWNLOAD;
import static com.mapswithme.util.statistics.Statistics.EventName.DOWNLOADER_DIALOG_SHOW;

public class UpdaterDialogFragment extends BaseMwmDialogFragment
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.DOWNLOADER);
  private static final String TAG = UpdaterDialogFragment.class.getSimpleName();

  private static final String EXTRA_LEFTOVER_MAPS = "extra_leftover_maps";

  private static final String ARG_UPDATE_IMMEDIATELY = "arg_update_immediately";
  private static final String ARG_TOTAL_SIZE = "arg_total_size";
  private static final String ARG_TOTAL_SIZE_MB = "arg_total_size_mb";
  private static final String ARG_OUTDATED_MAPS = "arg_outdated_maps";

  private TextView mTitle;
  private TextView mUpdateBtn;
  private ProgressBar mProgressBar;
  private TextView mCancelBtn;

  private int mListenerSlot = 0;
  @Nullable
  private String mTotalSize;
  private long mTotalSizeMb;
  private boolean mAutoUpdate;
  @Nullable
  private String[] mOutdatedMaps;
  /**
   * Stores maps which are left to finish autoupdating process.
   */
  @Nullable
  private HashSet<String> mLeftoverMaps;

  @Nullable
  private BaseNewsFragment.NewsDialogListener mDoneListener;

  @Nullable
  private DetachableStorageCallback mStorageCallback;

  private void finish()
  {
    dismiss();
    if (mDoneListener != null)
      mDoneListener.onDialogDone();
  }

  @NonNull
  private final View.OnClickListener mCancelClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      Statistics.INSTANCE.trackDownloaderDialogEvent(MapManager.nativeIsDownloading()
                                                     ? DOWNLOADER_DIALOG_LATER
                                                     : DOWNLOADER_DIALOG_CANCEL,
                                                     mTotalSizeMb);

      if (MapManager.nativeIsDownloading())
        MapManager.nativeCancel(CountryItem.getRootId());

      finish();
    }
  };

  @NonNull
  private final View.OnClickListener mUpdateClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      MapManager.warnOn3gUpdate(getActivity(), CountryItem.getRootId(), new Runnable()
      {
        @Override
        public void run()
        {
          MapManager.nativeUpdate(CountryItem.getRootId());
          UiUtils.show(mProgressBar);
          UiUtils.hide(mUpdateBtn);
          mTitle.setText(String.format(Locale.getDefault(), "%s %d%%",
                                       getString(R.string.whats_new_auto_update_updating_maps), 0));
          mCancelBtn.setText(R.string.cancel);

          Statistics.INSTANCE.trackDownloaderDialogEvent(DOWNLOADER_DIALOG_MANUAL_DOWNLOAD,
                                                         mTotalSizeMb);
        }
      });
    }
  };

  public static boolean showOn(@NonNull FragmentActivity activity,
                               @Nullable BaseNewsFragment.NewsDialogListener doneListener)
  {
    final FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    final UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    if (info == null)
      return false;

    @Framework.DoAfterUpdate
    final int result;

    final long size = info.totalSize / Constants.MB;
    Fragment f = fm.findFragmentByTag(UpdaterDialogFragment.class.getName());
    if (f != null)
    {
      ((UpdaterDialogFragment)f).mDoneListener = doneListener;
      return true;
    }
    else
    {
      result = Framework.nativeToDoAfterUpdate();
      if (result == Framework.DO_AFTER_UPDATE_MIGRATE || result == Framework.DO_AFTER_UPDATE_NOTHING)
        return false;

      Statistics.INSTANCE.trackDownloaderDialogEvent(DOWNLOADER_DIALOG_SHOW, size);
    }

    final Bundle args = new Bundle();
    args.putBoolean(ARG_UPDATE_IMMEDIATELY, result == Framework.DO_AFTER_UPDATE_AUTO_UPDATE);
    args.putString(ARG_TOTAL_SIZE, StringUtils.getFileSizeString(info.totalSize));
    args.putLong(ARG_TOTAL_SIZE_MB, size);
    args.putStringArray(ARG_OUTDATED_MAPS, Framework.nativeGetOutdatedCountries());

    final UpdaterDialogFragment fragment = new UpdaterDialogFragment();
    fragment.setArguments(args);
    fragment.mDoneListener = doneListener;
    FragmentTransaction transaction = fm.beginTransaction()
      .setCustomAnimations(android.R.anim.fade_in, android.R.anim.fade_out);
    fragment.show(transaction, UpdaterDialogFragment.class.getName());

    return true;
  }

  @Override
  protected int getCustomTheme()
  {
    return super.getFullscreenTheme();
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    if (savedInstanceState != null)
    {  // As long as we use HashSet to store leftover maps this cast is safe.
      //noinspection unchecked
      mLeftoverMaps = (HashSet<String>) savedInstanceState.getSerializable(EXTRA_LEFTOVER_MAPS);
    }
    readArguments();
    mStorageCallback = new DetachableStorageCallback(this, mLeftoverMaps, mOutdatedMaps);
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    outState.putSerializable(EXTRA_LEFTOVER_MAPS, mLeftoverMaps);
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Dialog res = super.onCreateDialog(savedInstanceState);
    res.requestWindowFeature(Window.FEATURE_NO_TITLE);

    View content = View.inflate(getActivity(), R.layout.fragment_updater, null);
    res.setContentView(content);

    mTitle = (TextView) content.findViewById(R.id.title);
    mUpdateBtn  = (TextView) content.findViewById(R.id.update_btn);
    mProgressBar = (ProgressBar) content.findViewById(R.id.progress);
    mCancelBtn = (TextView) content.findViewById(R.id.cancel_btn);

    initViews();

    return res;
  }

  @Override
  public void onResume()
  {
    super.onResume();

    if (isAllUpdated())
    {
      finish();
      return;
    }

    // The storage callback must be non-null at this point.
    //noinspection ConstantConditions
    mStorageCallback.attach(this);
    mListenerSlot = MapManager.nativeSubscribe(mStorageCallback);

    if (mAutoUpdate && !MapManager.nativeIsDownloading())
    {
      MapManager.warnOn3gUpdate(getActivity(), CountryItem.getRootId(), new Runnable()
      {
        @Override
        public void run()
        {
          MapManager.nativeUpdate(CountryItem.getRootId());
          Statistics.INSTANCE.trackDownloaderDialogEvent(DOWNLOADER_DIALOG_DOWNLOAD,
                                                         mTotalSizeMb);
        }
      });
    }
  }

  @Override
  public void onPause()
  {
    if (mStorageCallback != null)
      mStorageCallback.detach();

    super.onPause();
  }

  @Override
  public void onDestroy()
  {
    if (mListenerSlot != 0)
    {
      MapManager.nativeUnsubscribe(mListenerSlot);
      mListenerSlot = 0;
    }
    super.onDestroy();
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    if (MapManager.nativeIsDownloading())
      MapManager.nativeCancel(CountryItem.getRootId());

    if (mDoneListener != null)
      mDoneListener.onDialogDone();

    super.onCancel(dialog);
  }

  private void readArguments()
  {
    Bundle args = getArguments();
    if (args == null)
      return;

    mAutoUpdate = args.getBoolean(ARG_UPDATE_IMMEDIATELY);
    if (!mAutoUpdate && MapManager.nativeIsDownloading())
      mAutoUpdate = true;

    mTotalSize = args.getString(ARG_TOTAL_SIZE);
    mTotalSizeMb = args.getLong(ARG_TOTAL_SIZE_MB, 0L);
    mOutdatedMaps = args.getStringArray(ARG_OUTDATED_MAPS);
    if (mLeftoverMaps == null && mOutdatedMaps != null && mOutdatedMaps.length > 0)
    {
      mLeftoverMaps = new HashSet<>(Arrays.asList(mOutdatedMaps));
    }
  }

  private void initViews()
  {
    UiUtils.showIf(mAutoUpdate, mProgressBar);
    UiUtils.showIf(!mAutoUpdate, mUpdateBtn);

    mUpdateBtn.setText(getString(R.string.whats_new_auto_update_button_size, mTotalSize));
    mUpdateBtn.setOnClickListener(mUpdateClickListener);
    mCancelBtn.setText(mAutoUpdate ? R.string.cancel : R.string.whats_new_auto_update_button_later);
    mCancelBtn.setOnClickListener(mCancelClickListener);
    mTitle.setText(mAutoUpdate ? String.format(Locale.getDefault(), "%s %d%%",
                                               getString(R.string.whats_new_auto_update_updating_maps),
                                               MapManager.nativeGetOverallProgress(mOutdatedMaps))
                               : getString(R.string.whats_new_auto_update_title));
  }

  private boolean isAllUpdated()
  {
    return mOutdatedMaps == null || mLeftoverMaps == null || mLeftoverMaps.isEmpty();
  }

  private static class DetachableStorageCallback implements MapManager.StorageCallback
  {
    @Nullable
    private UpdaterDialogFragment mFragment;
    @Nullable
    private final Set<String> mLeftoverMaps;
    @Nullable
    private final String[] mOutdatedMaps;

    DetachableStorageCallback(@Nullable UpdaterDialogFragment fragment,
                              @Nullable Set<String> leftoverMaps,
                              @Nullable String[] outdatedMaps)
    {
      mFragment = fragment;
      mLeftoverMaps = leftoverMaps;
      mOutdatedMaps = outdatedMaps;
    }

    @Override
    public void onStatusChanged(List<MapManager.StorageCallbackData> data)
    {
      for (MapManager.StorageCallbackData item : data)
      {
        if (item.isLeafNode && item.newStatus == CountryItem.STATUS_FAILED)
        {
          showErrorDialog(item);
          return;
        }
        else if (item.isLeafNode && item.newStatus == CountryItem.STATUS_DONE)
        {
          LOGGER.i(TAG, "Update finished for: " + item.countryId);
          if (mLeftoverMaps != null)
            mLeftoverMaps.remove(item.countryId);
        }
      }

      if (mFragment != null && mFragment.isAdded() && mFragment.isAllUpdated())
        mFragment.finish();
    }

    private void showErrorDialog(MapManager.StorageCallbackData item)
    {
      if (mFragment == null || !mFragment.isAdded())
        return;

      String text;
      switch (item.errorCode)
      {
        case CountryItem.ERROR_NO_INTERNET:
          text = mFragment.getString(R.string.common_check_internet_connection_dialog);
          break;

        case CountryItem.ERROR_OOM:
          text = mFragment.getString(R.string.downloader_no_space_title);
          break;

        default:
          text = String.valueOf(item.errorCode);
      }
      Statistics.INSTANCE.trackDownloaderDialogError(mFragment.mTotalSizeMb, text);
      MapManager.showErrorDialog(mFragment.getActivity(), item, new Utils.Proc<Boolean>()
      {
        @Override
        public void invoke(@NonNull Boolean result)
        {
          if (result)
          {
            MapManager.warnOn3gUpdate(mFragment.getActivity(), CountryItem.getRootId(), new Runnable()
            {
              @Override
              public void run()
              {
                MapManager.nativeUpdate(CountryItem.getRootId());
              }
            });
          }
          else
          {
            MapManager.nativeCancel(CountryItem.getRootId());
            mFragment.finish();
          }
        }
      });
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      if (mOutdatedMaps == null || mFragment == null || !mFragment.isAdded())
        return;

      int progress = MapManager.nativeGetOverallProgress(mOutdatedMaps);
      mFragment.mTitle.setText(String.format(Locale.getDefault(), "%s %d%%",
                                             mFragment.getString(R.string.whats_new_auto_update_updating_maps),
                                             progress));
    }

    void attach(@NonNull UpdaterDialogFragment fragment)
    {
      mFragment = fragment;
    }

    void detach()
    {
      mFragment = null;
    }
  }
}
