package com.mapswithme.maps.downloader.country;

import android.graphics.Typeface;

@Deprecated
public class OldCountryItem
{
  public static final OldCountryItem EMPTY = new OldCountryItem("", OldMapStorage.NOT_DOWNLOADED, OldStorageOptions.MAP_OPTION_MAP_ONLY, false);

  private String mName;
  private int mOptions;
  private int mStatus;
  private boolean mHasChildren;

  private static final Typeface LIGHT = Typeface.create("sans-serif-light", Typeface.NORMAL);
  private static final Typeface REGULAR = Typeface.create("sans-serif", Typeface.NORMAL);

  public OldCountryItem(String name, int status, int options, boolean hasChildren)
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
    case OldMapStorage.NOT_DOWNLOADED:
    case OldMapStorage.DOWNLOADING:
    case OldMapStorage.IN_QUEUE:
    case OldMapStorage.DOWNLOAD_FAILED:
      return LIGHT;
    default:
      return REGULAR;
    }
  }

  public int getType()
  {
    if (mHasChildren)
      return OldDownloadAdapter.TYPE_GROUP;

    switch (mStatus)
    {
    case OldMapStorage.GROUP:
      return OldDownloadAdapter.TYPE_GROUP;
    case OldMapStorage.COUNTRY:
      return OldDownloadAdapter.TYPE_COUNTRY_GROUP;
    case OldMapStorage.NOT_DOWNLOADED:
      return OldDownloadAdapter.TYPE_COUNTRY_NOT_DOWNLOADED;
    case OldMapStorage.ON_DISK:
    case OldMapStorage.ON_DISK_OUT_OF_DATE:
      return OldDownloadAdapter.TYPE_COUNTRY_READY;
    default:
      return OldDownloadAdapter.TYPE_COUNTRY_IN_PROCESS;
    }
  }

  @Override
  public String toString()
  {
    return "Name : " + mName + "; options : " + mOptions + "; status : " + mStatus + "; has children : " + mHasChildren;
  }
}
