package com.mapswithme.country;

import android.graphics.Typeface;

import com.mapswithme.maps.MapStorage;

public class CountryItem
{
  public static final CountryItem EMPTY = new CountryItem("", MapStorage.NOT_DOWNLOADED, StorageOptions.MAP_OPTION_MAP_ONLY, false);

  private String mName;
  private int mOptions;
  private int mStatus;
  private boolean mHasChildren;

  private static final Typeface LIGHT = Typeface.create("sans-serif-light", Typeface.NORMAL);
  private static final Typeface REGULAR = Typeface.create("sans-serif", Typeface.NORMAL);

  public CountryItem(String name, int status, int options, boolean hasChildren)
  {
    mName = name;
    mStatus = status;
    mOptions = options;
    mHasChildren = hasChildren;
  }

  public int getStatus()
  {
    return mStatus;
  }

  public String getName()
  {
    return mName;
  }

  public int getOptions()
  {
    return mOptions;
  }

  public boolean hasChildren()
  {
    return mHasChildren;
  }

  public void setStatus(int mStatus)
  {
    this.mStatus = mStatus;
  }

  public void setName(String name)
  {
    mName = name;
  }

  public void setOptions(int options)
  {
    mOptions = options;
  }

  public Typeface getTypeface()
  {
    switch (mStatus)
    {
    case MapStorage.NOT_DOWNLOADED:
    case MapStorage.DOWNLOADING:
    case MapStorage.IN_QUEUE:
    case MapStorage.DOWNLOAD_FAILED:
      return LIGHT;
    default:
      return REGULAR;
    }
  }

  public int getType()
  {
    if (mHasChildren)
      return DownloadAdapter.TYPE_GROUP;

    switch (mStatus)
    {
    case MapStorage.GROUP:
      return DownloadAdapter.TYPE_GROUP;
    case MapStorage.COUNTRY:
      return DownloadAdapter.TYPE_COUNTRY_GROUP;
    case MapStorage.NOT_DOWNLOADED:
      return DownloadAdapter.TYPE_COUNTRY_NOT_DOWNLOADED;
    case MapStorage.ON_DISK:
    case MapStorage.ON_DISK_OUT_OF_DATE:
      return DownloadAdapter.TYPE_COUNTRY_READY;
    default:
      return DownloadAdapter.TYPE_COUNTRY_IN_PROCESS;
    }
  }

  @Override
  public String toString()
  {
    return "Name : " + mName + "; options : " + mOptions + "; status : " + mStatus + "; has children : " + mHasChildren;
  }
}
