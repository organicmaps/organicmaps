package com.mapswithme.country;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Handler;
import android.text.TextUtils;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;
import android.view.View.OnCreateContextMenuListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import com.mapswithme.maps.MapStorage;
import com.mapswithme.maps.R;
import com.mapswithme.maps.guides.GuideInfo;
import com.mapswithme.maps.guides.GuidesUtils;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.Constants;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;
import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.AnimatorSet;
import com.nineoldandroids.animation.ObjectAnimator;
import com.nineoldandroids.view.ViewHelper;

import java.lang.ref.WeakReference;

abstract class BaseDownloadAdapter extends BaseAdapter
{
  static final int TYPE_GROUP = 0;
  static final int TYPE_COUNTRY_GROUP = 1;
  static final int TYPE_COUNTRY_IN_PROCESS = 2;
  static final int TYPE_COUNTRY_READY = 3;
  static final int TYPE_COUNTRY_NOT_DOWNLOADED = 4;
  static final int TYPES_COUNT = 5;
  static final int INVALID_POSITION = -1;

  private int mActiveAnimationsCount;
  public static final String PROPERTY_TRANSLATION_X = "translationX";
  public static final String PROPERTY_ALPHA = "alpha";
  public static final String PROPERTY_X = "x";
  private static final long ANIMATION_LENGTH = 250;
  private Handler mHandler = new Handler();
  private ListView mListView;

  private static class SafeAdapterRunnable implements Runnable
  {
    private WeakReference<BaseDownloadAdapter> mAdapterReference;

    public SafeAdapterRunnable(BaseDownloadAdapter adapter)
    {
      mAdapterReference = new WeakReference<BaseDownloadAdapter>(adapter);
    }

    @Override
    public void run()
    {
      if (mAdapterReference == null)
        return;

      final BaseDownloadAdapter adapter = mAdapterReference.get();
      if (adapter == null)
        return;

      if (adapter.mActiveAnimationsCount == 0)
        adapter.fillList();
      else
        adapter.mHandler.postDelayed(this, ANIMATION_LENGTH);
    }
  }

  private SafeAdapterRunnable mDatasetChangedRunnable = new SafeAdapterRunnable(this);

  protected final LayoutInflater mInflater;
  protected final Activity mActivity;
  protected final boolean mHasGoogleStore;

  protected final String mStatusDownloaded;
  protected final String mStatusFailed;
  protected final String mStatusOutdated;
  protected final String mStatusNotDownloaded;


  public BaseDownloadAdapter(Activity activity)
  {
    mActivity = activity;
    mInflater = mActivity.getLayoutInflater();
    mHasGoogleStore = Utils.hasAnyGoogleStoreInstalled();

    mStatusDownloaded = mActivity.getString(R.string.downloader_downloaded).toUpperCase();
    mStatusFailed = mActivity.getString(R.string.downloader_status_failed).toUpperCase();
    mStatusOutdated = mActivity.getString(R.string.downloader_status_outdated).toUpperCase();
    mStatusNotDownloaded = mActivity.getString(R.string.download).toUpperCase();
  }

  protected abstract void fillList();

  public abstract void onItemClick(int position, View view);

  protected abstract void expandGroup(int position);

  protected abstract void updateCountry(int position, int newOptions);

  protected abstract void downloadCountry(int position, int options);

  protected abstract void deleteCountry(int position, int options);

  protected abstract long[] getItemSizes(int position, int options);

  protected abstract long[] getDownloadableItemSizes(int position);

  protected abstract GuideInfo getGuideInfo(int position);

  protected abstract void cancelDownload(int position);

  protected abstract void setCountryListener();

  protected abstract void resetCountryListener();

  protected void processNotDownloaded(final String name, int position, int newOptions, ViewHolder holder)
  {
    // TODO if download is greater then 50MB check 3g/WIFI connection
    startItemDownloading(holder, position, newOptions);
    Statistics.INSTANCE.trackCountryDownload();
  }

  protected void confirmDownloadCancelation(final ViewHolder holder, final int position, final String name)
  {
    final Dialog dlg = new AlertDialog.Builder(mActivity)
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

  protected void processOutOfDate(final String name, int position, int newOptions)
  {
    // TODO if download is greater then 50MB check 3g/WIFI connection
    updateCountry(position, newOptions);
    Statistics.INSTANCE.trackCountryUpdate();
  }

  protected void processOnDisk(final String name, final int position, final int newOptions)
  {
    new AlertDialog.Builder(mActivity)
        .setTitle(name)
        .setMessage(R.string.are_you_sure)
        .setPositiveButton(R.string.delete, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            deleteCountry(position, newOptions);
            Statistics.INSTANCE.trackCountryDeleted();
            dlg.dismiss();
          }
        })
        .create()
        .show();
  }

  /**
   * Process routine from parent Activity.
   *
   * @return true If "back" was processed.
   */
  public abstract boolean onBackPressed();

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
    public TextView mName;
    public WheelProgressView mProgress;
    public TextView mPercent;
    public TextView mSize;
    private LinearLayout mInfo;
    private TextView mPercentSlided;
    private TextView mSizeSlided;
    private LinearLayout mInfoSlided;
    private WheelProgressView mProgressSlided;

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
      bindCountry(position, type, holder);
      break;
    }

    setItemName(position, holder);
    convertView.setBackgroundResource(R.drawable.list_selector_holo_light);
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
    if (holder.mProgressSlided != null)
    {
      holder.mProgressSlided.setOnClickListener(listener);
      holder.mInfoSlided.setOnClickListener(listener);
      holder.mInfo.setOnClickListener(listener);
    }

    return convertView;
  }

  private int getLayoutForType(int type)
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

  protected void bindCountry(int position, int type, ViewHolder holder)
  {
    bindSizeAndProgress(holder, type, position);
  }

  protected void bindSizeAndProgress(final ViewHolder holder, final int type, final int position)
  {
    final CountryItem item = getItem(position);
    if (item == null)
      return;

    ViewHelper.setTranslationX(holder.mInfoSlided, 0);
    ViewHelper.setTranslationX(holder.mInfo, 0);
    ViewHelper.setTranslationX(holder.mProgress, 0);
    ViewHelper.setTranslationX(holder.mProgressSlided, 0);
    long[] sizes;
    switch (item.getStatus())
    {
    case MapStorage.DOWNLOADING:
      holder.mProgress.setVisibility(View.GONE);
      holder.mProgressSlided.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          confirmDownloadCancelation(holder, position, item.getName());
        }
      });
      holder.mInfo.setVisibility(View.INVISIBLE);
      holder.mInfoSlided.setVisibility(View.VISIBLE);
      holder.mProgressSlided.setVisibility(View.VISIBLE);

      sizes = getDownloadableItemSizes(position);
      setHolderSizeString(holder, 0, sizes[1]);
      setHolderPercentString(holder, mActivity.getString(R.string.downloader_queued), R.color.downloader_gray);
      break;

    case MapStorage.ON_DISK_OUT_OF_DATE:
      holder.mProgress.setVisibility(View.GONE);
      holder.mProgressSlided.setVisibility(View.GONE);
      holder.mInfo.setVisibility(View.VISIBLE);
      holder.mInfo.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          startItemDownloading(holder, position, getItem(position).getOptions());
        }
      });
      holder.mInfoSlided.setVisibility(View.GONE);

      sizes = getDownloadableItemSizes(position);
      setHolderSizeString(holder, 0, sizes[1]);
      setHolderPercentString(holder, mStatusOutdated, R.color.downloader_green);
      break;

    case MapStorage.ON_DISK:
      holder.mProgress.setVisibility(View.GONE);
      holder.mProgressSlided.setVisibility(View.GONE);
      holder.mInfo.setVisibility(View.VISIBLE);
      holder.mInfoSlided.setVisibility(View.GONE);

      sizes = getItemSizes(position, StorageOptions.MAP_OPTION_MAP_AND_CAR_ROUTING);
      setHolderSizeString(holder, 0, sizes[0]);
      setHolderPercentString(holder, mStatusDownloaded, R.color.downloader_gray);
      break;
    case MapStorage.DOWNLOAD_FAILED:
      holder.mProgress.setVisibility(View.GONE);
      holder.mProgressSlided.setVisibility(View.GONE);
      holder.mInfo.setVisibility(View.VISIBLE);
      holder.mInfo.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          // TODO retry
          downloadCountry(position, getItem(position).getOptions());
        }
      });
      holder.mInfoSlided.setVisibility(View.GONE);

      sizes = getDownloadableItemSizes(position);
      setHolderSizeString(holder, 0, sizes[1]);
      setHolderPercentString(holder, mStatusFailed, R.color.downloader_red);
      break;

    case MapStorage.IN_QUEUE:
      holder.mProgress.setVisibility(View.GONE);
      holder.mProgressSlided.setVisibility(View.VISIBLE);
      holder.mProgressSlided.setProgress(0);
      holder.mProgressSlided.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          confirmDownloadCancelation(holder, position, item.getName());
        }
      });
      holder.mInfo.setVisibility(View.INVISIBLE);
      holder.mInfoSlided.setVisibility(View.VISIBLE);

      sizes = getDownloadableItemSizes(position);
      setHolderSizeString(holder, 0, sizes[1]);
      setHolderPercentString(holder, mActivity.getString(R.string.downloader_queued), R.color.downloader_gray);
      break;

    case MapStorage.NOT_DOWNLOADED:
      holder.mProgress.setVisibility(View.GONE);
      holder.mProgressSlided.setVisibility(View.GONE);
      holder.mInfo.setVisibility(View.VISIBLE);
      holder.mInfoSlided.setVisibility(View.GONE);

      sizes = getItemSizes(position, StorageOptions.MAP_OPTION_MAP_ONLY);
      // FIXME next lines produces HUGE LAGS, fix it!!!
//      long[] sizesRemote = getItemSizes(position, StorageOptions.MAP_OPTION_CAR_ROUTING);
//      setHolderSizeString(holder, sizes[1], sizesRemote[1]);
      setHolderSizeString(holder, sizes[1], 42000000);
      setHolderPercentString(holder, mStatusNotDownloaded, R.color.downloader_green);
      break;

    }
  }

  private void startItemDownloading(final ViewHolder holder, final int position, int newOptions)
  {
    setHolderPercentString(holder, mActivity.getString(R.string.downloader_queued), R.color.downloader_gray);

    ObjectAnimator animator = ObjectAnimator.ofFloat(holder.mProgress, PROPERTY_TRANSLATION_X, 0,
        -mActivity.getResources().getDimensionPixelOffset(R.dimen.progress_wheel_width));
    animator.setDuration(ANIMATION_LENGTH);

    ObjectAnimator infoAnimator = ObjectAnimator.ofFloat(holder.mInfo, PROPERTY_TRANSLATION_X, 0,
        -mActivity.getResources().getDimensionPixelOffset(R.dimen.progress_wheel_width));
    infoAnimator.setDuration(ANIMATION_LENGTH);

    AnimatorSet animatorSet = new AnimatorSet();
    animatorSet.playTogether(animator, infoAnimator);
    animatorSet.addListener(new UiUtils.SimpleNineoldAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        holder.mInfoSlided.setVisibility(View.VISIBLE);
        holder.mInfo.setVisibility(View.INVISIBLE);
        holder.mProgressSlided.setVisibility(View.VISIBLE);
        holder.mProgress.setVisibility(View.GONE);
        mActiveAnimationsCount--;
      }
    });
    mActiveAnimationsCount++;
    animatorSet.start();
    holder.animator = animatorSet;

    holder.mProgress.setVisibility(View.VISIBLE);

    final CountryItem item = getItem(position);
    if (item == null)
      return;

    downloadCountry(position, newOptions);
  }

  private void stopItemDownloading(final ViewHolder holder, final int position)
  {
    ObjectAnimator animator = ObjectAnimator.ofFloat(holder.mInfoSlided, PROPERTY_TRANSLATION_X, 0,
        mActivity.getResources().getDimensionPixelOffset(R.dimen.progress_wheel_width));
    animator.setDuration(ANIMATION_LENGTH);

    ObjectAnimator infoAnimator = ObjectAnimator.ofFloat(holder.mProgressSlided, PROPERTY_TRANSLATION_X, 0,
        mActivity.getResources().getDimensionPixelOffset(R.dimen.progress_wheel_width));
    infoAnimator.setDuration(ANIMATION_LENGTH);

    AnimatorSet animatorSet = new AnimatorSet();
    animatorSet.playTogether(animator, infoAnimator);
    animatorSet.addListener(new UiUtils.SimpleNineoldAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        holder.mInfo.setVisibility(View.VISIBLE);
        holder.mInfoSlided.setVisibility(View.GONE);
        holder.mProgressSlided.setVisibility(View.GONE);
        holder.mProgress.setVisibility(View.GONE);
        mActiveAnimationsCount--;
      }
    });
    mActiveAnimationsCount++;
    animatorSet.start();
    holder.animator = animatorSet;

    final CountryItem item = getItem(position);
    if (item == null)
      return;

    cancelDownload(position);
  }

  @Override
  public abstract CountryItem getItem(int position);

  private void setHolderSizeString(ViewHolder holder, long first, long second)
  {
    String text;
    if (first <= 0)
      text = getSizeString(second);
    else
      text = getSizeString(first) + "/" + getSizeString(second);
    holder.mSize.setText(text);
    holder.mSizeSlided.setText(text);
  }

  private void setHolderPercentString(ViewHolder holder, String text, int color)
  {
    holder.mPercent.setText(text);
    holder.mPercent.setTextColor(mActivity.getResources().getColor(color));
    holder.mPercentSlided.setText(text);
    holder.mPercentSlided.setTextColor(mActivity.getResources().getColor(color));
  }

  protected void setItemName(int position, ViewHolder holder)
  {
    // set name and style
    final CountryItem item = getItem(position);
    if (item == null)
      return;

    holder.mName.setText(item.getName());
    holder.mName.setTypeface(item.getTypeface());
    holder.mName.setTextColor(item.getTextColor());
  }

  /**
   * Called from children to notify about item status changes.
   *
   * @param position
   */
  protected void onCountryStatusChanged(int position)
  {
    final CountryItem item = getItem(position);
    if (item != null)
    {
      // use this hard reset, because of caching different ViewHolders according to item's type
      mHandler.postDelayed(mDatasetChangedRunnable, ANIMATION_LENGTH);
    }
  }

  /**
   * Called from children to notify about item progress.
   *
   * @param position
   */
  protected void onCountryProgress(int position, long current, long total)
  {
    // do update only one item's view; don't call notifyDataSetChanged
    final View v = mListView.getChildAt(position - mListView.getFirstVisiblePosition());
    if (v != null)
    {
      final ViewHolder holder = (ViewHolder) v.getTag();
      if (holder != null && holder.mProgress != null)
      {
        final int percent = (int) (current * 100 / total);
        holder.mProgressSlided.setProgress(percent);
        holder.mPercent.setText(percent + "%");
        holder.mPercentSlided.setText(percent + "%");
      }
    }
  }

  protected abstract void showCountry(final int position);

  protected void showCountryContextMenu(final CountryItem countryItem, final View anchor, final int position)
  {
    if (countryItem == null)
      return;

    final int MENU_CANCEL = 0;
    final int MENU_DOWNLOAD = 1;
    final int MENU_UPDATE_MAP_AND_ROUTING = 2;
    final int MENU_DOWNLOAD_MAP_AND_ROUTING = 3;
    final int MENU_UPDATE_MAP_DOWNLOAD_ROUTING = 4;
    final int MENU_UPDATE = 5;
    final int MENU_DOWNLOAD_ROUTING = 6;
    final int MENU_SHOW = 7;
    final int MENU_GUIDE = 8;
    final int MENU_DELETE_ROUTING = 9;
    final int MENU_DELETE = 10;


    final int status = countryItem.getStatus();
    final String name = countryItem.getName();
    final int options = countryItem.getOptions();

    final OnMenuItemClickListener menuItemClickListener = new OnMenuItemClickListener()
    {
      @Override
      public boolean onMenuItemClick(MenuItem item)
      {
        final int id = item.getItemId();

        ViewHolder holder = (ViewHolder) anchor.getTag();
        switch (id)
        {
        case MENU_DELETE:
          processOnDisk(name, position, StorageOptions.MAP_OPTION_MAP_ONLY);
          break;
        case MENU_UPDATE:
          processOutOfDate(name, position, StorageOptions.MAP_OPTION_MAP_ONLY);
          break;
        case MENU_DOWNLOAD:
          processNotDownloaded(name, position, StorageOptions.MAP_OPTION_MAP_ONLY, holder);
          break;
        case MENU_CANCEL:
          confirmDownloadCancelation(holder, position, name);
          break;
        case MENU_SHOW:
          showCountry(position);
          break;
        case MENU_GUIDE:
          GuidesUtils.openOrDownloadGuide(getGuideInfo(position), mActivity);
          break;
        case MENU_DELETE_ROUTING:
          processOnDisk(name, position, StorageOptions.MAP_OPTION_CAR_ROUTING);
          break;
        case MENU_DOWNLOAD_ROUTING:
          processNotDownloaded(name, position, StorageOptions.MAP_OPTION_CAR_ROUTING, holder);
          break;
        case MENU_DOWNLOAD_MAP_AND_ROUTING:
          processNotDownloaded(name, position, StorageOptions.MAP_OPTION_MAP_AND_CAR_ROUTING, holder);
          break;
        case MENU_UPDATE_MAP_DOWNLOAD_ROUTING:
          // TODO
          break;
        case MENU_UPDATE_MAP_AND_ROUTING:
          processOutOfDate(name, position, StorageOptions.MAP_OPTION_MAP_AND_CAR_ROUTING);
          break;
        }

        return true;
      }
    };

    if (anchor.getParent() != null) // if view is out of list parent is null and context menu cannot bo shown
    {
      anchor.setOnCreateContextMenuListener(new OnCreateContextMenuListener()
      {
        @Override
        public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
        {
          menu.setHeaderTitle(countryItem.getName());

          if (status == MapStorage.ON_DISK || status == MapStorage.ON_DISK_OUT_OF_DATE)
          {
            String titleDelete = mActivity.getString(R.string.downloader_delete_map);
            menu.add(0, MENU_DELETE, MENU_DELETE, titleDelete).setOnMenuItemClickListener(menuItemClickListener);

            if (options != StorageOptions.MAP_OPTION_MAP_ONLY)
            {
              titleDelete = mActivity.getString(R.string.downloader_delete_routing);
              menu.add(0, MENU_DELETE_ROUTING, MENU_DELETE_ROUTING, titleDelete).setOnMenuItemClickListener(menuItemClickListener);
            }

            final String titleShow = mActivity.getString(R.string.zoom_to_country);
            menu.add(0, MENU_SHOW, MENU_SHOW, titleShow).setOnMenuItemClickListener(menuItemClickListener);
          }

          if (status == MapStorage.ON_DISK)
          {
            if (options == StorageOptions.MAP_OPTION_MAP_ONLY)
            {
              String titleShow = mActivity.getString(R.string.downloader_download_routing);
              // TODO add size
              menu.add(0, MENU_DOWNLOAD_ROUTING, MENU_DOWNLOAD_ROUTING, titleShow).setOnMenuItemClickListener(menuItemClickListener);
            }
          }

          if (status == MapStorage.ON_DISK_OUT_OF_DATE)
          {
            String titleUpdate;
            switch (options)
            {
            case StorageOptions.MAP_OPTION_MAP_ONLY:
              titleUpdate = mActivity.getString(R.string.downloader_update_map);
              // TODO size
              menu.add(0, MENU_UPDATE, MENU_UPDATE, titleUpdate)
                  .setOnMenuItemClickListener(menuItemClickListener);

              titleUpdate = mActivity.getString(R.string.downloader_download_routing);
              menu.add(0, MENU_UPDATE_MAP_DOWNLOAD_ROUTING, MENU_UPDATE_MAP_DOWNLOAD_ROUTING, titleUpdate)
                  .setOnMenuItemClickListener(menuItemClickListener);
              break;
            case StorageOptions.MAP_OPTION_MAP_AND_CAR_ROUTING:
              titleUpdate = mActivity.getString(R.string.downloader_update_map);
              // TODO size
              menu.add(0, MENU_UPDATE_MAP_AND_ROUTING, MENU_UPDATE_MAP_AND_ROUTING, titleUpdate)
                  .setOnMenuItemClickListener(menuItemClickListener);
              break;
            }
          }

          if (status == MapStorage.DOWNLOADING || status == MapStorage.IN_QUEUE)
            menu.add(0, MENU_CANCEL, MENU_CANCEL, mActivity.getString(R.string.cancel_download))
                .setOnMenuItemClickListener(menuItemClickListener);
          if (mHasGoogleStore)
          {
            final GuideInfo info = getGuideInfo(position);
            if (info != null && !TextUtils.isEmpty(info.mTitle))
              menu.add(0, MENU_GUIDE, MENU_GUIDE, info.mTitle)
                  .setOnMenuItemClickListener(menuItemClickListener);
          }

          if (status == MapStorage.NOT_DOWNLOADED)
          {
            // TODO sizes
            String title = mActivity.getString(R.string.downloader_download_map);
            menu.add(0, MENU_DOWNLOAD, MENU_DOWNLOAD, title)
                .setOnMenuItemClickListener(menuItemClickListener);

            title = mActivity.getString(R.string.downloader_download_map_and_routing);
            menu.add(0, MENU_DOWNLOAD_MAP_AND_ROUTING, MENU_DOWNLOAD_MAP_AND_ROUTING, title)
                .setOnMenuItemClickListener(menuItemClickListener);
          }
          if (status == MapStorage.DOWNLOAD_FAILED)
          {
            String title = mActivity.getString(R.string.downloader_retry);
            menu.add(0, MENU_DOWNLOAD, MENU_DOWNLOAD, title)
                .setOnMenuItemClickListener(menuItemClickListener);

            title = mActivity.getString(R.string.downloader_download_map_and_routing);
            menu.add(0, MENU_DOWNLOAD_MAP_AND_ROUTING, MENU_DOWNLOAD_MAP_AND_ROUTING, title)
                .setOnMenuItemClickListener(menuItemClickListener);
          }
        }
      });

      anchor.showContextMenu();
      anchor.setOnCreateContextMenuListener(null);
    }
  }

  protected String getSizeString(long size)
  {
    if (size > Constants.MB)
      return (size + Constants.MB / 2) / Constants.MB + " " + mActivity.getString(R.string.mb);
    else
      return (size + Constants.KB - 1) / Constants.KB + " " + mActivity.getString(R.string.kb);
  }
}
