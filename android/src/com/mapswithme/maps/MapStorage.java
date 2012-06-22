package com.mapswithme.maps;

public class MapStorage
{
  public static final int GROUP = -2;
  public static final int COUNTRY = -1;
  public static final int ON_DISK = 0;
  public static final int NOT_DOWNLOADED = 1;
  public static final int DOWNLOAD_FAILED = 2;
  public static final int DOWNLOADING = 3;
  public static final int IN_QUEUE = 4;
  public static final int UNKNOWN = 5;
  public static final int GENERATING_INDEX = 6;
  public static final int ON_DISK_OUT_OF_DATE = 7;

  public interface Listener
  {
    public void onCountryStatusChanged(Index idx);
    public void onCountryProgress(Index idx, long current, long total);
  };

  public static class Index
  {
    int mGroup;
    int mCountry;
    int mRegion;

    public Index()
    {
      mGroup = -1;
      mCountry = -1;
      mRegion = -1;
    }

    public Index(int group, int country, int region)
    {
      mGroup = group;
      mCountry = country;
      mRegion = region;
    }

    public boolean isRoot()
    {
      return (mGroup == -1 && mCountry == -1 && mRegion == -1);
    }

    public boolean isValid()
    {
      return !isRoot();
    }

    public Index getChild(int position)
    {
      Index ret = new Index(mGroup, mCountry, mRegion);

      if (ret.mGroup == -1) ret.mGroup = position;
      else if (ret.mCountry == -1) ret.mCountry = position;
      else
      {
        assert(ret.mRegion == -1);
        ret.mRegion = position;
      }

      return ret;
    }

    public Index getParent()
    {
      Index ret = new Index(mGroup, mCountry, mRegion);

      if (ret.mRegion != -1) ret.mRegion = -1;
      else if (ret.mCountry != -1) ret.mCountry = -1;
      else
      {
        assert(ret.mGroup != -1);
        ret.mGroup = -1;
      }

      return ret;
    }

    public boolean isEqual(Index idx)
    {
      return (mGroup == idx.mGroup && mCountry == idx.mCountry && mRegion == idx.mRegion);
    }

    public boolean isChild(Index idx)
    {
      return (idx.getParent().isEqual(this));
    }

    public int getPosition()
    {
      if (mRegion != -1) return mRegion;
      else if (mCountry != -1) return mCountry;
      else return mGroup;
    }

    @Override
    public String toString()
    {
      return ("Index(" + mGroup + ", " + mCountry + ", " + mRegion + ")");
    }
  }

  public native int countriesCount(Index idx);
  public native int countryStatus(Index idx);
  public native long countryLocalSizeInBytes(Index idx);
  public native long countryRemoteSizeInBytes(Index idx);
  public native String countryName(Index idx);
  public native String countryFlag(Index idx);

  public native void downloadCountry(Index idx);
  public native void deleteCountry(Index idx);
  public native void deleteCountryFiles(Index idx);

  public native Index findIndexByName(String name);

  public native void showCountry(Index idx);

  public native int subscribe(Listener l);
  public native void unsubscribe(int slotId);


  private MapStorage()
  {
  }

  private static MapStorage mInstance = null;

  public static MapStorage getInstance()
  {
    if (mInstance == null)
      mInstance = new MapStorage();

    return mInstance;
  }
}
