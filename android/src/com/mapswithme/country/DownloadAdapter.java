package com.mapswithme.country;

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;
import android.view.View.OnClickListener;
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
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

/// ListView adapter
class DownloadAdapter extends BaseAdapter
{
  /// @name Different row types.
  //@{
  private static final int TYPE_GROUP = 0;
  private static final int TYPE_COUNTRY_GROUP = 1;
  private static final int TYPE_COUNTRY_IN_PROCESS = 2;
  private static final int TYPE_COUNTRY_READY = 3;
  private static final int TYPE_COUNTRY_NOT_DOWNLOADED = 4;
  private static final int TYPES_COUNT = 5;
  //@}

  //{@ System
  private final LayoutInflater mInflater;
  private final Activity mContext;
  private final ExecutorService executor =
      Executors.newFixedThreadPool(Runtime.getRuntime().availableProcessors());
  private final boolean mHasGoogleStore;
  //@}

  //@{ Map
  private int mSlotID = 0;
  final MapStorage mStorage;
  private Index mIdx = new Index();
  private DownloadAdapter.CountryItem[] mItems = null;
  //@}

  private static final Typeface LIGHT = Typeface.create("sans-serif-light", Typeface.NORMAL);
  private static final Typeface REGULAR = Typeface.create("sans-serif", Typeface.NORMAL);

  private class CountryItem
  {
    public final String mName;
    public final Index  mCountryIdx;
    public final String mFlag;

    private Future<Integer> mStatusFuture;

    public CountryItem(MapStorage storage, Index idx)
    {
      mCountryIdx = idx;
      mName = storage.countryName(idx);

      final String flag = storage.countryFlag(idx);
      // The aapt can't process resources with name "do". Hack with renaming.
      mFlag = flag.equals("do") ? "do_hack" : flag;

      updateStatus(storage, idx);
    }

    public int getStatus()
    {
      try
      {
        return mStatusFuture.get();
      }
      catch (final Exception e)
      {
        throw new RuntimeException(e);
      }
    }

    /**
     * Asynchronous blocking status update.
     */
    public void updateStatus(final MapStorage storage, final Index idx)
    {
      mStatusFuture = executor.submit(new Callable<Integer>()
      {
        @Override
        public Integer call() throws Exception
        {
          if (idx.getCountry() == -1 || (idx.getRegion() == -1 && mFlag.length() == 0))
            return MapStorage.GROUP;
          else if (idx.getRegion() == -1 && storage.countriesCount(idx) > 0)
            return MapStorage.COUNTRY;
          else
            return storage.countryStatus(idx);
        }});
    }

    public int getTextColor()
    {
      switch (getStatus())
      {
      case MapStorage.ON_DISK_OUT_OF_DATE:
        return 0xFF666666;
      case MapStorage.NOT_DOWNLOADED:
        return 0xFF333333;
      case MapStorage.DOWNLOAD_FAILED:
        return 0xFFFF0000;
      default:
        return 0xFF000000;
      }
    }

    public Typeface getTypeface()
    {
      switch (getStatus())
      {
        case MapStorage.NOT_DOWNLOADED:
        case MapStorage.DOWNLOADING:
        case MapStorage.DOWNLOAD_FAILED:
          return LIGHT;
        default:
          return REGULAR;
      }
    }

    /// Get item type for list view representation;
    public int getType()
    {
      switch (getStatus())
      {
      case MapStorage.GROUP:          return TYPE_GROUP;
      case MapStorage.COUNTRY:        return TYPE_COUNTRY_GROUP;
      case MapStorage.NOT_DOWNLOADED: return TYPE_COUNTRY_NOT_DOWNLOADED;

      case MapStorage.ON_DISK:
      case MapStorage.ON_DISK_OUT_OF_DATE:
        return TYPE_COUNTRY_READY;

      default : return TYPE_COUNTRY_IN_PROCESS;
      }
    }
  }

  public DownloadAdapter(Activity context)
  {
    mStorage = MWMApplication.get().getMapStorage();
    mContext = context;
    mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

    mHasGoogleStore = Utils.hasAnyGoogleStoreInstalled();
    fillList();
  }

  /// Fill list for current m_group and m_country.
  private void fillList()
  {
    final int count = mStorage.countriesCount(mIdx);
    if (count > 0)
    {
      mItems = new DownloadAdapter.CountryItem[count];
      for (int i = 0; i < count; ++i)
        mItems[i] = new CountryItem(mStorage, mIdx.getChild(i));
    }

    notifyDataSetChanged();
  }

  /// Process list item click.
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
        onCountryMenuClicked(position, getItem(position), view);
  }

  private void showNotEnoughFreeSpaceDialog(String spaceNeeded, String countryName)
  {
    final Dialog dlg = new AlertDialog.Builder(mContext)
      .setMessage(String.format(mContext.getString(R.string.free_space_for_country), spaceNeeded, countryName))
      .setNegativeButton(mContext.getString(R.string.close), new DialogInterface.OnClickListener()
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

  private boolean hasFreeSpace(long size)
  {
    return MWMApplication.get().hasFreeSpace(size);
  }

  private void processNotDownloaded(final Index idx, final String name)
  {
    final long size = mStorage.countryRemoteSizeInBytes(idx);
    if (!hasFreeSpace(size + MB))
      showNotEnoughFreeSpaceDialog(getSizeString(size), name);
    else
    {
      mStorage.downloadCountry(idx);
      Statistics.INSTANCE.trackCountryDownload(mContext);
    }
  }

  private void processDownloading(final Index idx, final String name)
  {
    // Confirm canceling
    final Dialog dlg = new AlertDialog.Builder(mContext)
      .setTitle(name)
      .setMessage(R.string.are_you_sure)
      .setPositiveButton(R.string.cancel_download, new DialogInterface.OnClickListener()
      {
        @Override
        public void onClick(DialogInterface dlg, int which)
        {
          mStorage.deleteCountry(idx);
          dlg.dismiss();
        }
      })
      .create();
    dlg.setCanceledOnTouchOutside(true);
    dlg.show();
  }

  private void processOutOfDate(final Index idx, final String name)
  {
    final long remoteSize = mStorage.countryRemoteSizeInBytes(idx);
    if (!hasFreeSpace(remoteSize + MB))
      showNotEnoughFreeSpaceDialog(getSizeString(remoteSize), name);
    else
    {
      mStorage.downloadCountry(idx);
      Statistics.INSTANCE.trackCountryUpdate(mContext);
    }
  }

  private void processOnDisk(final Index idx, final String name)
  {
    // Confirm deleting
    new AlertDialog.Builder(mContext)
      .setTitle(name)
      .setMessage(R.string.are_you_sure)
      .setPositiveButton(R.string.delete, new DialogInterface.OnClickListener()
      {
        @Override
        public void onClick(DialogInterface dlg, int which)
        {
          mStorage.deleteCountry(idx);
          Statistics.INSTANCE.trackCountryDeleted(mContext);
          dlg.dismiss();
        }
      })
      .create()
      .show();
  }

  private void updateStatuses()
  {
    for (int i = 0; i < mItems.length; ++i)
    {
      final Index idx = mIdx.getChild(i);
      if (idx.isValid())
        mItems[i].updateStatus(mStorage, idx);
    }
  }

  /// @name Process routine from parent Activity.
  //@{
  /// @return true If "back" was processed.
  public boolean onBackPressed()
  {
    // we are on the root level already - return
    if (mIdx.isRoot())
      return false;

    // go to the parent level
    mIdx = mIdx.getParent();

    fillList();
    return true;
  }

  public void onResume(MapStorage.Listener listener)
  {
    if (mSlotID == 0)
      mSlotID = mStorage.subscribe(listener);

    // update actual statuses for items after resuming activity
    updateStatuses();
    notifyDataSetChanged();
  }

  public void onPause()
  {
    if (mSlotID != 0)
    {
      mStorage.unsubscribe(mSlotID);
      mSlotID = 0;
    }
  }
  //@}

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
  public DownloadAdapter.CountryItem getItem(int position)
  {
    return mItems[position];
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  private static class ViewHolder
  {
    public TextView      mName        = null;
    public ImageView     mFlag        = null;
    public ImageView     mGuide       = null;
    public ProgressBar   mProgress    = null;

    void initFromView(View v)
    {
      mName        = (TextView) v.findViewById(R.id.title);
      mFlag        = (ImageView) v.findViewById(R.id.country_flag);
      mGuide       = (ImageView) v.findViewById(R.id.guide_available);
      mProgress    = (ProgressBar) v.findViewById(R.id.download_progress);
    }
  }

  private String formatStringWithSize(int strID, Index index)
  {
    return mContext.getString(strID, getSizeString(mStorage.countryRemoteSizeInBytes(index)));
  }

  private void setFlag(int position, ImageView v)
  {
    final String strID = mItems[position].mFlag;

    int id = -1;
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
        v.setVisibility(View.INVISIBLE);
    }
    catch (final Exception e)
    {
      v.setVisibility(View.INVISIBLE);
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
        convertView = mInflater.inflate(R.layout.download_item_group, null);
        holder.initFromView(convertView);
        break;

      case TYPE_COUNTRY_GROUP:
        convertView = mInflater.inflate(R.layout.download_item_country_group, null);
        holder.initFromView(convertView);
        break;

      case TYPE_COUNTRY_IN_PROCESS:
      case TYPE_COUNTRY_READY:
      case TYPE_COUNTRY_NOT_DOWNLOADED:
        convertView = mInflater.inflate(R.layout.download_item_country, null);
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
      {
        populateForGuide(position, holder);
        setUpProgress(holder, type, position);
      }
    }

    setItemText(position, holder);
    convertView.setBackgroundResource(R.drawable.list_selector_holo_light);
    final View fview = convertView;

    convertView.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        onItemClick(position, fview);
      }
    });

    return convertView;
  }



  private void setUpProgress(DownloadAdapter.ViewHolder holder, int type, int position)
  {
    if (type == TYPE_COUNTRY_IN_PROCESS && getItem(position).getStatus() != MapStorage.DOWNLOAD_FAILED)
    {
      holder.mProgress.setProgress(0);
      UiUtils.show(holder.mProgress);
    }
    else
      UiUtils.invisible(holder.mProgress);
  }

  private void setItemText(int position, DownloadAdapter.ViewHolder holder)
  {
    // set text and style
    final CountryItem item = mItems[position];
    holder.mName.setText(item.mName);
    holder.mName.setTypeface(item.getTypeface());
    holder.mName.setTextColor(item.getTextColor());

    Drawable drawable = null; // by default county item has no icon
    final int status = getItem(position).getStatus();
    if (status == MapStorage.ON_DISK_OUT_OF_DATE) // out of date has Aquarius icon
      drawable = mContext.getResources().getDrawable(R.drawable.ic_update);
    else if (status == MapStorage.ON_DISK)
      drawable = mContext.getResources().getDrawable(R.drawable.ic_downloaded_country); // downloaded has Tick icon

    holder.mName.setCompoundDrawablesWithIntrinsicBounds(null, null, drawable, null);
  }

  private void populateForGuide(int position, DownloadAdapter.ViewHolder holder)
  {
    if (mHasGoogleStore)
    {
      final CountryItem item = getItem(position);
      final GuideInfo gi = Framework.getGuideInfoForIndexWithApiCheck(item.mCountryIdx);
      if (gi != null)
        UiUtils.show(holder.mGuide);
      else
        UiUtils.hide(holder.mGuide);
    }
  }

  /// Get list item position by index(g, c, r).
  /// @return -1 If no such item in display list.
  private int getItemPosition(Index idx)
  {
    if (mIdx.isChild(idx))
    {
      final int position = idx.getPosition();
      if (position >= 0 && position < mItems.length)
        return position;
      else
        Log.e(DownloadUI.TAG, "Incorrect item position for: " + idx.toString());
    }
    return -1;
  }

  /// @return Current country status (@see MapStorage).
  public int onCountryStatusChanged(Index idx)
  {
    final int position = getItemPosition(idx);
    if (position != -1)
    {
      mItems[position].updateStatus(mStorage, idx);
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
        final DownloadAdapter.ViewHolder holder = (DownloadAdapter.ViewHolder) v.getTag();
        if (holder != null)
          holder.mProgress.setProgress((int) (current*100/total));
      }
    }
  }

  private void showCountry(final Index countryIndex)
  {
    mStorage.showCountry(countryIndex);
    mContext.finish();
  }

  private void onCountryMenuClicked(int position, final CountryItem countryItem, final View anchor)
  {
    final int MENU_DELETE   = 0;
    final int MENU_UPDATE   = 1;
    final int MENU_DOWNLOAD = 2;
    final int MENU_CANCEL   = 3;
    final int MENU_SHOW     = 4;
    final int MENU_GUIDE    = 5;

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
          GuidesUtils.openOrDownloadGuide(Framework.getGuideInfoForIndexWithApiCheck(countryIndex), mContext);
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

    anchor.setOnCreateContextMenuListener(new OnCreateContextMenuListener()
    {
      @Override
      public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
      {
        menu.setHeaderTitle(countryItem.mName);

        if (status == MapStorage.ON_DISK || status == MapStorage.ON_DISK_OUT_OF_DATE)
        {
          final String titleDelete = mContext.getString(R.string.delete) + " "
              + getSizeString(mStorage.countryLocalSizeInBytes(countryIndex));
          menu.add(0, MENU_DELETE, MENU_DELETE, titleDelete).setOnMenuItemClickListener(menuItemClickListener);

          final String titleShow = mContext.getString(R.string.zoom_to_country);
          menu.add(0, MENU_SHOW, MENU_SHOW, titleShow).setOnMenuItemClickListener(menuItemClickListener);
        }

        if (status == MapStorage.ON_DISK_OUT_OF_DATE)
        {
          final String titleUpdate = formatStringWithSize(R.string.update_mb_or_kb, countryIndex);
          menu.add(0, MENU_UPDATE, MENU_UPDATE, titleUpdate)
            .setOnMenuItemClickListener(menuItemClickListener);
        }

        if (status == MapStorage.DOWNLOADING || status == MapStorage.IN_QUEUE)
          menu.add(0, MENU_CANCEL, MENU_CANCEL, mContext.getString(R.string.cancel_download))
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

  private String getSizeString(long size)
  {
    if (size > MB)
      return (size + 512 * 1024) / MB + " " + mContext.getString(R.string.mb);
    else
      return (size + 1023) / 1024 + " " + mContext.getString(R.string.kb);
  }
  private final static long MB = 1024 * 1024;
}
