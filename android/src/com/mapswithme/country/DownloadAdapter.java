package com.mapswithme.country;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
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

  private final LayoutInflater mInflater;
  private final Activity mContext;

  private int mSlotID = 0;
  final MapStorage mStorage;

  private final String mPackageName;

  private static class CountryItem
  {
    public final String mName;
    public final Index  mCountryIdx;
    public final String mFlag;

    /// @see constants in MapStorage
    public int mStatus;

    public CountryItem(MapStorage storage, Index idx)
    {
      mCountryIdx = idx;
      mName = storage.countryName(idx);

      final String flag = storage.countryFlag(idx);
      // The aapt can't process resources with name "do". Hack with renaming.
      mFlag = flag.equals("do") ? "do_hack" : flag;

      updateStatus(storage, idx);
    }

    public void updateStatus(MapStorage storage, Index idx)
    {
      if (idx.getCountry() == -1 || (idx.getRegion() == -1 && mFlag.length() == 0))
        mStatus = MapStorage.GROUP;
      else if (idx.getRegion() == -1 && storage.countriesCount(idx) > 0)
        mStatus = MapStorage.COUNTRY;
      else
        mStatus = storage.countryStatus(idx);
    }

    public int getTextColor()
    {
      //TODO introduce resources
      switch (mStatus)
      {
      case MapStorage.ON_DISK:             return 0xFF00A144;
      case MapStorage.ON_DISK_OUT_OF_DATE: return 0xFFFF69B4;
      case MapStorage.NOT_DOWNLOADED:      return 0xFF000000;
      case MapStorage.DOWNLOAD_FAILED:     return 0xFFFF0000;
      case MapStorage.DOWNLOADING:         return 0xFF342BB6;
      case MapStorage.IN_QUEUE:            return 0xFF5B94DE;
      default:                             return 0xFF000000;
      }
    }

    /// Get item type for list view representation;
    public int getType()
    {
      switch (mStatus)
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

  private Index mIdx = new Index();
  private DownloadAdapter.CountryItem[] mItems = null;

  private final String m_kb;
  private final String m_mb;

  private final boolean mHasGoogleStore;

  private final AlertDialog.Builder mAlert;
  private final DialogInterface.OnClickListener m_alertCancelHandler =
      new DialogInterface.OnClickListener()
  {
    @Override
    public void onClick(DialogInterface dlg, int which)
    {
      dlg.dismiss();
    }
  };

  public DownloadAdapter(Activity context)
  {
    final MWMApplication app = (MWMApplication) context.getApplication();
    mStorage = app.getMapStorage();
    mPackageName = app.getPackageName();

    mContext = context;
    mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

    m_kb = context.getString(R.string.kb);
    m_mb = context.getString(R.string.mb);

    mAlert = new AlertDialog.Builder(mContext);

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
  public boolean onItemClick(int position)
  {
    if (mItems[position].mStatus < 0)
    {
      // expand next level
      mIdx = mIdx.getChild(position);

      fillList();
      return true;
    }
    else
    {
      processCountry(position);
      return false;
    }
  }

  private void showNotEnoughFreeSpaceDialog(String spaceNeeded, String countryName)
  {
    new AlertDialog.Builder(mContext)
    .setMessage(String.format(mContext.getString(R.string.free_space_for_country), spaceNeeded, countryName))
    .setNegativeButton(mContext.getString(R.string.close), m_alertCancelHandler)
    .create()
    .show();
  }

  private boolean hasFreeSpace(long size)
  {
    final MWMApplication app = (MWMApplication) mContext.getApplication();
    return app.hasFreeSpace(size);
  }

  private void processCountry(int position)
  {
    final Index idx = mIdx.getChild(position);
    final String name = mItems[position].mName;

    // Get actual status here
    switch (mStorage.countryStatus(idx))
    {
    case MapStorage.ON_DISK:
      // Confirm deleting
      mAlert
      .setTitle(name)
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
      .setNegativeButton(R.string.cancel, m_alertCancelHandler)
      .create()
      .show();
      break;

    case MapStorage.ON_DISK_OUT_OF_DATE:
      final long remoteSize = mStorage.countryRemoteSizeInBytes(idx);

      // Update or delete
      new AlertDialog.Builder(mContext)
      .setTitle(name)
      .setPositiveButton(mContext.getString(R.string.update_mb_or_kb, getSizeString(remoteSize)),
                         new DialogInterface.OnClickListener()
      {
        @Override
        public void onClick(DialogInterface dlg, int which)
        {
          if (!hasFreeSpace(remoteSize + MB))
            showNotEnoughFreeSpaceDialog(getSizeString(remoteSize), name);
          else
          {
            mStorage.downloadCountry(idx);
            Statistics.INSTANCE.trackCountryUpdate(mContext);
          }

          dlg.dismiss();
        }
      })
      .setNeutralButton(R.string.delete, new DialogInterface.OnClickListener()
      {
        @Override
        public void onClick(DialogInterface dlg, int which)
        {
          mStorage.deleteCountry(idx);
          Statistics.INSTANCE.trackCountryDeleted(mContext);
          dlg.dismiss();
        }
      })
      .setNegativeButton(R.string.cancel, m_alertCancelHandler)
      .create()
      .show();
      break;

    case MapStorage.NOT_DOWNLOADED:
      // Check for available free space
      final long size = mStorage.countryRemoteSizeInBytes(idx);
      if (!hasFreeSpace(size + MB))
      {
        showNotEnoughFreeSpaceDialog(getSizeString(size), name);
      }
      else
      {
        // Confirm downloading
        mAlert
        .setTitle(name)
        .setPositiveButton(mContext.getString(R.string.download_mb_or_kb, getSizeString(size)),
                           new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            mStorage.downloadCountry(idx);
            Statistics.INSTANCE.trackCountryDownload(mContext);
            dlg.dismiss();
          }
        })
        .setNegativeButton(R.string.cancel, m_alertCancelHandler)
        .create()
        .show();
      }
      break;

    case MapStorage.DOWNLOAD_FAILED:
      // Do not confirm downloading if status is failed, just start it
      mStorage.downloadCountry(idx);
      break;

    case MapStorage.DOWNLOADING:
      // Confirm canceling
      mAlert
      .setTitle(name)
      .setPositiveButton(R.string.cancel_download, new DialogInterface.OnClickListener()
      {
        @Override
        public void onClick(DialogInterface dlg, int which)
        {
          mStorage.deleteCountry(idx);
          dlg.dismiss();
        }
      })
      .setNegativeButton(R.string.do_nothing, m_alertCancelHandler)
      .create()
      .show();
      break;

    case MapStorage.IN_QUEUE:
      // Silently discard country from the queue
      mStorage.deleteCountry(idx);
      break;
    }

    // Actual status will be updated in "updateStatus" callback.
  }

  private void updateStatuses()
  {
    for (int i = 0; i < mItems.length; ++i)
    {
      final Index idx = mIdx.getChild(i);

      assert(idx.isValid());
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
    public TextView      mName    = null;
    public ImageView     mFlag    = null;
    public ImageView     mGuide   = null;
    public ProgressBar mProgress  = null;
    public View      mCountryMenu = null;

    void initFromView(View v)
    {
      mName = (TextView) v.findViewById(R.id.title);
      mFlag = (ImageView) v.findViewById(R.id.country_flag);
      mGuide = (ImageView) v.findViewById(R.id.guide_available);
      mCountryMenu = v.findViewById(R.id.country_menu);
      mProgress = (ProgressBar) v.findViewById(R.id.download_progress);
    }
  }

  /// Process "Map" button click in list view.
  private class MapClickListener implements OnClickListener
  {
    private final int m_position;

    public MapClickListener(int position) { m_position = position; }

    @Override
    public void onClick(View v)
    {
      mStorage.showCountry(mIdx.getChild(m_position));

      // close parent activity
      mContext.finish();
    }
  }

  private String formatStringWithSize(int strID, int position)
  {
    return mContext.getString(strID, getSizeString(mStorage.countryLocalSizeInBytes(mIdx.getChild(position))));
  }

  private String getSummary(int position)
  {
    int res = 0;

    switch (mItems[position].mStatus)
    {
    case MapStorage.ON_DISK:
      return formatStringWithSize(R.string.downloaded_touch_to_delete, position);

    case MapStorage.ON_DISK_OUT_OF_DATE:
      return formatStringWithSize(R.string.downloaded_touch_to_update, position);

    case MapStorage.NOT_DOWNLOADED: res = R.string.touch_to_download; break;
    case MapStorage.DOWNLOAD_FAILED: res = R.string.download_has_failed; break;
    case MapStorage.DOWNLOADING: res = R.string.downloading; break;
    case MapStorage.IN_QUEUE: res = R.string.marked_for_downloading; break;
    default:
      return "An unknown error occured!";
    }

    return mContext.getString(res);
  }

  private void setFlag(int position, ImageView v)
  {
    final Resources res = mContext.getResources();

    final String strID = mItems[position].mFlag;
    final int id = res.getIdentifier(strID, "drawable", mPackageName);
    if (id > 0)
      v.setImageDrawable(res.getDrawable(id));
    else
      Log.e(DownloadUI.TAG, "Failed to get resource id from: " + strID);
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    DownloadAdapter.ViewHolder holder = null;
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
    {
      holder = (DownloadAdapter.ViewHolder) convertView.getTag();
    }

    // for everything that has flag: regions + countries
    if (type != TYPE_GROUP)
    {
      setFlag(position, holder.mFlag);


      // this part if only for downloadable items
      if (type != TYPE_COUNTRY_GROUP)
      {
        populateForGuide(position, holder);

        if (type == TYPE_COUNTRY_IN_PROCESS)
        {
          holder.mProgress.setProgress(0);
          UiUtils.show(holder.mProgress);
        }
        else
          UiUtils.hide(holder.mProgress);
      }
    }
    setItemText(position, holder);

    return convertView;
  }



  private void setItemText(int position, DownloadAdapter.ViewHolder holder)
  {
    // set texts
    holder.mName.setText(mItems[position].mName);
    holder.mName.setTextColor(mItems[position].getTextColor());
  }



  private void populateForGuide(int position, DownloadAdapter.ViewHolder holder)
  {
    if (mHasGoogleStore)
    {
      final CountryItem item = getItem(position);
      final GuideInfo gi = Framework.getGuideInfoForIndex(item.mCountryIdx);
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

      return mItems[position].mStatus;
    }

    return MapStorage.UNKNOWN;
  }

  public void onCountryProgress(ListView list, Index idx, long current, long total)
  {
    final int position = getItemPosition(idx);
    if (position != -1)
    {
      assert(mItems[position].mStatus == 3);

      // do update only one item's view; don't call notifyDataSetChanged
      final View v = list.getChildAt(position - list.getFirstVisiblePosition());
      if (v != null)
      {
        final DownloadAdapter.ViewHolder holder = (DownloadAdapter.ViewHolder) v.getTag();
        holder.mProgress.setProgress((int) (current*100/total));
      }
    }
  }

  private String getSizeString(long size)
  {
    if (size > MB)
    {
      // do the correct rounding of MB
      return (size + 512 * 1024) / MB + " " + m_mb;
    }
    else
    {
      // get upper bound size for Kb
      return (size + 1023) / 1024 + " " + m_kb;
    }
  }
  private final static long MB = 1024 * 1024;
}