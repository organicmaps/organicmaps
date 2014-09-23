package com.mapswithme.country;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;
import android.view.View.OnCreateContextMenuListener;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.MapStorage;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.R;
import com.mapswithme.maps.guides.GuideInfo;
import com.mapswithme.maps.guides.GuidesUtils;
import com.mapswithme.util.Constants;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

abstract class BaseDownloadAdapter extends BaseAdapter
{
  static final int TYPE_GROUP = 0;
  static final int TYPE_COUNTRY_GROUP = 1;
  static final int TYPE_COUNTRY_IN_PROCESS = 2;
  static final int TYPE_COUNTRY_READY = 3;
  static final int TYPE_COUNTRY_NOT_DOWNLOADED = 4;
  static final int TYPES_COUNT = 5;

  protected final LayoutInflater mInflater;
  protected final Activity mActivity;
  protected final boolean mHasGoogleStore;
  protected int mSlotID = 0;

  public BaseDownloadAdapter(Activity activity)
  {
    mActivity = activity;
    mInflater = mActivity.getLayoutInflater();
    mHasGoogleStore = Utils.hasAnyGoogleStoreInstalled();
  }

  protected abstract void fillList();

  public abstract void onItemClick(int position, View view);

  protected abstract void expandGroup(int position);

  protected void showNotEnoughFreeSpaceDialog(String spaceNeeded, String countryName)
  {
    final Dialog dlg = new AlertDialog.Builder(mActivity)
        .setMessage(String.format(mActivity.getString(R.string.free_space_for_country), spaceNeeded, countryName))
        .setNegativeButton(mActivity.getString(R.string.close), new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            dlg.dismiss();
          }
        })
        .create();
    dlg.setCanceledOnTouchOutside(true);
    dlg.show();
  }

  protected void processNotDownloaded(final Index idx, final String name)
  {
    final long size = MapStorage.INSTANCE.countryRemoteSizeInBytes(idx);
    if (!MWMApplication.get().hasFreeSpace(size + Constants.MB))
      showNotEnoughFreeSpaceDialog(getSizeString(size), name);
    else
    {
      MapStorage.INSTANCE.downloadCountry(idx);
      Statistics.INSTANCE.trackCountryDownload();
    }
  }

  protected void cancelDownloading(final ViewHolder holder, final Index idx, final String name)
  {
    // Confirm canceling
    final Dialog dlg = new AlertDialog.Builder(mActivity)
        .setTitle(name)
        .setMessage(R.string.are_you_sure)
        .setPositiveButton(R.string.cancel_download, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            final int position = getItemPosition(idx);
            stopItemDownloading(holder, position);
            dlg.dismiss();
          }
        })
        .create();
    dlg.setCanceledOnTouchOutside(true);
    dlg.show();
  }

  protected void processOutOfDate(final Index idx, final String name)
  {
    final long remoteSize = MapStorage.INSTANCE.countryRemoteSizeInBytes(idx);
    if (!MWMApplication.get().hasFreeSpace(remoteSize + Constants.MB))
      showNotEnoughFreeSpaceDialog(getSizeString(remoteSize), name);
    else
    {
      MapStorage.INSTANCE.downloadCountry(idx);
      Statistics.INSTANCE.trackCountryUpdate();
    }
  }

  protected void processOnDisk(final Index idx, final String name)
  {
    // Confirm deleting
    new AlertDialog.Builder(mActivity)
        .setTitle(name)
        .setMessage(R.string.are_you_sure)
        .setPositiveButton(R.string.delete, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            MapStorage.INSTANCE.deleteCountry(idx);
            Statistics.INSTANCE.trackCountryDeleted();
            dlg.dismiss();
          }
        })
        .create()
        .show();
  }

  protected void updateStatuses()
  {
    for (int i = 0; i < getCount(); ++i)
    {
      getItem(i).updateStatus();
    }
  }

  /**
   * Process routine from parent Activity.
   *
   * @return true If "back" was processed.
   */
  public abstract boolean onBackPressed();

  public void onResume(MapStorage.Listener listener)
  {
    if (mSlotID == 0)
      mSlotID = MapStorage.INSTANCE.subscribe(listener);

    updateStatuses();
    notifyDataSetChanged();
  }

  public void onPause()
  {
    if (mSlotID != 0)
    {
      MapStorage.INSTANCE.unsubscribe(mSlotID);
      mSlotID = 0;
    }
  }

  @Override
  public int getItemViewType(int position)
  {
    return getItem(position).getType();
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
    public ImageView mFlag;
    public ProgressBar mProgress;
    public TextView mPercent;
    public TextView mSize;
    private LinearLayout mInfo;
    private TextView mPercentSlided;
    private TextView mSizeSlided;
    private LinearLayout mInfoSlided;
    private ProgressBar mProgressSlided;

    void initFromView(View v)
    {
      mName = (TextView) v.findViewById(R.id.title);
      mFlag = (ImageView) v.findViewById(R.id.country_flag);
      mProgress = (ProgressBar) v.findViewById(R.id.download_progress);
      mPercent = (TextView) v.findViewById(R.id.tv__percent);
      mSize = (TextView) v.findViewById(R.id.tv__size);
      mInfo = (LinearLayout) v.findViewById(R.id.ll__info);
      mPercentSlided = (TextView) v.findViewById(R.id.tv__percent_slided);
      mSizeSlided = (TextView) v.findViewById(R.id.tv__size_slided);
      mInfoSlided = (LinearLayout) v.findViewById(R.id.ll__info_slided);
      mProgressSlided = (ProgressBar) v.findViewById(R.id.download_progress_slided);
    }
  }

  protected String formatStringWithSize(int strID, Index index)
  {
    return mActivity.getString(strID, getSizeString(MapStorage.INSTANCE.countryRemoteSizeInBytes(index)));
  }

  protected void bindFlag(int position, ImageView v)
  {
    final String strID = getItem(position).mFlag;

    int id;
    try
    {
      // works faster than 'getIdentifier()'
      id = R.drawable.class.getField(strID).getInt(null);

      if (id > 0)
      {
        v.setImageResource(id);
        v.setVisibility(View.VISIBLE);
      }
      else
        v.setVisibility(View.GONE);
    } catch (final Exception e)
    {
      v.setVisibility(View.GONE);
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

    switch (type)
    {
    case TYPE_GROUP:
      bindGroup(position, type, holder);
      break;

    case TYPE_COUNTRY_GROUP:
      bindRegion(position, type, holder);
      break;

    case TYPE_COUNTRY_IN_PROCESS:
    case TYPE_COUNTRY_READY:
    case TYPE_COUNTRY_NOT_DOWNLOADED:
      bindCountry(position, type, holder);
      break;
    }

    setItemName(position, holder);
    convertView.setBackgroundResource(R.drawable.list_selector_holo_light);
    final View fview = convertView;

    convertView.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        onItemClick(position, fview);
      }
    });

    return convertView;
  }

  private int getLayoutForType(int type)
  {
    switch (type)
    {
    case TYPE_GROUP:
      return R.layout.download_item_group;

    case TYPE_COUNTRY_GROUP:
      return R.layout.download_item_country_group;

    case TYPE_COUNTRY_IN_PROCESS:
    case TYPE_COUNTRY_READY:
    case TYPE_COUNTRY_NOT_DOWNLOADED:
      return R.layout.download_item_country;
    }

    return 0;
  }

  protected void bindCountry(int position, int type, BaseDownloadAdapter.ViewHolder holder)
  {
    bindFlag(position, holder.mFlag);
    bindSizeAndProgress(holder, type, position);
  }

  protected void bindSizeAndProgress(final BaseDownloadAdapter.ViewHolder holder, final int type, final int position)
  {
    final CountryItem item = getItem(position);
    if (holder.mSize != null)
      setHolderSizeString(holder, MapStorage.INSTANCE.countryRemoteSizeInBytes(item.mCountryIdx));

    switch (getItem(position).getStatus())
    {
    case MapStorage.DOWNLOADING:
      holder.mProgress.setVisibility(View.GONE);
      holder.mProgressSlided.setVisibility(View.VISIBLE);
      holder.mInfoSlided.clearAnimation();
      holder.mInfoSlided.setVisibility(View.VISIBLE);
      holder.mInfo.setVisibility(View.GONE);
      // FIXME
      setHolderPercentString(holder, "0%", R.color.downloader_gray_bg);
      break;
    case MapStorage.ON_DISK_OUT_OF_DATE:
      holder.mProgress.setVisibility(View.GONE);
      holder.mProgressSlided.setVisibility(View.GONE);
      holder.mInfoSlided.setVisibility(View.GONE);
      holder.mInfo.setVisibility(View.VISIBLE);
      setHolderPercentString(holder, "UPDATE", R.color.downloader_green);
      break;
    case MapStorage.ON_DISK:
      holder.mProgress.setVisibility(View.GONE);
      holder.mProgressSlided.setVisibility(View.GONE);
      holder.mInfoSlided.setVisibility(View.GONE);
      holder.mInfo.setVisibility(View.VISIBLE);
      setHolderPercentString(holder, "DOWNLOADED", R.color.downloader_gray_bg);
      break;
    case MapStorage.DOWNLOAD_FAILED:
      holder.mProgress.setVisibility(View.GONE);
      holder.mProgressSlided.setVisibility(View.GONE);
      holder.mInfoSlided.setVisibility(View.GONE);
      holder.mInfo.setVisibility(View.VISIBLE);
      setHolderPercentString(holder, "FAILED", R.color.downloader_red);
      break;
    case MapStorage.IN_QUEUE:
      holder.mProgress.setVisibility(View.GONE);
      holder.mProgressSlided.setVisibility(View.VISIBLE);
      holder.mProgressSlided.setProgress(0);
      holder.mInfoSlided.clearAnimation();
      holder.mInfoSlided.setVisibility(View.VISIBLE);
      holder.mInfo.setVisibility(View.GONE);
      // FIXME
      setHolderPercentString(holder, "0%", R.color.downloader_gray_bg);
      break;
    case MapStorage.NOT_DOWNLOADED:
      holder.mProgress.setVisibility(View.GONE);
      holder.mProgressSlided.setVisibility(View.GONE);
      holder.mInfo.clearAnimation();
      holder.mInfo.setVisibility(View.VISIBLE);
      holder.mInfoSlided.setVisibility(View.GONE);
      setHolderPercentString(holder, "DOWNLOAD", R.color.downloader_green);
      holder.mInfo.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          startItemDownloading(holder, position);
        }
      });
      holder.mProgress.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          stopItemDownloading(holder, position);
        }
      });
      holder.mProgress.setVisibility(View.GONE);

      holder.mInfoSlided.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          stopItemDownloading(holder, position);
        }
      });
      break;

    }
  }

  private void startItemDownloading(final ViewHolder holder, final int position)
  {
    // TODO get actual percent of download
    holder.mProgressSlided.setProgress(0);
    holder.mProgressSlided.clearAnimation();
    holder.mInfoSlided.clearAnimation();
    setHolderPercentString(holder, "0%", R.color.downloader_gray_bg);
    Animation slideAnim = UiUtils.generateAbsoluteSlideAnimation(0,
        -mActivity.getResources().getDimensionPixelOffset(R.dimen.margin_large), 0, 0);
    slideAnim.setDuration(500);
    slideAnim.setFillAfter(true);
    holder.mInfo.startAnimation(slideAnim);

    slideAnim = UiUtils.generateAbsoluteSlideAnimation(0,
        -mActivity.getResources().getDimensionPixelOffset(R.dimen.margin_large), 0, 0);
    slideAnim.setDuration(500);
    slideAnim.setFillAfter(true);
    slideAnim.setAnimationListener(
        new UiUtils.SimpleAnimationListener()
        {
          @Override
          public void onAnimationEnd(Animation animation)
          {
            final CountryItem item = getItem(position);
            processNotDownloaded(item.mCountryIdx, item.mName);
            holder.mInfoSlided.setVisibility(View.VISIBLE);
            holder.mInfoSlided.bringToFront();
            holder.mInfo.setVisibility(View.GONE);
            holder.mProgressSlided.setVisibility(View.VISIBLE);
            holder.mProgress.setVisibility(View.GONE);
            holder.mProgress.clearAnimation();
          }
        }
    );
    holder.mProgress.startAnimation(slideAnim);
    holder.mProgress.setVisibility(View.VISIBLE);
  }

  private void stopItemDownloading(final ViewHolder holder, final int position)
  {
    Animation slideAnim = UiUtils.generateAbsoluteSlideAnimation(0,
        mActivity.getResources().getDimensionPixelOffset(R.dimen.margin_large), 0, 0);
    slideAnim.setDuration(500);
    slideAnim.setFillAfter(true);
    slideAnim.setAnimationListener(
        new UiUtils.SimpleAnimationListener()
        {
          @Override
          public void onAnimationEnd(Animation animation)
          {
            holder.mInfo.bringToFront();
            holder.mInfo.setVisibility(View.VISIBLE);
            holder.mInfoSlided.setVisibility(View.GONE);
            holder.mProgressSlided.setVisibility(View.GONE);
            holder.mProgressSlided.clearAnimation();
            holder.mProgress.setVisibility(View.GONE);
            holder.mProgress.clearAnimation();
            MapStorage.INSTANCE.deleteCountry(getItem(position).mCountryIdx);
          }
        }
    );
    holder.mInfoSlided.startAnimation(slideAnim);

    slideAnim = UiUtils.generateAbsoluteSlideAnimation(0,
        mActivity.getResources().getDimensionPixelOffset(R.dimen.margin_large), 0, 0);
    slideAnim.setDuration(500);
    slideAnim.setFillAfter(true);
    holder.mProgressSlided.startAnimation(slideAnim);
  }

  @Override
  public abstract CountryItem getItem(int position);

  private void setHolderSizeString(ViewHolder holder, long size)
  {
    holder.mSize.setText(getSizeString(size));
    holder.mSizeSlided.setText(getSizeString(size));
  }

  private void setHolderPercentString(ViewHolder holder, String text, int color)
  {
    holder.mPercent.setText(text);
    holder.mPercent.setTextColor(mActivity.getResources().getColor(color));
    holder.mPercentSlided.setText(text);
    holder.mPercentSlided.setTextColor(mActivity.getResources().getColor(color));
  }

  protected void bindGroup(int position, int type, BaseDownloadAdapter.ViewHolder holder)
  {
  }

  protected void bindRegion(int position, int type, ViewHolder holder)
  {
    bindFlag(position, holder.mFlag);
  }

  protected void setItemName(int position, BaseDownloadAdapter.ViewHolder holder)
  {
    // set name and style
    final CountryItem item = getItem(position);
    holder.mName.setText(item.mName);
    holder.mName.setTypeface(item.getTypeface());
    holder.mName.setTextColor(item.getTextColor());
  }

  protected abstract int getItemPosition(Index idx);

  /**
   * @param idx
   * @return Current country status (@see MapStorage).
   */
  public int onCountryStatusChanged(Index idx)
  {
    final int position = getItemPosition(idx);
    if (position != -1)
    {
      getItem(position).updateStatus();
      // use this hard reset, because of caching different ViewHolders according to item's type
      notifyDataSetChanged();
      return getItem(position).getStatus();
    }
    return MapStorage.UNKNOWN;
  }

  public void onCountryProgress(ListView list, Index idx, long current, long total)
  {
    final int position = getItemPosition(idx);
    if (position != -1)
    {
      // do update only one item's view; don't call notifyDataSetChanged
      final View v = list.getChildAt(position - list.getFirstVisiblePosition());
      if (v != null)
      {
        final BaseDownloadAdapter.ViewHolder holder = (BaseDownloadAdapter.ViewHolder) v.getTag();
        if (holder != null && holder.mProgress != null)
        {
          final int percent = (int) (current * 100 / total);
          holder.mProgressSlided.setProgress(percent);
          holder.mPercent.setText(percent + "%");
          holder.mPercentSlided.setText(percent + "%");
        }
      }
    }
  }

  protected void showCountry(final Index countryIndex)
  {
    Framework.nativeShowCountry(countryIndex, false);
    mActivity.finish();
  }

  protected void onCountryMenuClicked(final CountryItem countryItem, final View anchor)
  {
    final int MENU_DELETE = 0;
    final int MENU_UPDATE = 1;
    final int MENU_DOWNLOAD = 2;
    final int MENU_CANCEL = 3;
    final int MENU_SHOW = 4;
    final int MENU_GUIDE = 5;

    final int status = countryItem.getStatus();
    final Index countryIndex = countryItem.mCountryIdx;
    final String name = countryItem.mName;

    final OnMenuItemClickListener menuItemClickListener = new OnMenuItemClickListener()
    {
      @Override
      public boolean onMenuItemClick(MenuItem item)
      {
        final int id = item.getItemId();

        if (MENU_DELETE == id)
          processOnDisk(countryIndex, name);
        else if (MENU_UPDATE == id)
          processOutOfDate(countryIndex, name);
        else if (MENU_GUIDE == id)
          GuidesUtils.openOrDownloadGuide(Framework.getGuideInfoForIndexWithApiCheck(countryIndex), mActivity);
        else if (MENU_DOWNLOAD == id)
          processNotDownloaded(countryIndex, name);
        else if (MENU_CANCEL == id)
          cancelDownloading((ViewHolder) anchor.getTag(), countryIndex, name);
        else if (MENU_SHOW == id)
          showCountry(countryIndex);
        else
          return false;

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
          menu.setHeaderTitle(countryItem.mName);

          if (status == MapStorage.ON_DISK || status == MapStorage.ON_DISK_OUT_OF_DATE)
          {
            final String titleDelete = mActivity.getString(R.string.delete) + " "
                + getSizeString(MapStorage.INSTANCE.countryLocalSizeInBytes(countryIndex));
            menu.add(0, MENU_DELETE, MENU_DELETE, titleDelete).setOnMenuItemClickListener(menuItemClickListener);

            final String titleShow = mActivity.getString(R.string.zoom_to_country);
            menu.add(0, MENU_SHOW, MENU_SHOW, titleShow).setOnMenuItemClickListener(menuItemClickListener);
          }

          if (status == MapStorage.ON_DISK_OUT_OF_DATE)
          {
            final String titleUpdate = formatStringWithSize(R.string.update_mb_or_kb, countryIndex);
            menu.add(0, MENU_UPDATE, MENU_UPDATE, titleUpdate)
                .setOnMenuItemClickListener(menuItemClickListener);
          }

          if (status == MapStorage.DOWNLOADING || status == MapStorage.IN_QUEUE)
            menu.add(0, MENU_CANCEL, MENU_CANCEL, mActivity.getString(R.string.cancel_download))
                .setOnMenuItemClickListener(menuItemClickListener);

          if (mHasGoogleStore)
          {
            final GuideInfo info = Framework.getGuideInfoForIndexWithApiCheck(countryItem.mCountryIdx);
            if (info != null)
              menu.add(0, MENU_GUIDE, MENU_GUIDE, info.mTitle)
                  .setOnMenuItemClickListener(menuItemClickListener);
          }

          if (status == MapStorage.NOT_DOWNLOADED || status == MapStorage.DOWNLOAD_FAILED)
          {
            final String titleDownload = formatStringWithSize(R.string.download_mb_or_kb, countryIndex);
            menu.add(0, MENU_DOWNLOAD, MENU_DOWNLOAD, titleDownload)
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

  protected abstract boolean isRoot();
}
