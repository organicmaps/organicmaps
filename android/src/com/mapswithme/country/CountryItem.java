package com.mapswithme.country;

import android.graphics.Typeface;

import com.mapswithme.maps.MapStorage;

public class CountryItem
{
  public final String mName;
  public final MapStorage.Index mCountryIdx;
  public final String mFlag;

  private int mStatus;

  private static final Typeface LIGHT = Typeface.create("sans-serif-light", Typeface.NORMAL);
  private static final Typeface REGULAR = Typeface.create("sans-serif", Typeface.NORMAL);

  public CountryItem(MapStorage.Index idx)
  {
    mCountryIdx = idx;
    mName = MapStorage.INSTANCE.countryName(idx);

    final String flag = MapStorage.INSTANCE.countryFlag(idx);
    // The aapt can't process resources with name "do". Hack with renaming.
    mFlag = flag.equals("do") ? "do_hack" : flag;

    updateStatus();
  }

  public int getStatus()
  {
    return mStatus;
  }

  public void updateStatus()
  {
    if (mCountryIdx.getCountry() == -1 || (mCountryIdx.getRegion() == -1 && mFlag.length() == 0))
      mStatus = MapStorage.GROUP;
    else if (mCountryIdx.getRegion() == -1 && MapStorage.INSTANCE.countriesCount(mCountryIdx) > 0)
      mStatus = MapStorage.COUNTRY;
    else
      mStatus = MapStorage.INSTANCE.countryStatus(mCountryIdx);
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

  public int getType()
  {
    switch (getStatus())
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
}
