package com.mapswithme.maps;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
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

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    // Root
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
      return sizeInBytes / (1024 * 1024) + " Mb";
    else if ((sizeInBytes + 1023) / 1024 > 999)
      return "1 Mb";
    else
      return (sizeInBytes + 1023) / 1024 + " Kb";
  }

  private void updateCountryCell(final Preference cell, int group, int country, int region)
  {
    final int status = countryStatus(group, country, region);

    switch (status)
    {
    case 0: // EOnDisk
      cell.setSummary("Downloaded (" + formatSizeString(countryLocalSizeInBytes(group, country, region))
          + "), touch to delete");
//      ((CheckBoxPreference)cell).setChecked(true);
      cell.setLayoutResource(R.layout.country_on_disk);
      break;
    case 1: // ENotDownloaded
      cell.setSummary("Touch to download");// + formatSizeString(countryRemoteSizeInBytes(group, country, region)));
//      ((CheckBoxPreference)cell).setChecked(false);
      cell.setLayoutResource(R.layout.country_not_downloaded);
      break;
    case 2: // EDownloadFailed
      cell.setSummary("Download has failed, touch again for one more try");
      cell.setLayoutResource(R.layout.country_download_failed);
//      ((CheckBoxPreference)cell).setChecked(false);
      break;
    case 3: // EDownloading
      cell.setSummary("Downloading...");
      cell.setLayoutResource(R.layout.country_downloading);
//      ((CheckBoxPreference)cell).setChecked(true);
      break;
    case 4: // EInQueue
      cell.setSummary("Marked for downloading, touch to cancel");
      cell.setLayoutResource(R.layout.country_in_the_queue);
//      ((CheckBoxPreference)cell).setChecked(true);
      break;
    case 5: // EUnknown
      cell.setSummary("Unknown state :(");
//      ((CheckBoxPreference)cell).setChecked(false);
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
      c.setSummary("Downloading " + current * 100 / total + "%, touch to cancel");
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
//      parent.setKey(group + " " + country + " " + region);
      parent.setTitle(name);
//      parent.setSummary("");
      return createCountriesHierarchy(parent, group, country, region);
    }
  }
  
  private PreferenceScreen createCountriesHierarchy(PreferenceScreen root, int group, int country, int region)
  {
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
        m_alert.setPositiveButton("Delete", new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dlg, int which) {
            deleteCountry(group, country, region);
            dlg.dismiss();
          }
        });
        m_alert.setNegativeButton("Cancel", m_alertCancelHandler);
        m_alert.create().show();
        break;
        
      case 1: // ENotDownloaded
        m_alert.setTitle(countryName(group, country, region));
        m_alert.setPositiveButton("Download " + formatSizeString(countryRemoteSizeInBytes(group, country, region)),
            new DialogInterface.OnClickListener() {
              public void onClick(DialogInterface dlg, int which) {
                downloadCountry(group, country, region);
                dlg.dismiss();
              }
            });
        m_alert.setNegativeButton("Cancel", m_alertCancelHandler);
        m_alert.create().show();
        break;
        
      case 2: // EDownloadFailed
        // Do not confirm download if status is failed, just start it
        downloadCountry(group, country, region);
        break;

      case 3: // EDownloading
        /// Confirm canceling
        m_alert.setTitle(countryName(group, country, region));
        m_alert.setPositiveButton("Cancel download", new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dlg, int which) {
            deleteCountry(group, country, region);
            dlg.dismiss();
          }
        });
        m_alert.setNegativeButton("Do nothing", m_alertCancelHandler);
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
}
