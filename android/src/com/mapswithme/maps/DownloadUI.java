package com.mapswithme.maps;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.Environment;
import android.os.StatFs;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import com.mapswithme.maps.MapStorage.Index;


public class DownloadUI extends ListActivity implements MapStorage.Listener
{
  private static String TAG = "DownloadUI";

  /// ListView adapter
  private static class DownloadAdapter extends BaseAdapter
  {
    /// @name Different row types.
    //@{
    private static final int TYPE_GROUP = 0;
    private static final int TYPE_COUNTRY_GROUP = 1;
    private static final int TYPE_COUNTRY_IN_PROCESS = 2;
    private static final int TYPE_COUNTRY_READY = 3;
    private static final int TYPES_COUNT = 4;
    //@}

    private LayoutInflater m_inflater;
    private Activity m_context;

    private int m_slotID = 0;
    private MapStorage m_storage;

    private String m_packageName;

    private static class CountryItem
    {
      public String m_name;
      public String m_flag;

      /// @see constants in MapStorage
      public int m_status;

      public CountryItem(MapStorage storage, Index idx)
      {
        m_name = storage.countryName(idx);

        m_flag = storage.countryFlag(idx);
        // The aapt can't process resources with name "do". Hack with renaming.
        if (m_flag.equals("do"))
          m_flag = "do_hack";

        updateStatus(storage, idx);
      }

      public void updateStatus(MapStorage storage, Index idx)
      {
        if (idx.mCountry == -1 || (idx.mRegion == -1 && m_flag.length() == 0))
        {
          // group and not a country
          m_status = MapStorage.GROUP;
        }
        else if (idx.mRegion == -1 && storage.countriesCount(idx) > 0)
        {
          // country with grouping
          m_status = MapStorage.COUNTRY;
        }
        else
        {
          // country or region without grouping
          m_status = storage.countryStatus(idx);
        }
      }

      public int getTextColor()
      {
        switch (m_status)
        {
        case MapStorage.ON_DISK: return 0xFF00A144;
        case MapStorage.ON_DISK_OUT_OF_DATE: return 0xFFFF69B4;
        case MapStorage.NOT_DOWNLOADED: return 0xFFFFFFFF;
        case MapStorage.DOWNLOAD_FAILED: return 0xFFFF0000;
        case MapStorage.DOWNLOADING: return 0xFF342BB6;
        case MapStorage.IN_QUEUE: return 0xFF5B94DE;
        default: return 0xFFFFFFFF;
        }
      }

      /// Get item type for list view representation;
      public int getType()
      {
        switch (m_status)
        {
        case MapStorage.GROUP: return TYPE_GROUP;
        case MapStorage.COUNTRY: return TYPE_COUNTRY_GROUP;
        case MapStorage.ON_DISK:
        case MapStorage.ON_DISK_OUT_OF_DATE:
          return TYPE_COUNTRY_READY;
        default : return TYPE_COUNTRY_IN_PROCESS;
        }
      }
    }

    private Index m_idx = new Index();
    private CountryItem[] m_items = null;

    private String m_kb;
    private String m_mb;

    private AlertDialog.Builder m_alert;
    private DialogInterface.OnClickListener m_alertCancelHandler =
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
      m_storage = MapStorage.getInstance();
      m_packageName = context.getApplication().getPackageName();

      m_context = context;
      m_inflater = (LayoutInflater) m_context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

      m_kb = context.getString(R.string.kb);
      m_mb = context.getString(R.string.mb);

      m_alert = new AlertDialog.Builder(m_context);

      fillList();
    }

    private String getSizeString(long size)
    {
      final long Mb = 1024 * 1024;
      if (size > Mb)
      {
        // do the correct rounding of Mb
        return (size + 512 * 1024) / Mb + " " + m_mb;
      }
      else
      {
        // get upper bound size for Kb
        return (size + 1023) / 1024 + " " + m_kb;
      }
    }

    /// Fill list for current m_group and m_country.
    private void fillList()
    {
      final int count = m_storage.countriesCount(m_idx);
      if (count > 0)
      {
        m_items = new CountryItem[count];
        for (int i = 0; i < count; ++i)
          m_items[i] = new CountryItem(m_storage, m_idx.getChild(i));
      }

      notifyDataSetChanged();
    }

    /// Process list item click.
    public boolean onItemClick(int position)
    {
      if (m_items[position].m_status < 0)
      {
        // expand next level
        m_idx = m_idx.getChild(position);

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
      new AlertDialog.Builder(m_context)
      .setMessage(String.format(m_context.getString(R.string.free_space_for_country), spaceNeeded, countryName))
      .setNegativeButton(m_context.getString(R.string.close), m_alertCancelHandler)
      .create()
      .show();
    }

    static private long getFreeSpace()
    {
      StatFs stat = new StatFs(Environment.getExternalStorageDirectory().getPath());
      return (long)stat.getAvailableBlocks() * (long)stat.getBlockSize();
    }

    private void processCountry(int position)
    {
      final Index idx = m_idx.getChild(position);

      final String name = m_items[position].m_name;

      // Get actual status here
      switch (m_storage.countryStatus(idx))
      {
      case MapStorage.ON_DISK:
        // Confirm deleting
        m_alert
        .setTitle(name)
        .setPositiveButton(R.string.delete, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            m_storage.deleteCountry(idx);
            dlg.dismiss();
          }
        })
        .setNegativeButton(android.R.string.cancel, m_alertCancelHandler)
        .create()
        .show();
        break;

      case MapStorage.ON_DISK_OUT_OF_DATE:
        final long remoteSize = m_storage.countryRemoteSizeInBytes(idx);

        // Update or delete
        new AlertDialog.Builder(m_context)
        .setTitle(name)
        .setPositiveButton(m_context.getString(R.string.update_mb_or_kb, getSizeString(remoteSize)),
                           new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            final long size = remoteSize - m_storage.countryLocalSizeInBytes(idx);
            if (size > getFreeSpace())
              showNotEnoughFreeSpaceDialog(getSizeString(size), name);
            else
            {
              m_storage.deleteCountry(idx);
              m_storage.downloadCountry(idx);
            }

            dlg.dismiss();
          }
        })
        .setNeutralButton(R.string.delete, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            m_storage.deleteCountry(idx);
            dlg.dismiss();
          }
        })
        .setNegativeButton(android.R.string.cancel, m_alertCancelHandler)
        .create()
        .show();
        break;

      case MapStorage.NOT_DOWNLOADED:
        // Check for available free space
        final long size = m_storage.countryRemoteSizeInBytes(idx);
        if (size > getFreeSpace())
        {
          showNotEnoughFreeSpaceDialog(getSizeString(size), name);
        }
        else
        {
          // Confirm downloading
          m_alert
          .setTitle(name)
          .setPositiveButton(m_context.getString(R.string.download_mb_or_kb, getSizeString(size)),
                             new DialogInterface.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dlg, int which)
            {
              m_storage.downloadCountry(idx);
              dlg.dismiss();
            }
          })
          .setNegativeButton(android.R.string.cancel, m_alertCancelHandler)
          .create()
          .show();
        }
        break;

      case MapStorage.DOWNLOAD_FAILED:
        // Do not confirm downloading if status is failed, just start it
        m_storage.downloadCountry(idx);
        break;

      case MapStorage.DOWNLOADING:
        // Confirm canceling
        m_alert
        .setTitle(name)
        .setPositiveButton(R.string.cancel_download, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            m_storage.deleteCountry(idx);
            dlg.dismiss();
          }
        })
        .setNegativeButton(R.string.do_nothing, m_alertCancelHandler)
        .create()
        .show();
        break;

      case MapStorage.IN_QUEUE:
        // Silently discard country from the queue
        m_storage.deleteCountry(idx);
        break;
      }

      // Actual status will be updated in "updateStatus" callback.
    }

    private void updateStatuses()
    {
      for (int i = 0; i < m_items.length; ++i)
      {
        final Index idx = m_idx.getChild(i);

        assert(idx.isValid());
        if (idx.isValid())
          m_items[i].updateStatus(m_storage, idx);
      }
    }

    /// @name Process routine from parent Activity.
    //@{
    /// @return true If "back" was processed.
    public boolean onBackPressed()
    {
      // we are on the root level already - return
      if (m_idx.isRoot())
        return false;

      // go to the parent level
      m_idx = m_idx.getParent();

      fillList();
      return true;
    }

    public void onResume(MapStorage.Listener listener)
    {
      if (m_slotID == 0)
        m_slotID = m_storage.subscribe(listener);

      // update actual statuses for items after resuming activity
      updateStatuses();
      notifyDataSetChanged();
    }

    public void onPause()
    {
      if (m_slotID != 0)
      {
        m_storage.unsubscribe(m_slotID);
        m_slotID = 0;
      }
    }
    //@}

    @Override
    public int getItemViewType(int position)
    {
      return m_items[position].getType();
    }

    @Override
    public int getViewTypeCount()
    {
      return TYPES_COUNT;
    }

    @Override
    public int getCount()
    {
      return (m_items != null ? m_items.length : 0);
    }

    @Override
    public CountryItem getItem(int position)
    {
      return m_items[position];
    }

    @Override
    public long getItemId(int position)
    {
      return position;
    }

    private static class ViewHolder
    {
      public TextView m_name = null;
      public TextView m_summary = null;
      public ImageView m_flag = null;
      public ImageView m_map = null;

      void initFromView(View v)
      {
        m_name = (TextView) v.findViewById(R.id.title);
        m_summary = (TextView) v.findViewById(R.id.summary);
        m_flag = (ImageView) v.findViewById(R.id.country_flag);
        m_map = (ImageView) v.findViewById(R.id.show_country);
      }
    }

    /// Process "Map" button click in list view.
    private class MapClickListener implements OnClickListener
    {
      private int m_position;

      public MapClickListener(int position) { m_position = position; }

      @Override
      public void onClick(View v)
      {
        m_storage.showCountry(m_idx.getChild(m_position));

        // close parent activity
        m_context.finish();
      }
    }

    private String formatStringWithSize(int strID, int position)
    {
      return m_context.getString(strID, getSizeString(m_storage.countryLocalSizeInBytes(m_idx.getChild(position))));
    }

    private String getSummary(int position)
    {
      int res = 0;

      switch (m_items[position].m_status)
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

      return m_context.getString(res);
    }

    private void setFlag(int position, ImageView v)
    {
      Resources res = m_context.getResources();
      final int id = res.getIdentifier(m_items[position].m_flag, "drawable", m_packageName);
      if (id > 0)
        v.setImageDrawable(res.getDrawable(id));
      else
        Log.e(TAG, "Failed to get resource id from: " + m_items[position].m_flag);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent)
    {
      ViewHolder holder = null;

      if (convertView == null)
      {
        holder = new ViewHolder();
        switch (getItemViewType(position))
        {
        case TYPE_GROUP:
          convertView = m_inflater.inflate(R.layout.download_item_group, null);
          holder.initFromView(convertView);
          break;

        case TYPE_COUNTRY_GROUP:
          convertView = m_inflater.inflate(R.layout.download_item_country_group, null);
          holder.initFromView(convertView);
          break;

        case TYPE_COUNTRY_IN_PROCESS:
          convertView = m_inflater.inflate(R.layout.download_item_country, null);
          holder.initFromView(convertView);
          holder.m_map.setVisibility(Button.INVISIBLE);
          break;

        case TYPE_COUNTRY_READY:
          convertView = m_inflater.inflate(R.layout.download_item_country, null);
          holder.initFromView(convertView);
          break;
        }

        convertView.setTag(holder);
      }
      else
      {
        holder = (ViewHolder) convertView.getTag();
      }

      // set texts
      holder.m_name.setText(m_items[position].m_name);
      holder.m_name.setTextColor(m_items[position].getTextColor());
      if (holder.m_summary != null)
        holder.m_summary.setText(getSummary(position));

      // attach to "Map" button if needed
      if (holder.m_map != null && holder.m_map.getVisibility() == Button.VISIBLE)
        holder.m_map.setOnClickListener(new MapClickListener(position));

      // show flag if needed
      if (holder.m_flag != null && holder.m_flag.getVisibility() == ImageView.VISIBLE)
        setFlag(position, holder.m_flag);

      return convertView;
    }

    /// Get list item position by index(g, c, r).
    /// @return -1 If no such item in display list.
    private int getItemPosition(Index idx)
    {
      if (m_idx.isChild(idx))
      {
        final int position = idx.getPosition();
        if (position >= 0 && position < m_items.length)
          return position;
        else
          Log.e(TAG, "Incorrect item position for: " + idx.toString());
      }
      return -1;
    }

    public void onCountryStatusChanged(Index idx)
    {
      Log.d(TAG, "onCountryStatusChanged for index: " + idx.toString());

      final int position = getItemPosition(idx);
      if (position != -1)
      {
        m_items[position].updateStatus(m_storage, idx);

        // use this hard reset, because of caching different ViewHolders according to item's type
        notifyDataSetChanged();
      }
    }

    public void onCountryProgress(ListView list, Index idx, long current, long total)
    {
      final int position = getItemPosition(idx);
      if (position != -1)
      {
        assert(m_items[position].m_status == 3);

        // do update only one item's view; don't call notifyDataSetChanged
        View v = list.getChildAt(position - list.getFirstVisiblePosition());
        if (v != null)
        {
          ViewHolder holder = (ViewHolder) v.getTag();
          holder.m_summary.setText(String.format(m_context.getString(R.string.downloading_touch_to_cancel),
                                                 current * 100 / total));
          v.invalidate();
        }
      }
    }
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.downloader_list_view);

    setListAdapter(new DownloadAdapter(this));
  }

  private DownloadAdapter getDA()
  {
    return (DownloadAdapter) getListView().getAdapter();
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    getDA().onResume(this);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    getDA().onPause();
  }

  @Override
  public void onBackPressed()
  {
    if (getDA().onBackPressed())
    {
      // scroll list view to the top
      setSelection(0);
    }
    else
      super.onBackPressed();
  }

  @Override
  protected void onListItemClick(ListView l, View v, int position, long id)
  {
    super.onListItemClick(l, v, position, id);

    if (getDA().onItemClick(position))
    {
      // scroll list view to the top
      setSelection(0);
    }
  }

  @Override
  public void onCountryStatusChanged(Index idx)
  {
    getDA().onCountryStatusChanged(idx);
  }

  @Override
  public void onCountryProgress(Index idx, long current, long total)
  {
    getDA().onCountryProgress(getListView(), idx, current, total);
  }
}
