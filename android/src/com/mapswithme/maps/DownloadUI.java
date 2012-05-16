package com.mapswithme.maps;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.Context;
import android.content.DialogInterface;
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


public class DownloadUI extends ListActivity
{
	private static String TAG = "DownloadUI";

	/// @name JNI functions set for countries manipulation.
	//@{
  private static native int countriesCount(int group, int country, int region);
  private static native int countryStatus(int group, int country, int region);
  private static native long countryLocalSizeInBytes(int group, int country, int region);
  private static native long countryRemoteSizeInBytes(int group, int country, int region);
  private static native String countryName(int group, int country, int region);
  private static native void showCountry(int group, int country, int region);

  private native void nativeCreate();
  private native void nativeDestroy();

  private static native void downloadCountry(int group, int country, int region);
  private static native void deleteCountry(int group, int country, int region);
  //@}

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

		private static class CountryItem
		{
			public String m_name;

			// -2 = Group item
			// -1 = Group item for country
			// 0 = EOnDisk
	// 1 = ENotDownloaded
	// 2 = EDownloadFailed
	// 3 = EDownloading
	// 4 = EInQueue
	// 5 = EUnknown
			// 6 = EGeneratingIndex
			public int m_status;

			public CountryItem(int group, int country, int region)
			{
				m_name = countryName(group, country, region);
				updateStatus(group, country, region);
			}

			public void updateStatus(int group, int country, int region)
			{
				if (country == -1)
					m_status = -2;
				else if (region == -1 && countriesCount(group, country, -1) > 0)
					m_status = -1;
				else
					m_status = countryStatus(group, country, region);
			}

			public int getTextColor()
			{
				switch (m_status)
				{
				case 0: return 0xFF00A144;
				case 1: return 0xFFFFFFFF;
				case 2: return 0xFFFF0000;
				case 3: return 0xFF342BB6;
				case 4: return 0xFF5B94DE;
				default: return 0xFFFFFFFF;
				}
			}

			/// Get item type for list view representation;
			public int getType()
			{
				switch (m_status)
				{
				case -2: return TYPE_GROUP;
				case -1: return TYPE_COUNTRY_GROUP;
				case 0: return TYPE_COUNTRY_READY;
				default : return TYPE_COUNTRY_IN_PROCESS;
				}
			}
		}

		private int m_group = -1;
		private int m_country = -1;
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

	  static
	  {
		// TODO: Delete this if possible.
	    // Used to avoid crash when ndk library is not loaded.
	    // We just "touch" MWMActivity which should load it automatically
	    MWMActivity.getCurrentContext();
	  }

		public DownloadAdapter(Activity context)
		{
			m_context = context;
			m_inflater = (LayoutInflater) m_context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

			m_kb = context.getString(R.string.kb);
			m_mb = context.getString(R.string.mb);

	    m_alert = new AlertDialog.Builder(m_context);
	    m_alert.setCancelable(true);

			fillList();
		}

		private String getSizeString(long size)
		{
	    if (size > 1024 * 1024)
	      return size / (1024 * 1024) + " " + m_mb;
	    else if ((size + 1023) / 1024 > 999)
	      return "1 " + m_mb;
	    else
	      return (size + 1023) / 1024 + " " + m_kb;
		}

		private void fillList()
		{
			final int count = countriesCount(m_group, m_country, -1);
			if (count > 0)
			{
				m_items = new CountryItem[count];
				for (int i = 0; i < count; ++i)
				{
					int g = m_group;
					int c = m_country;
					int r = -1;

					if (g == -1) g = i;
					else if (c == -1) c = i;
					else r = i;

					m_items[i] = new CountryItem(g, c, r);
				}
			}

			notifyDataSetChanged();
		}

		/// Process list item click.
		private class ItemClickListener implements OnClickListener
		{
			private int m_position;

			public ItemClickListener(int position) { m_position = position; }

			@Override
      public void onClick(View v)
			{
				if (m_items[m_position].m_status < 0)
				{
					// expand next level
					if (m_group == -1)
						m_group = m_position;
					else
					{
						assert(m_country == -1);
						m_country = m_position;
					}

					fillList();
				}
				else
					processCountry(m_position);
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

	  private long getFreeSpace()
	  {
	    StatFs stat = new StatFs(Environment.getExternalStorageDirectory().getPath());
	    return (long)stat.getAvailableBlocks() * (long)stat.getBlockSize();
	  }

		private void processCountry(int position)
		{
	assert(m_group != -1);
	final int c = (m_country == -1 ? position : m_country);
	final int r = (m_country == -1 ? -1 : position);

	final String name = m_items[position].m_name;

	// Get actual status here
	switch (countryStatus(m_group, c, r))
      {
      case 0: // EOnDisk
	// Confirm deleting
        m_alert
		.setTitle(name)
		.setPositiveButton(R.string.delete,
				new DialogInterface.OnClickListener()
				{
					@Override
					public void onClick(DialogInterface dlg, int which)
					{
						deleteCountry(m_group, c, r);
						dlg.dismiss();
					}
				})
		.setNegativeButton(android.R.string.cancel, m_alertCancelHandler)
		.create()
		.show();
        break;

      case 1: // ENotDownloaded
        // Check for available free space
        final long size = countryRemoteSizeInBytes(m_group, c, r);
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
						downloadCountry(m_group, c, r);
						dlg.dismiss();
					}
			          })
			      .setNegativeButton(android.R.string.cancel, m_alertCancelHandler)
			      .create()
			      .show();
        }
        break;

      case 2: // EDownloadFailed
        // Do not confirm downloading if status is failed, just start it
        downloadCountry(m_group, c, r);
        break;

      case 3: // EDownloading
        // Confirm canceling
        m_alert
		.setTitle(name)
		.setPositiveButton(R.string.cancel_download,
				new DialogInterface.OnClickListener()
			        {
			          @Override
			          public void onClick(DialogInterface dlg, int which)
			          {
			            deleteCountry(m_group, c, r);
			            dlg.dismiss();
			          }
			        })
			    .setNegativeButton(R.string.do_nothing, m_alertCancelHandler)
		.create()
		.show();
        break;

      case 4: // EInQueue
        // Silently discard country from the queue
        deleteCountry(m_group, c, r);
        break;
      }

	// Actual status will be updated in "updateStatus" callback.
		}

		/// @return true If "back" was processed.
		public boolean onBackPressed()
		{
			// go to the parent level
			if (m_country != -1)
				m_country = -1;
			else if (m_group != -1)
				m_group = -1;
			else
				return false;

			fillList();
			return true;
		}

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
      public Button m_map = null;

      void initFromView(View v)
      {
			m_name = (TextView) v.findViewById(R.id.title);
			m_summary = (TextView) v.findViewById(R.id.summary);
			m_flag = (ImageView) v.findViewById(R.id.country_flag);
			m_map = (Button) v.findViewById(R.id.show_country);
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
		assert(m_group != -1);
		int c = m_country;
		int r = -1;
		if (c == -1) c = m_position;
		else r = m_position;

		showCountry(m_group, c, r);

		// close parent activity
		m_context.finish();
	}
    }

		private String getSummary(int position)
		{
			int res = 0;

			switch (m_items[position].m_status)
			{
			case 0:
			assert(m_group != -1);
		int c = m_country;
		int r = -1;
		if (c == -1) c = position;
		else r = position;

				return String.format(m_context.getString(R.string.downloaded_touch_to_delete),
														 getSizeString(countryLocalSizeInBytes(m_group, c, r)));

			case 1: res = R.string.touch_to_download; break;
			case 2: res = R.string.download_has_failed; break;

			// print 1%; this value will be updated soon
			case 3: return String.format(m_context.getString(R.string.downloading_touch_to_cancel), 1);

			case 4: res = R.string.marked_for_downloading; break;
			default:
				return "An unknown error occured!";
			}

			return m_context.getString(res);
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
			holder.m_flag.setVisibility(ImageView.INVISIBLE);
			break;

		case TYPE_COUNTRY_GROUP:
			convertView = m_inflater.inflate(R.layout.download_item_group, null);
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

	holder.m_name.setText(m_items[position].m_name);
	holder.m_name.setTextColor(m_items[position].getTextColor());
	if (holder.m_summary != null)
		holder.m_summary.setText(getSummary(position));

	if (holder.m_map != null)
				holder.m_map.setOnClickListener(new MapClickListener(position));

		// Important: Process item click like this, because of:
		// http://stackoverflow.com/questions/1821871/android-how-to-fire-onlistitemclick-in-listactivity-with-buttons-in-list
		convertView.setOnClickListener(new ItemClickListener(position));

	return convertView;
    }

    /// Get list item position by index(g, c, r).
    /// @return -1 If no such item in display list.
    private int getItemPosition(int group, int country, int region)
    {
	if (group == m_group && (m_country == -1 || m_country == country))
	{
		final int position = (m_country == -1) ? country : region;
		if (position >= 0 && position < m_items.length)
			return position;
		else
			Log.e(TAG, "Incorrect item position for: " + group + ", " + country + ", " + region);
	}
	return -1;
    }

    /// @name Callbacks from native code.
    //@{
    public void updateStatus(int group, int country, int region)
    {
	final int position = getItemPosition(group, country, region);
	if (position != -1)
	{
		m_items[position].updateStatus(group, country, region);

	// use this hard reset, because of caching different ViewHolders according to item's type
		notifyDataSetChanged();
	}
    }

    public void updatePercent(ListView list,
													int group, int country, int region,
													long current, long total)
    {
	final int position = getItemPosition(group, country, region);
	if (position != -1)
	{
		assert(m_items[position].m_status == 3);

		View v = list.getChildAt(position);
		ViewHolder holder = (ViewHolder) v.getTag();
		holder.m_summary.setText(String.format(m_context.getString(R.string.downloading_touch_to_cancel),
																																		 current * 100 / total));
		v.invalidate();
	}
    }
    //@}
	}

	@Override
  protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		setContentView(R.layout.downloader_list_view);

		nativeCreate();

		setListAdapter(new DownloadAdapter(this));
	}

	@Override
  public void onDestroy()
  {
    super.onDestroy();

    nativeDestroy();
  }

	private DownloadAdapter getDA()
	{
		return (DownloadAdapter) getListView().getAdapter();
	}

	@Override
  public void onBackPressed()
	{
		if (!getDA().onBackPressed())
			super.onBackPressed();
	}

	/// @name Callbacks from native code.
	//@{
  public void onChangeCountry(int group, int country, int region)
  {
	getDA().updateStatus(group, country, region);
  }

  public void onProgress(int group, int country, int region, long current, long total)
  {
	// try to update only specified item's text
	getDA().updatePercent(getListView(), group, country, region, current, total);
  }
  //@}
}
