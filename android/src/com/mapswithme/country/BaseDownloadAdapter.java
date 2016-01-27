package com.mapswithme.country;

import android.animation.Animator;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Handler;
import android.support.annotation.ColorInt;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import java.lang.ref.WeakReference;

import com.mapswithme.maps.MapStorage;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.DownloadHelper;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

abstract class BaseDownloadAdapter extends BaseAdapter
{
  static final int TYPE_GROUP = 0;
  static final int TYPE_COUNTRY_GROUP = 1;
  static final int TYPE_COUNTRY_IN_PROCESS = 2;
  static final int TYPE_COUNTRY_READY = 3;
  static final int TYPE_COUNTRY_NOT_DOWNLOADED = 4;
  static final int TYPES_COUNT = TYPE_COUNTRY_NOT_DOWNLOADED + 1;

  private int mActiveAnimationsCount;
  private static final String PROPERTY_TRANSLATION_X = "translationX";
  private static final long ANIMATION_LENGTH = 250;
  private final Handler mHandler = new Handler();
  ListView mListView;

  private static class SafeAdapterRunnable implements Runnable
  {
    private final WeakReference<BaseDownloadAdapter> mAdapterReference;

    private SafeAdapterRunnable(BaseDownloadAdapter adapter)
    {
      mAdapterReference = new WeakReference<>(adapter);
    }

    @Override
    public void run()
    {
      final BaseDownloadAdapter adapter = mAdapterReference.get();
      if (adapter == null)
        return;

      if (adapter.mActiveAnimationsCount == 0)
        adapter.notifyDataSetChanged();
      else
        adapter.mHandler.postDelayed(this, ANIMATION_LENGTH);
    }
  }

  private final SafeAdapterRunnable mDatasetChangedRunnable = new SafeAdapterRunnable(this);

  final LayoutInflater mInflater;
  final DownloadFragment mFragment;

  private final String mStatusDownloaded;
  private final String mStatusFailed;
  private final String mStatusOutdated;
  private final String mStatusNotDownloaded;
  private final String mMapOnly;

  private final int mColorDownloaded;
  private final int mColorOutdated;
  private final int mColorNotDownloaded;
  private final int mColorFailed;

  BaseDownloadAdapter(DownloadFragment fragment)
  {
    mFragment = fragment;
    mInflater = mFragment.getLayoutInflater();

    mStatusDownloaded = mFragment.getString(R.string.downloader_downloaded).toUpperCase();
    mStatusFailed = mFragment.getString(R.string.downloader_status_failed).toUpperCase();
    mStatusOutdated = mFragment.getString(R.string.downloader_status_outdated).toUpperCase();
    mStatusNotDownloaded = mFragment.getString(R.string.download).toUpperCase();
    mMapOnly = mFragment.getString(R.string.downloader_map_only).toUpperCase();

    mColorDownloaded = ThemeUtils.getColor(mInflater.getContext(), R.attr.countryDownloaded);
    mColorOutdated = ThemeUtils.getColor(mInflater.getContext(), R.attr.countryOutdated);
    mColorNotDownloaded = ThemeUtils.getColor(mInflater.getContext(), R.attr.countryNotDownloaded);
    mColorFailed = ThemeUtils.getColor(mInflater.getContext(), R.attr.countryFailed);
  }

  public abstract void onItemClick(int position, View view);

  protected abstract void expandGroup(int position);

  protected abstract void updateCountry(int position, int newOptions);

  protected abstract void downloadCountry(int position, int options);

  protected abstract void deleteCountry(int position, int options);

  // returns remote sizes for 2 options [map, map + route].
  protected abstract long[] getRemoteItemSizes(int position);

  // returns local and remote size for downloading items(DO NOT call that method for NOT_DOWNLOADED items).
  protected abstract long[] getDownloadableItemSizes(int position);

  protected abstract void cancelDownload(int position);

  protected abstract void retryDownload(int position);

  protected abstract void setCountryListener();

  protected abstract void resetCountryListener();

  protected abstract void showCountry(final int position);

  /**
   * Process routine from parent Activity.
   *
   * @return true If "back" was processed.
   */
  public abstract boolean onBackPressed();

  @Override
  public abstract CountryItem getItem(int position);

  protected int adjustPosition(int position)
  {
    return position;
  }

  protected int unadjustPosition(int position)
  {
    return position;
  }

  private void processNotDownloaded(final String name, final int position, final int newOptions, final ViewHolder holder)
  {
    final long[] remoteSizes = getRemoteItemSizes(adjustPosition(position));
    final long size = newOptions > StorageOptions.MAP_OPTION_MAP_ONLY ? remoteSizes[0] : remoteSizes[1];
    DownloadHelper.downloadWithCellularCheck(mFragment.getActivity(), size, name, new DownloadHelper.OnDownloadListener()
    {
      @Override
      public void onDownload()
      {
        startItemDownloading(holder, position, newOptions);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_MAP_DOWNLOAD);
      }
    });
  }

  private void confirmDownloadCancellation(final ViewHolder holder, final int position, final String name)
  {
    final Dialog dlg = new AlertDialog.Builder(mFragment.getActivity())
        .setTitle(name)
        .setMessage(R.string.are_you_sure)
        .setPositiveButton(R.string.cancel_download, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            stopItemDownloading(holder, position);
            dlg.dismiss();
          }
        })
        .create();
    dlg.setCanceledOnTouchOutside(true);
    dlg.show();
  }

  private void processOutOfDate(final String name, final int position, final int newOptions)
  {
    final long[] remoteSizes = getRemoteItemSizes(position);
    final long size = newOptions > StorageOptions.MAP_OPTION_MAP_ONLY ? remoteSizes[0] : remoteSizes[1];
    DownloadHelper.downloadWithCellularCheck(mFragment.getActivity(), size, name, new DownloadHelper.OnDownloadListener()
    {
      @Override
      public void onDownload()
      {
        updateCountry(position, newOptions);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_MAP_UPDATE);
      }
    });
  }

  private void processOnDisk(final String name, final int position, final int newOptions)
  {
    new AlertDialog.Builder(mFragment.getActivity())
        .setTitle(name)
        .setMessage(R.string.are_you_sure)
        .setPositiveButton(R.string.delete, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            deleteCountry(position, newOptions);
            Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_MAP_DELETE);
            dlg.dismiss();
          }
        }).show();
  }

  private void processFailed(ViewHolder holder, int position)
  {
    holder.mProgressSlided.setProgressColor(ThemeUtils.getColor(mListView.getContext(), R.attr.colorAccent));
    holder.mProgressSlided.setCenterDrawable(null);
    retryDownload(position);
  }

  public void onResume(ListView listView)
  {
    mListView = listView;
    setCountryListener();

    notifyDataSetChanged();
  }

  public void onPause()
  {
    mListView = null;
    resetCountryListener();
  }

  @Override
  public int getItemViewType(int position)
  {
    final CountryItem item = getItem(position);
    if (item != null)
      return item.getType();
    return TYPE_GROUP;
  }

  @Override
  public int getViewTypeCount()
  {
    return TYPES_COUNT;
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  protected static class ViewHolder
  {
    TextView mName;
    WheelProgressView mProgress;
    TextView mPercent;
    TextView mSize;
    LinearLayout mInfo;
    TextView mPercentSlided;
    TextView mSizeSlided;
    LinearLayout mInfoSlided;
    WheelProgressView mProgressSlided;
    ImageView mImageRoutingStatus;

    private Animator animator;

    void initFromView(View v)
    {
      mName = (TextView) v.findViewById(R.id.title);
      mProgress = (WheelProgressView) v.findViewById(R.id.download_progress);
      mPercent = (TextView) v.findViewById(R.id.tv__percent);
      mSize = (TextView) v.findViewById(R.id.tv__size);
      mInfo = (LinearLayout) v.findViewById(R.id.ll__info);
      mPercentSlided = (TextView) v.findViewById(R.id.tv__percent_slided);
      mSizeSlided = (TextView) v.findViewById(R.id.tv__size_slided);
      mInfoSlided = (LinearLayout) v.findViewById(R.id.ll__info_slided);
      mProgressSlided = (WheelProgressView) v.findViewById(R.id.download_progress_slided);
      mImageRoutingStatus = (ImageView) v.findViewById(R.id.iv__routing_status_slided);
    }

    void showDownloadingViews(boolean show)
    {
      UiUtils.showIf(show, mInfoSlided, mProgressSlided);
      UiUtils.showIf(!show, mInfo, mImageRoutingStatus, mProgress);
    }

    void setSizeText(long first, long second)
    {
      String text;
      if (first <= 0)
        text = StringUtils.getFileSizeString(second);
      else
        text = StringUtils.getFileSizeString(first) + "/" + StringUtils.getFileSizeString(second);
      mSize.setText(text);
      mSizeSlided.setText(text);
    }

    void setPercentText(String text)
    {
      mPercent.setText(text);
      mPercentSlided.setText(text);
    }

    void setPercentColor(@ColorInt int color)
    {
      mPercent.setTextColor(color);
      mPercentSlided.setTextColor(color);
    }

    void setProgress(int percent)
    {
      mProgressSlided.setProgress(percent);
    }
  }

  @Override
  public View getView(final int position, View convertView, ViewGroup parent)
  {
    ViewHolder holder;
    final int type = getItemViewType(position);

    if (convertView == null)
    {
      holder = new ViewHolder();
      convertView = mInflater.inflate(getLayoutForType(type), parent, false);
      holder.initFromView(convertView);
      convertView.setTag(holder);
    }
    else
      holder = (ViewHolder) convertView.getTag();

    if (holder.animator != null)
    {
      holder.animator.end();
      holder.animator = null;
    }

    switch (type)
    {
    case TYPE_COUNTRY_IN_PROCESS:
    case TYPE_COUNTRY_READY:
    case TYPE_COUNTRY_NOT_DOWNLOADED:
      bindCountry(position, holder);
      break;
    }

    setItemName(position, holder);
    final View fview = convertView;

    final View.OnClickListener listener = new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        onItemClick(position, fview);
      }
    };
    convertView.setOnClickListener(listener);
    if (holder.mImageRoutingStatus != null)
    {
      holder.mImageRoutingStatus.setOnClickListener(listener);
      holder.mInfoSlided.setOnClickListener(listener);
      holder.mInfo.setOnClickListener(listener);
    }

    return convertView;
  }

  private static int getLayoutForType(int type)
  {
    switch (type)
    {
    case TYPE_GROUP:
    case TYPE_COUNTRY_GROUP:
      return R.layout.download_item_group;
    case TYPE_COUNTRY_IN_PROCESS:
    case TYPE_COUNTRY_READY:
    case TYPE_COUNTRY_NOT_DOWNLOADED:
      return R.layout.download_item_country;
    }

    return 0;
  }

  private void bindCountry(final int position, final ViewHolder holder)
  {
    final CountryItem item = getItem(position);
    if (item == null)
      return;

    final int adjustedPosition = adjustPosition(position);
    holder.mInfoSlided.setTranslationX(0);
    holder.mInfo.setTranslationX(0);
    holder.mProgress.setTranslationX(0);
    holder.mProgressSlided.setTranslationX(0);

    long[] sizes;
    switch (item.getStatus())
    {
    case MapStorage.DOWNLOADING:
      UiUtils.hide(holder.mImageRoutingStatus, holder.mProgress, holder.mInfo, holder.mSize, holder.mProgress);
      UiUtils.show(holder.mInfoSlided, holder.mProgressSlided);
      holder.mProgressSlided.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          confirmDownloadCancellation(holder, position, item.getName());
        }
      });

      // FIXME when only routing is downloaded, second size in returned array is incorrect and contains total map+routing size.
      sizes = getDownloadableItemSizes(adjustedPosition);
      holder.setSizeText(0, sizes[1]);
      final int percent = (int) (sizes[0] * 100 / sizes[1]);
      holder.setPercentText(percent + "%");
      holder.setPercentColor(mFragment.getResources().getColor(R.color.downloader_gray));
      holder.setProgress(percent);
      break;

    case MapStorage.ON_DISK_OUT_OF_DATE:
      UiUtils.hide(holder.mProgress, holder.mProgressSlided, holder.mInfo);
      UiUtils.show(holder.mInfoSlided, holder.mImageRoutingStatus);
      holder.mInfoSlided.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          startItemUpdating(holder, adjustedPosition, item.getOptions());
          Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_MAP_UPDATE);
        }
      });

      bindCarRoutingIcon(holder, item);
      sizes = getDownloadableItemSizes(adjustedPosition);
      holder.setSizeText(0, sizes[1]);
      holder.setPercentText(mStatusOutdated);
      holder.setPercentColor(ThemeUtils.getColor(mListView.getContext(), R.attr.colorAccent));
      break;

    case MapStorage.ON_DISK:
      UiUtils.hide(holder.mProgress, holder.mProgressSlided, holder.mInfo);
      UiUtils.show(holder.mInfoSlided, holder.mImageRoutingStatus);

      bindCarRoutingIcon(holder, item);
      sizes = getRemoteItemSizes(adjustedPosition);
      if (item.getOptions() == StorageOptions.MAP_OPTION_MAP_ONLY)
        holder.setPercentText(mMapOnly);
      else
        holder.setPercentText(mStatusDownloaded);

      holder.setPercentColor(mFragment.getResources().getColor(R.color.downloader_gray));
      holder.setSizeText(0, sizes[0]);
      break;

    case MapStorage.DOWNLOAD_FAILED:
      UiUtils.show(holder.mInfoSlided, holder.mProgressSlided);
      UiUtils.hide(holder.mProgress, holder.mImageRoutingStatus, holder.mInfo, holder.mSize, holder.mPercent);

      final View.OnClickListener listener = new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          processFailed(holder, adjustedPosition);
        }
      };
      holder.mProgressSlided.setOnClickListener(listener);
      holder.mInfoSlided.setOnClickListener(listener);
      holder.mProgressSlided.setProgressColor(mColorFailed);
      holder.mProgressSlided.setCenterDrawable(ContextCompat.getDrawable(mFragment.getActivity(), R.drawable.ic_retry_failed));

      sizes = getDownloadableItemSizes(adjustedPosition);
      holder.setSizeText(0, sizes[1]);
      holder.setPercentText(mStatusFailed);
      holder.setPercentColor(mColorFailed);
      break;

    case MapStorage.IN_QUEUE:
      UiUtils.show(holder.mInfoSlided, holder.mProgressSlided);
      UiUtils.hide(holder.mProgress, holder.mImageRoutingStatus, holder.mInfo, holder.mSize, holder.mPercent);

      holder.mProgressSlided.setProgress(0);
      holder.mProgressSlided.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          confirmDownloadCancellation(holder, position, item.getName());
        }
      });
      holder.mInfoSlided.setVisibility(View.VISIBLE);

      sizes = getDownloadableItemSizes(adjustedPosition);
      holder.setSizeText(0, sizes[1]);
      holder.setPercentText(mFragment.getString(R.string.downloader_queued));
      holder.setPercentColor(mFragment.getResources().getColor(R.color.downloader_gray));
      break;

    case MapStorage.NOT_DOWNLOADED:
      UiUtils.show(holder.mInfo, holder.mSize, holder.mPercent);
      UiUtils.hide(holder.mProgress, holder.mImageRoutingStatus, holder.mProgressSlided);
      UiUtils.invisible(holder.mInfoSlided);

      sizes = getRemoteItemSizes(adjustedPosition);
      holder.setSizeText(0, sizes[1]);
      holder.setPercentText(mStatusNotDownloaded);
      holder.setPercentColor(ThemeUtils.getColor(mListView.getContext(), R.attr.colorAccent));
      break;
    }
  }

  private static void bindCarRoutingIcon(ViewHolder holder, CountryItem item)
  {
    boolean night = ThemeUtils.isNightTheme();
    int icon = (item.getOptions() == StorageOptions.MAP_OPTION_MAP_ONLY) ? night ? R.drawable.ic_routing_get_night
                                                                                 : R.drawable.ic_routing_get
                                                                         : night ? R.drawable.ic_routing_ok_night
                                                                                 : R.drawable.ic_routing_ok;
    holder.mImageRoutingStatus.setImageResource(icon);
  }

  private void startItemDownloading(final ViewHolder holder, final int position, int newOptions)
  {
    holder.setPercentText(mFragment.getString(R.string.downloader_queued));
    holder.setPercentColor(mFragment.getResources().getColor(R.color.downloader_gray));
    holder.mProgress.setVisibility(View.VISIBLE);

    ObjectAnimator animator = ObjectAnimator.ofFloat(holder.mProgress, PROPERTY_TRANSLATION_X, 0,
                                                     -mFragment.getResources().getDimensionPixelOffset(R.dimen.progress_wheel_width));
    animator.setDuration(ANIMATION_LENGTH);

    AnimatorSet animatorSet = new AnimatorSet();
    animatorSet.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        holder.showDownloadingViews(true);
        mActiveAnimationsCount--;
      }
    });

    if (getItem(position).getStatus() == MapStorage.NOT_DOWNLOADED)
    {
      ObjectAnimator infoAnimator = ObjectAnimator.ofFloat(holder.mInfo, PROPERTY_TRANSLATION_X, 0,
                                                           -mFragment.getResources().getDimensionPixelOffset(R.dimen.progress_wheel_width));
      infoAnimator.setDuration(ANIMATION_LENGTH);

      animatorSet.playTogether(animator, infoAnimator);
    }
    else
      animatorSet.play(animator);

    animatorSet.start();
    holder.animator = animatorSet;
    mActiveAnimationsCount++;

    downloadCountry(adjustPosition(position), newOptions);
  }

  private void startItemUpdating(final ViewHolder holder, int position, int options)
  {
    holder.setPercentText(mFragment.getString(R.string.downloader_queued));
    holder.setPercentColor(mFragment.getResources().getColor(R.color.downloader_gray));

    final ObjectAnimator animator = ObjectAnimator.ofFloat(holder.mProgress, PROPERTY_TRANSLATION_X, 0,
                                                           -mFragment.getResources().getDimensionPixelOffset(R.dimen.progress_wheel_width));
    animator.setDuration(ANIMATION_LENGTH);

    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        animator.removeListener(this);
        holder.showDownloadingViews(true);
        mActiveAnimationsCount--;
      }
    });

    animator.start();
    holder.animator = animator;
    mActiveAnimationsCount++;

    downloadCountry(position, options);
  }

  private void stopItemDownloading(final ViewHolder holder, final int position)
  {
    final CountryItem item = getItem(position);
    mActiveAnimationsCount++;
    if (item.getStatus() == MapStorage.NOT_DOWNLOADED)
    {
      ObjectAnimator animator = ObjectAnimator.ofFloat(holder.mInfoSlided, PROPERTY_TRANSLATION_X, 0,
                                                       mFragment.getResources().getDimensionPixelOffset(R.dimen.progress_wheel_width));
      animator.setDuration(ANIMATION_LENGTH);

      ObjectAnimator infoAnimator = ObjectAnimator.ofFloat(holder.mProgressSlided, PROPERTY_TRANSLATION_X, 0,
                                                           mFragment.getResources().getDimensionPixelOffset(R.dimen.progress_wheel_width));
      infoAnimator.setDuration(ANIMATION_LENGTH);

      final AnimatorSet animatorSet = new AnimatorSet();
      animatorSet.playTogether(animator, infoAnimator);
      animatorSet.addListener(new UiUtils.SimpleAnimatorListener()
      {
        @Override
        public void onAnimationEnd(Animator animation)
        {
          animatorSet.removeListener(this);
          holder.showDownloadingViews(false);
          mActiveAnimationsCount--;
        }
      });
      animatorSet.start();
      holder.animator = animatorSet;
    }
    else
    {
      if (item.getStatus() != MapStorage.DOWNLOADING)
      {
        holder.mImageRoutingStatus.setVisibility(View.VISIBLE);
        bindCarRoutingIcon(holder, item);
      }

      final ObjectAnimator infoAnimator = ObjectAnimator.ofFloat(holder.mProgressSlided, PROPERTY_TRANSLATION_X, 0,
                                                                 mFragment.getResources().getDimensionPixelOffset(R.dimen.progress_wheel_width));
      infoAnimator.setDuration(ANIMATION_LENGTH);

      infoAnimator.addListener(new UiUtils.SimpleAnimatorListener()
      {
        @Override
        public void onAnimationEnd(Animator animation)
        {
          infoAnimator.removeListener(this);
          holder.showDownloadingViews(false);
          mActiveAnimationsCount--;
        }
      });
      infoAnimator.start();
      // invalidation is needed for 'update all/cancel all' buttons
      holder.animator = infoAnimator;
    }

    mHandler.postDelayed(mDatasetChangedRunnable, ANIMATION_LENGTH);
    cancelDownload(adjustPosition(position)); // cancel download before checking real item status(now its just DOWNLOADING or IN_QUEUE)
  }

  private int getTextColor(int status)
  {
    switch (status)
    {
      case MapStorage.ON_DISK_OUT_OF_DATE:
        return mColorOutdated;

      case MapStorage.NOT_DOWNLOADED:
        return mColorNotDownloaded;

      case MapStorage.DOWNLOAD_FAILED:
        return mColorFailed;

      default:
        return mColorDownloaded;
    }
  }

  private void setItemName(int position, ViewHolder holder)
  {
    // set name and style
    final CountryItem item = getItem(position);
    if (item == null)
      return;

    holder.mName.setText(item.getName());
    holder.mName.setTypeface(item.getTypeface());
    holder.mName.setTextColor(getTextColor(item.getStatus()));
  }

  /**
   * Called from children to notify about item status changes.
   */
  protected void onCountryStatusChanged(int position)
  {
    final CountryItem item = getItem(position);
    if (item != null && mFragment.isAdded())
    {
      // use this hard reset, because of caching different ViewHolders according to item's type
      mHandler.postDelayed(mDatasetChangedRunnable, ANIMATION_LENGTH);

      if (item.getStatus() == MapStorage.DOWNLOAD_FAILED && mFragment.getListAdapter() == this)
        UiUtils.checkConnectionAndShowAlert(mFragment.getActivity(), String.format(mFragment.getString(R.string.download_country_failed), item.getName()));
    }
  }

  /**
   * Called from children to notify about item progress.
   */
  void onCountryProgress(int position, long current, long total)
  {
    if (mListView == null || !mFragment.isAdded())
      return;

    // Do update only one item's view; don't call notifyDataSetChanged
    int index = (position - mListView.getFirstVisiblePosition());

    // Skip first two items if at the root level and there are downloaded maps
    index = unadjustPosition(index);

    final View v = mListView.getChildAt(index);
    if (v != null)
    {
      final ViewHolder holder = (ViewHolder) v.getTag();
      if (holder != null && holder.mProgress != null)
      {
        final int percent = (int) (current * 100 / total);
        holder.setProgress(percent);
        holder.setPercentText(percent + "%");
      }
    }
  }

  protected boolean hasHeaderItems()
  {
    return false;
  }

  void showCountryContextMenu(final CountryItem countryItem, final View anchor, final int position)
  {
    if (countryItem == null || anchor.getParent() == null)
      return;

    final int MENU_CANCEL = 0;
    final int MENU_UPDATE = 1;
    final int MENU_DOWNLOAD = 2;
    final int MENU_EXPLORE = 3;
    final int MENU_DELETE = 4;
    final int MENU_RETRY = 5;

    final int status = countryItem.getStatus();
    final String name = countryItem.getName();
    final int adjustedPosition = adjustPosition(position);

    OnMenuItemClickListener menuItemClickListener = new OnMenuItemClickListener()
    {
      @Override
      public boolean onMenuItemClick(MenuItem item)
      {
        final int id = item.getItemId();

        ViewHolder holder = (ViewHolder) anchor.getTag();
        switch (id)
        {
        case MENU_DELETE:
          processOnDisk(name, adjustedPosition, StorageOptions.MAP_OPTION_MAP_ONLY);
          break;
        case MENU_CANCEL:
          confirmDownloadCancellation(holder, position, name);
          break;
        case MENU_EXPLORE:
          showCountry(adjustedPosition);
          break;
        case MENU_DOWNLOAD:
          processNotDownloaded(name, position, StorageOptions.MAP_OPTION_MAP_ONLY, holder);
          break;
        case MENU_UPDATE:
          processOutOfDate(name, adjustedPosition, StorageOptions.MAP_OPTION_MAP_ONLY);
          break;
        case MENU_RETRY:
          processFailed(holder, adjustedPosition);
          break;
        }

        return true;
      }
    };

    long[] remoteSizes = getRemoteItemSizes(adjustedPosition);

    BottomSheetHelper.Builder bs = BottomSheetHelper.create(mFragment.getActivity(), name)
                                                    .listener(menuItemClickListener);

    if (status == MapStorage.ON_DISK_OUT_OF_DATE)
    {
      BottomSheetHelper.sheet(bs, MENU_UPDATE, R.drawable.ic_update,
                              mFragment.getString(R.string.downloader_update_map) + ", " +
                              StringUtils.getFileSizeString(remoteSizes[0]));
    }

    if (status == MapStorage.ON_DISK || status == MapStorage.ON_DISK_OUT_OF_DATE)
    {
      bs.sheet(MENU_EXPLORE, R.drawable.ic_explore, R.string.zoom_to_country);

      bs.sheet(MENU_DELETE, R.drawable.ic_delete, R.string.downloader_delete_map);
    }

    switch (status)
    {
    case MapStorage.DOWNLOADING:
    case MapStorage.IN_QUEUE:
      bs.sheet(MENU_CANCEL, R.drawable.ic_cancel, R.string.cancel_download);
      break;

    case MapStorage.NOT_DOWNLOADED:
      BottomSheetHelper.sheet(bs, MENU_DOWNLOAD, R.drawable.ic_download_map,
                              mFragment.getString(R.string.downloader_download_map) + ", " +
                              StringUtils.getFileSizeString(remoteSizes[1]));
      break;

    case MapStorage.DOWNLOAD_FAILED:
      bs.sheet(MENU_RETRY, R.drawable.ic_retry, R.string.downloader_retry);
      break;
    }

    bs.tint().show();
  }
}
