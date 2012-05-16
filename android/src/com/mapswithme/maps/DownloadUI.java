package com.mapswithme.maps;

import com.mapswithme.util.ConnectionState;
import com.mapswithme.maps.R;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.os.StatFs;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import android.util.Log;

public class DownloadUI extends PreferenceActivity implements MapStorage.Listener
{
  private static String TAG = "DownloadUI";

  private int mSlotId = 0;
  
  private AlertDialog.Builder m_alert;
  private DialogInterface.OnClickListener m_alertCancelHandler = new DialogInterface.OnClickListener() {
    public void onClick(DialogInterface dlg, int which) { dlg.dismiss(); } };

  // Cached resources strings
  private String m_kb;
  private String m_mb;

  private MapStorage mMapStorage;
  
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    mMapStorage = MapStorage.getInstance();
    
    super.onCreate(savedInstanceState);

    m_kb = getString(R.string.kb);
    m_mb = getString(R.string.mb);

    PreferenceScreen root = getPreferenceManager().createPreferenceScreen(this);
    setPreferenceScreen(createCountriesHierarchy(root, new MapStorage.Index()));

    m_alert = new AlertDialog.Builder(this);
    m_alert.setCancelable(true);
  }
  
  @Override
  protected void onResume()
  {
    super.onResume();
    
    if (mSlotId == 0)
      mSlotId = mMapStorage.subscribe(this);
  }
  
  protected void onPause()
  {
    super.onPause();
    
    if (mSlotId != 0)
    {
      mMapStorage.unsubscribe(mSlotId);
      mSlotId = 0;
    }
  }
  
  private String formatSizeString(long sizeInBytes)
  {
    if (sizeInBytes > 1024 * 1024)
      return sizeInBytes / (1024 * 1024) + " " + m_mb;
    else if ((sizeInBytes + 1023) / 1024 > 999)
      return "1 " + m_mb;
    else
      return (sizeInBytes + 1023) / 1024 + " " + m_kb;
  }

  private void updateCountryCell(final Preference cell, MapStorage.Index idx)
  {
    final int status = mMapStorage.countryStatus(idx);

    switch (status)
    {
    case MapStorage.ON_DISK:
      cell.setSummary(getString(R.string.downloaded_touch_to_delete,
          formatSizeString(mMapStorage.countryLocalSizeInBytes(idx))));
      cell.setLayoutResource(R.layout.country_on_disk);
      break;
    case MapStorage.NOT_DOWNLOADED:
      cell.setSummary(getString(R.string.touch_to_download));
      cell.setLayoutResource(R.layout.country_not_downloaded);
      break;
    case MapStorage.DOWNLOAD_FAILED:
      cell.setSummary(getString(R.string.download_has_failed));
      cell.setLayoutResource(R.layout.country_download_failed);
      break;
    case MapStorage.DOWNLOADING:
      cell.setSummary(getString(R.string.downloading));
      cell.setLayoutResource(R.layout.country_downloading);
      break;
    case MapStorage.IN_QUEUE:
      cell.setSummary(getString(R.string.marked_for_downloading));
      cell.setLayoutResource(R.layout.country_in_the_queue);
      break;
    case MapStorage.UNKNOWN:
      cell.setSummary("Unknown state :(");
      break;
    case MapStorage.GENERATING_INDEX:
      cell.setSummary("Indexing for search...");
      cell.setLayoutResource(R.layout.country_generating_search_index);
    }
  }

  public void onCountryStatusChanged(MapStorage.Index idx)
  {
    final Preference cell = findPreference(idx.mGroup + " " + idx.mCountry + " " + idx.mRegion);
    if (cell == null)
    {
      Log.d(TAG, String.format("no preference found for %d %d %d", idx.mGroup, idx.mCountry, idx.mRegion));
      return;
    }
    updateCountryCell(cell, idx);
  }

  public void onCountryProgress(MapStorage.Index idx, long current, long total)
  {
    final Preference c = findPreference(idx.mGroup + " " + idx.mCountry + " " + idx.mRegion);

    if (c == null)
      Log.d(TAG, String.format("no preference found for %d %d %d", idx.mGroup, idx.mCountry, idx.mRegion));
    else
      c.setSummary(getString(R.string.downloading_touch_to_cancel, current * 100 / total));
  }

  private Preference createElement(MapStorage.Index idx)
  {
    final String name = mMapStorage.countryName(idx);
    if (mMapStorage.countriesCount(idx) == 0)
    { // it's downloadable country element
      final Preference cell = new Preference(this);
      cell.setKey(idx.mGroup + " " + idx.mCountry + " " + idx.mRegion);
      cell.setTitle(name);

      updateCountryCell(cell, idx);

      return cell;
    }
    else
    { // it's parent element for downloadable countries
      PreferenceScreen parent = getPreferenceManager().createPreferenceScreen(this);
      parent.setTitle(name);
      return createCountriesHierarchy(parent, idx);
    }
  }

  private PreferenceScreen createCountriesHierarchy(PreferenceScreen root, MapStorage.Index idx)
  {
    // Add "header" with "Map" and "Connection Settings" buttons
    final Preference cell = new Preference(this);
    if (!ConnectionState.isConnected(this))
      cell.setLayoutResource(R.layout.downloader_header_map_connection);
    else
      cell.setLayoutResource(R.layout.downloader_header_map);
    root.addPreference(cell);

    final int count = mMapStorage.countriesCount(idx);
    for (int i = 0; i < count; ++i)
    {
      if (idx.mGroup == -1)
        root.addPreference(createElement(new MapStorage.Index(i, idx.mCountry, idx.mRegion)));
      else if (idx.mCountry == -1)
        root.addPreference(createElement(new MapStorage.Index(idx.mGroup, i, idx.mRegion)));
      else if (idx.mRegion == -1)
        root.addPreference(createElement(new MapStorage.Index(idx.mGroup, idx.mCountry, i)));
    }
    return root;
  }

  private void showNotEnoughFreeSpaceDialog(String spaceNeeded, String countryName)
  {
    new AlertDialog.Builder(this).setMessage(String.format(getString(R.string.free_space_for_country), spaceNeeded, countryName))
        .setNegativeButton(getString(R.string.close), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dlg, int which) {
            dlg.dismiss();
          }
        })
        .create().show();
  }

  private long getFreeSpace()
  {
    StatFs stat = new StatFs(Environment.getExternalStorageDirectory().getPath());
    return (long)stat.getAvailableBlocks() * (long)stat.getBlockSize();
  }

  @Override
  public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference)
  {
    if (preference.hasKey())
    {
      final String[] keys = preference.getKey().split(" ");
      final int group = Integer.parseInt(keys[0]);
      final int country = Integer.parseInt(keys[1]);
      final int region = Integer.parseInt(keys[2]);

      switch (mMapStorage.countryStatus(new MapStorage.Index(group, country, region)))
      {
      case MapStorage.ON_DISK:
        m_alert.setTitle(mMapStorage.countryName(new MapStorage.Index(group, country, region)));
        m_alert.setPositiveButton(R.string.delete, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dlg, int which) {
            mMapStorage.deleteCountry(new MapStorage.Index(group, country, region));
            dlg.dismiss();
          }
        });
        m_alert.setNegativeButton(android.R.string.cancel, m_alertCancelHandler);
        m_alert.create().show();
        break;

      case MapStorage.NOT_DOWNLOADED:
        // Check for available free space
        final long size = mMapStorage.countryRemoteSizeInBytes(new MapStorage.Index(group, country, region));
        final String name = mMapStorage.countryName(new MapStorage.Index(group, country, region));
        if (size > getFreeSpace())
        {
          showNotEnoughFreeSpaceDialog(formatSizeString(size), name);
        }
        else
        {
          // Display download confirmation
          m_alert.setTitle(name);
          m_alert.setPositiveButton(getString(R.string.download_mb_or_kb, formatSizeString(size)),
              new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dlg, int which) {
                  mMapStorage.downloadCountry(new MapStorage.Index(group, country, region));
                  dlg.dismiss();
                }
              });
          m_alert.setNegativeButton(android.R.string.cancel, m_alertCancelHandler);
          m_alert.create().show();
        }
        break;

      case MapStorage.DOWNLOAD_FAILED:
        // Do not confirm download if status is failed, just start it
        mMapStorage.downloadCountry(new MapStorage.Index(group, country, region));
        break;

      case MapStorage.DOWNLOADING:
        /// Confirm canceling
        m_alert.setTitle(mMapStorage.countryName(new MapStorage.Index(group, country, region)));
        m_alert.setPositiveButton(R.string.cancel_download, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dlg, int which) {
            mMapStorage.deleteCountry(new MapStorage.Index(group, country, region));
            dlg.dismiss();
          }
        });
        m_alert.setNegativeButton(R.string.do_nothing, m_alertCancelHandler);
        m_alert.create().show();
        break;

      case MapStorage.IN_QUEUE:
        // Silently discard country from the queue
        mMapStorage.deleteCountry(new MapStorage.Index(group, country, region));
        break;

      case MapStorage.UNKNOWN:
        Log.d(TAG, "Unknown country state");
        break;
      }
      return true;
    }
    return super.onPreferenceTreeClick(preferenceScreen, preference);
  }

  public void onShowNetSettingsClicked(android.view.View v)
  {
    startActivity(new Intent(Settings.ACTION_WIRELESS_SETTINGS));
  }

  public void onShowMapClicked(android.view.View v)
  {
    Intent mwmActivityIntent = new Intent(this, MWMActivity.class);
    // Disable animation because MWMActivity should appear exactly over this one
    mwmActivityIntent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
    startActivity(mwmActivityIntent);
  }
}
