package com.mapswithme.maps.downloader;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.view.View;
import android.view.Window;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.Constants;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;
import java.util.Locale;

import static com.mapswithme.util.statistics.Statistics.EventName.DOWNLOADER_DIALOG_CANCEL;
import static com.mapswithme.util.statistics.Statistics.EventName.DOWNLOADER_DIALOG_DOWNLOAD;
import static com.mapswithme.util.statistics.Statistics.EventName.DOWNLOADER_DIALOG_LATER;
import static com.mapswithme.util.statistics.Statistics.EventName.DOWNLOADER_DIALOG_MANUAL_DOWNLOAD;
import static com.mapswithme.util.statistics.Statistics.EventName.DOWNLOADER_DIALOG_SHOW;

public class UpdaterDialogFragment extends BaseMwmDialogFragment
{

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

  @NonNull
  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback()
  {

    @Override
    public void onStatusChanged(List<MapManager.StorageCallbackData> data)
    {
      if (mOutdatedMaps == null)
        return;

      for (MapManager.StorageCallbackData item : data)
      {
        if (item.isLeafNode && item.newStatus == CountryItem.STATUS_FAILED)
        {
          String text;
          switch (item.errorCode)
          {
            case CountryItem.ERROR_NO_INTERNET:
              text = getString(R.string.common_check_internet_connection_dialog);
              break;

            case CountryItem.ERROR_OOM:
              text = getString(R.string.downloader_no_space_title);
              break;

            default:
              text = String.valueOf(item.errorCode);
          }
          Statistics.INSTANCE.trackDownloaderDialogError(mTotalSizeMb, text);
          MapManager.showError(getActivity(), item, new Utils.Proc<Boolean>()
          {
            @Override
            public void invoke(@NonNull Boolean param)
            {
              if (param)
                MapManager.nativeUpdate(CountryItem.getRootId());
              else
                dismiss();
            }
          });

          return;
        }
      }

      if (!isAllUpdated())
        return;

      dismiss();
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      if (mOutdatedMaps == null)
        return;

      int progress = MapManager.nativeGetOverallProgress(mOutdatedMaps);
      mTitle.setText(String.format(Locale.getDefault(), "%s %d%%",
                                   getString(R.string.updating_maps), progress));
    }
  };

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

      dismiss();
    }
  };

  @NonNull
  private final View.OnClickListener mUpdateClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      MapManager.nativeUpdate(CountryItem.getRootId());
      UiUtils.show(mProgressBar);
      UiUtils.hide(mUpdateBtn);
      mTitle.setText(String.format(Locale.getDefault(), "%s %d%%",
                                   getString(R.string.updating_maps), 0));
      mCancelBtn.setText(R.string.cancel);

      Statistics.INSTANCE.trackDownloaderDialogEvent(DOWNLOADER_DIALOG_MANUAL_DOWNLOAD,
                                                     mTotalSizeMb);
    }
  };

  public static boolean showOn(@NonNull FragmentActivity activity)
  {
    final FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    Fragment f = fm.findFragmentByTag(UpdaterDialogFragment.class.getName());
    if (f != null)
      return false;

    @Framework.DoAfterUpdate
    final int result = Framework.nativeToDoAfterUpdate();
    if (result == Framework.DO_AFTER_UPDATE_MIGRATE || result == Framework.DO_AFTER_UPDATE_NOTHING)
      return false;

    final UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    if (info == null)
      return false;

    final Bundle args = new Bundle();
    final long size = info.totalSize / Constants.MB;
    args.putBoolean(ARG_UPDATE_IMMEDIATELY, result == Framework.DO_AFTER_UPDATE_AUTO_UPDATE);
    args.putString(ARG_TOTAL_SIZE, StringUtils.getFileSizeString(info.totalSize));
    args.putLong(ARG_TOTAL_SIZE_MB, size);
    args.putStringArray(ARG_OUTDATED_MAPS, Framework.nativeGetOutdatedCountries());

    final UpdaterDialogFragment fragment = new UpdaterDialogFragment();
    fragment.setArguments(args);
    fragment.show(fm, UpdaterDialogFragment.class.getName());

    Statistics.INSTANCE.trackDownloaderDialogEvent(DOWNLOADER_DIALOG_SHOW, size);

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

    readArguments();
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
      dismiss();
      return;
    }

    mListenerSlot = MapManager.nativeSubscribe(mStorageCallback);

    if (mAutoUpdate && !MapManager.nativeIsDownloading())
    {
      MapManager.nativeUpdate(CountryItem.getRootId());
      Statistics.INSTANCE.trackDownloaderDialogEvent(DOWNLOADER_DIALOG_DOWNLOAD,
                                                     mTotalSizeMb);
    }
  }

  @Override
  public void onPause()
  {
    if (mListenerSlot != 0)
    {
      MapManager.nativeUnsubscribe(mListenerSlot);
      mListenerSlot = 0;
    }
    super.onPause();
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    if (MapManager.nativeIsDownloading())
      MapManager.nativeCancel(CountryItem.getRootId());

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
  }

  private void initViews()
  {
    UiUtils.showIf(mAutoUpdate, mProgressBar);
    UiUtils.showIf(!mAutoUpdate, mUpdateBtn);

    mUpdateBtn.setText(String.format(Locale.US, "%s (%s)", getString(R.string.downloader_update_all_button),
                                     mTotalSize));
    mUpdateBtn.setOnClickListener(mUpdateClickListener);
    mCancelBtn.setText(mAutoUpdate ? R.string.cancel : R.string.later);
    mCancelBtn.setOnClickListener(mCancelClickListener);
    mTitle.setText(mAutoUpdate ? String.format(Locale.getDefault(), "%s %d%%",
                                               getString(R.string.updating_maps),
                                               MapManager.nativeGetOverallProgress(mOutdatedMaps))
                               : getString(R.string.update_maps_ask));
  }

  private boolean isAllUpdated()
  {
    return MapManager.nativeGetOverallProgress(mOutdatedMaps) >= 100;
  }
}
