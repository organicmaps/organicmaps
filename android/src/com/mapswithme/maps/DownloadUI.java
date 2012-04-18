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

public class DownloadUI extends PreferenceActivity
{
  private static String TAG = "DownloadUI";

  private native int countriesCount(int group, int country, int region);
  private native int countryStatus(int group, int country, int region);
  private native long countryLocalSizeInBytes(int group, int country, int region);
  private native long countryRemoteSizeInBytes(int group, int country, int region);
  private native String countryName(int group, int country, int region);
  private native void nativeCreate();
  private native void nativeDestroy();

  private native void downloadCountry(int group, int country, int region);
  private native void deleteCountry(int group, int country, int region);

  private AlertDialog.Builder m_alert;
  private DialogInterface.OnClickListener m_alertCancelHandler = new DialogInterface.OnClickListener() {
    public void onClick(DialogInterface dlg, int which) { dlg.dismiss(); } };

  // Cached resources strings
  private String m_kb;
  private String m_mb;

  static
  {
    // Used to avoid crash when ndk library is not loaded.
    // We just "touch" MWMActivity which should load it automatically
    MWMActivity.getCurrentContext();
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    m_kb = getString(R.string.kb);
    m_mb = getString(R.string.mb);

    PreferenceScreen root = getPreferenceManager().createPreferenceScreen(this);
    setPreferenceScreen(createCountriesHierarchy(root, -1, -1, -1));

    m_alert = new AlertDialog.Builder(this);
    m_alert.setCancelable(true);

    nativeCreate();
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();

    nativeDestroy();
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

  private void updateCountryCell(final Preference cell, int group, int country, int region)
  {
    final int status = countryStatus(group, country, region);

    switch (status)
    {
    case 0: // EOnDisk
      cell.setSummary(getString(R.string.downloaded_touch_to_delete,
          formatSizeString(countryLocalSizeInBytes(group, country, region))));
      cell.setLayoutResource(R.layout.country_on_disk);
      break;
    case 1: // ENotDownloaded
      cell.setSummary(getString(R.string.touch_to_download));
      cell.setLayoutResource(R.layout.country_not_downloaded);
      break;
    case 2: // EDownloadFailed
      cell.setSummary(getString(R.string.download_has_failed));
      cell.setLayoutResource(R.layout.country_download_failed);
      break;
    case 3: // EDownloading
      cell.setSummary(getString(R.string.downloading));
      cell.setLayoutResource(R.layout.country_downloading);
      break;
    case 4: // EInQueue
      cell.setSummary(getString(R.string.marked_for_downloading));
      cell.setLayoutResource(R.layout.country_in_the_queue);
      break;
    case 5: // EUnknown
      cell.setSummary("Unknown state :(");
      break;
    case 6: // EGeneratingIndex
      cell.setSummary("Indexing for search...");
      cell.setLayoutResource(R.layout.country_generating_search_index);
    }
  }

  public void onChangeCountry(int group, int country, int region)
  {
    final Preference cell = findPreference(group + " " + country + " " + region);
    if (cell == null)
    {
      Log.d(TAG, String.format("no preference found for %d %d %d", group, country, region));
      return;
    }
    updateCountryCell(cell, group, country, region);
  }

  public void onProgress(int group, int country, int region, long current, long total)
  {
    final Preference c = findPreference(group + " " + country + " " + region);

    if (c == null)
      Log.d(TAG, String.format("no preference found for %d %d %d", group, country, region));
    else
      c.setSummary(getString(R.string.downloading_touch_to_cancel, current * 100 / total));
  }

  private Preference createElement(int group, int country, int region)
  {
    final String name = countryName(group, country, region);
    if (countriesCount(group, country, region) == 0)
    { // it's downloadable country element
      final Preference cell = new Preference(this);
      cell.setKey(group + " " + country + " " + region);
      cell.setTitle(name);

      updateCountryCell(cell, group, country, region);

      return cell;
    }
    else
    { // it's parent element for downloadable countries
      PreferenceScreen parent = getPreferenceManager().createPreferenceScreen(this);
      parent.setTitle(name);
      return createCountriesHierarchy(parent, group, country, region);
    }
  }

  private PreferenceScreen createCountriesHierarchy(PreferenceScreen root, int group, int country, int region)
  {
    // Add "header" with "Map" and "Connection Settings" buttons
    final Preference cell = new Preference(this);
    if (!ConnectionState.isConnected(this))
      cell.setLayoutResource(R.layout.downloader_header_map_connection);
    else
      cell.setLayoutResource(R.layout.downloader_header_map);
    root.addPreference(cell);

    final int count = countriesCount(group, country, region);
    for (int i = 0; i < count; ++i)
    {
      if (group == -1)
        root.addPreference(createElement(i, country, region));
      else if (country == -1)
        root.addPreference(createElement(group, i, region));
      else if (region == -1)
        root.addPreference(createElement(group, country, i));
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

      switch (countryStatus(group, country, region))
      {
      case 0: // EOnDisk
        m_alert.setTitle(countryName(group, country, region));
        m_alert.setPositiveButton(R.string.delete, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dlg, int which) {
            deleteCountry(group, country, region);
            dlg.dismiss();
          }
        });
        m_alert.setNegativeButton(android.R.string.cancel, m_alertCancelHandler);
        m_alert.create().show();
        break;

      case 1: // ENotDownloaded
        // Check for available free space
        final long size = countryRemoteSizeInBytes(group, country, region);
        final String name = countryName(group, country, region);
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
                  downloadCountry(group, country, region);
                  dlg.dismiss();
                }
              });
          m_alert.setNegativeButton(android.R.string.cancel, m_alertCancelHandler);
          m_alert.create().show();
        }
        break;

      case 2: // EDownloadFailed
        // Do not confirm download if status is failed, just start it
        downloadCountry(group, country, region);
        break;

      case 3: // EDownloading
        /// Confirm canceling
        m_alert.setTitle(countryName(group, country, region));
        m_alert.setPositiveButton(R.string.cancel_download, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dlg, int which) {
            deleteCountry(group, country, region);
            dlg.dismiss();
          }
        });
        m_alert.setNegativeButton(R.string.do_nothing, m_alertCancelHandler);
        m_alert.create().show();
        break;

      case 4: // EInQueue
        // Silently discard country from the queue
        deleteCountry(group, country, region);
        break;

      case 5: // EUnknown
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
    mwmActivityIntent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
    startActivity(mwmActivityIntent);
  }
}
