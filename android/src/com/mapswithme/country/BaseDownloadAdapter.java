package com.mapswithme.country;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;
import android.view.View.OnCreateContextMenuListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
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
  protected Index mIdx = new Index();
  protected CountryItem[] mItems;

  public BaseDownloadAdapter(Activity activity)
  {
    mActivity = activity;
    mInflater = mActivity.getLayoutInflater();

    mHasGoogleStore = Utils.hasAnyGoogleStoreInstalled();
    fillList();
  }

  protected abstract void fillList();

  public void onItemClick(int position, View view)
  {
    if (position >= mItems.length)
      return; // we have reports at GP that it crashes.

    if (mItems[position].getStatus() < 0)
    {
      // expand next level
      mIdx = mIdx.getChild(position);
      fillList();
    }
    else
      onCountryMenuClicked(getItem(position), view);
  }

  protected  void showNotEnoughFreeSpaceDialog(String spaceNeeded, String countryName)
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

  protected  void processNotDownloaded(final Index idx, final String name)
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

  protected  void processDownloading(final Index idx, final String name)
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
            MapStorage.INSTANCE.deleteCountry(idx);
            dlg.dismiss();
          }
        })
        .create();
    dlg.setCanceledOnTouchOutside(true);
    dlg.show();
  }

  protected  void processOutOfDate(final Index idx, final String name)
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

  protected  void processOnDisk(final Index idx, final String name)
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

  protected  void updateStatuses()
  {
    for (int i = 0; i < mItems.length; ++i)
    {
      final Index idx = mIdx.getChild(i);
      if (idx.isValid())
        mItems[i].updateStatus(idx);
    }
  }


  /**
   * Process routine from parent Activity.
   * @return true If "back" was processed.
   */
  public boolean onBackPressed()
  {
    if (mIdx.isRoot())
      return false;

    mIdx = mIdx.getParent();

    fillList();
    return true;
  }

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
    return mItems[position].getType();
  }

  @Override
  public int getViewTypeCount()
  {
    return TYPES_COUNT;
  }

  @Override
  public int getCount()
  {
    return (mItems != null ? mItems.length : 0);
  }

  @Override
  public CountryItem getItem(int position)
  {
    return mItems[position];
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

    void initFromView(View v)
    {
      mName = (TextView) v.findViewById(R.id.title);
      mFlag = (ImageView) v.findViewById(R.id.country_flag);
      mProgress = (ProgressBar) v.findViewById(R.id.download_progress);
    }
  }

  protected  String formatStringWithSize(int strID, Index index)
  {
    return mActivity.getString(strID, getSizeString(MapStorage.INSTANCE.countryRemoteSizeInBytes(index)));
  }

  protected  void setFlag(int position, ImageView v)
  {
    final String strID = mItems[position].mFlag;

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
    ViewHolder holder = null;
    final int type = getItemViewType(position);

    if (convertView == null)
    {
      holder = new ViewHolder();
      switch (type)
      {
      case TYPE_GROUP:
        convertView = mInflater.inflate(R.layout.download_item_group, parent, false);
        holder.initFromView(convertView);
        break;

      case TYPE_COUNTRY_GROUP:
        convertView = mInflater.inflate(R.layout.download_item_country_group, parent, false);
        holder.initFromView(convertView);
        break;

      case TYPE_COUNTRY_IN_PROCESS:
      case TYPE_COUNTRY_READY:
      case TYPE_COUNTRY_NOT_DOWNLOADED:
        convertView = mInflater.inflate(R.layout.download_item_country, parent, false);
        holder.initFromView(convertView);
        break;
      }

      convertView.setTag(holder);
    }
    else
      holder = (ViewHolder) convertView.getTag();

    // for everything that has flag: regions + countries
    if (type != TYPE_GROUP)
    {
      setFlag(position, holder.mFlag);

      // this part if only for downloadable items
      if (type != TYPE_COUNTRY_GROUP)
        setUpProgress(holder, type, position);
    }

    setItemText(position, holder);
    convertView.setBackgroundResource(R.drawable.list_selector_holo_light);

    return convertView;
  }


  protected  void setUpProgress(BaseDownloadAdapter.ViewHolder holder, int type, int position)
  {
    if (type == TYPE_COUNTRY_IN_PROCESS && getItem(position).getStatus() != MapStorage.DOWNLOAD_FAILED)
    {
      holder.mProgress.setProgress(0);
      UiUtils.show(holder.mProgress);
    }
    else
      UiUtils.invisible(holder.mProgress);
  }

  protected  void setItemText(int position, BaseDownloadAdapter.ViewHolder holder)
  {
    // set text and style
    final CountryItem item = mItems[position];
    holder.mName.setText(item.mName);
    holder.mName.setTypeface(item.getTypeface());
    holder.mName.setTextColor(item.getTextColor());

    Drawable drawable = null; // by default county item has no icon
    final int status = getItem(position).getStatus();
    if (status == MapStorage.ON_DISK_OUT_OF_DATE) // out of date has Aquarius icon
      drawable = mActivity.getResources().getDrawable(R.drawable.ic_update);
    else if (status == MapStorage.ON_DISK)
      drawable = mActivity.getResources().getDrawable(R.drawable.ic_downloaded_country); // downloaded has Tick icon

    holder.mName.setCompoundDrawablesWithIntrinsicBounds(null, null, drawable, null);
  }

  /**
   *
   * @param idx
   * @return -1 If no such item in display list.
   */
  protected  int getItemPosition(Index idx)
  {
    if (mIdx.isChild(idx))
    {
      final int position = idx.getPosition();
      if (position >= 0 && position < mItems.length)
        return position;
      else
        Log.e(DownloadActivity.TAG, "Incorrect item position for: " + idx.toString());
    }
    return -1;
  }

  /**
   * @param idx
   * @return Current country status (@see MapStorage).
   */
  public int onCountryStatusChanged(Index idx)
  {
    final int position = getItemPosition(idx);
    if (position != -1)
    {
      mItems[position].updateStatus(idx);
      // use this hard reset, because of caching different ViewHolders according to item's type
      notifyDataSetChanged();
      return mItems[position].getStatus();
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
          holder.mProgress.setProgress((int) (current * 100 / total));
      }
    }
  }

  protected  void showCountry(final Index countryIndex)
  {
    Framework.nativeShowCountry(countryIndex, false);
    mActivity.finish();
  }

  protected  void onCountryMenuClicked(final CountryItem countryItem, final View anchor)
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
          processDownloading(countryIndex, name);
        else if (MENU_SHOW == id)
          showCountry(countryIndex);
        else
          return false;

        return true;
      }
    };

    Log.d("TEST", "OnItemShow!");
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

  protected  String getSizeString(long size)
  {
    if (size > Constants.MB)
      return (size + Constants.MB / 2) / Constants.MB + " " + mActivity.getString(R.string.mb);
    else
      return (size + Constants.KB - 1) / Constants.KB + " " + mActivity.getString(R.string.kb);
  }
}
