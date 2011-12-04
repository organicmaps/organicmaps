package com.mapswithme.maps;

import android.os.Bundle;
import android.preference.CheckBoxPreference;
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
    
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    // Root
    PreferenceScreen root = getPreferenceManager().createPreferenceScreen(this);
    setPreferenceScreen(createCountriesHierarchy(root, -1, -1, -1));
    
    nativeCreate();
  }
  
  @Override
  public void onDestroy()
  {
    super.onDestroy();
    
    nativeDestroy();
  }
  
  public void onChangeCountryImpl(int group, int country, int region)
  {
    Log.d(TAG, String.format("onChangeCountry %1$d, %2$d, %3$d", group, country, region));
  }
    
  class ChangeCountryFn implements Runnable
  {
    int m_group;
    int m_country;
    int m_region; 
    Object m_ui;
    ChangeCountryFn(Object ui, int group, int country, int region)
    {
      m_ui = ui;
      m_group = group;
      m_country = country;
      m_region = region;
    }
    public void run()
    {
      DownloadUI dui = (DownloadUI)m_ui;
      dui.onChangeCountryImpl(m_group, m_country, m_region);
    }
  };

  public void onChangeCountry(int group, int country, int region)
  {
    runOnUiThread(new ChangeCountryFn(this, group, country, region));
    /// Should post a message onto gui thread as it could be called from the HttpThread
  }

  class ProgressFn implements Runnable
  {
    int m_group;
    int m_country;
    int m_region;
    long m_p1;
    long m_p2;
    Object m_ui;
    ProgressFn(Object ui, int group, int country, int region, long p1, long p2)
    {
      m_ui = ui;
      m_group = group;
      m_country = country;
      m_region = region;
      m_p1 = p1;
      m_p2 = p2;
    }
    public void run()
    {
      DownloadUI dui = (DownloadUI)m_ui;
      dui.onProgressImpl(m_group, m_country, m_region, m_p1, m_p2);
    }
  };
  
  public void onProgressImpl(int group, int country, int region, long p1, long p2)
  {
    Log.d(TAG, String.format("onProgress %1$d, %2$d, %3$d, %4$d, %5$d", group, country, region, p1, p2));
  }

  public void onProgress(int group, int country, int region, long p1, long p2)
  {
    runOnUiThread(new ProgressFn(this, group, country, region, p1, p2));

    /// Should post a message onto gui thread as it could be called from the HttpThread    
  }

  private Preference createElement(int group, int country, int region)
  {
    final String name = countryName(group, country, region);
    final int count = countriesCount(group, country, region);
    if (count == 0)
    { // it's downloadable country element
      CheckBoxPreference c = new CheckBoxPreference(this);
      c.setKey(group + " " + country + " " + region);
      c.setTitle(name);
      
      final long localBytes = countryLocalSizeInBytes(group, country, region);
      final long remoteBytes = countryRemoteSizeInBytes(group, country, region);
      
      final String sizeString;
      if (remoteBytes > 1024 * 1024)
        sizeString = remoteBytes / (1024 * 1024) + "Mb";
      else
        sizeString = remoteBytes / 1024 + "Kb";
      switch (countryStatus(group, country, region))
      {
      case 0: // EOnDisk
        c.setSummary("Click to delete " + sizeString);
        break;
      case 1: // ENotDownloaded
        c.setSummary("Click to download " + sizeString);
        break;
      case 2: // EDownloadFailed
        c.setSummary("Download has failed, click again to retry");
        break;
      case 3: // EDownloading
        c.setSummary("Downloading...");
        break;        
      case 4: // EInQueue
        c.setSummary("Marked to download");
        break;        
      case 5: // EUnknown
        c.setSummary("Unknown state :(");
        break;        
      }
      return c;
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
      
/*      switch (countryStatus(group, country, region))
      {
      case 0: //EOnDisk
      {
        /// Ask about deleting 
      }*/
      downloadCountry(group, country, region);
      
      Log.d(TAG, "started country download");
    }
    return super.onPreferenceTreeClick(preferenceScreen, preference);
  }
}
