package com.mapswithme.maps;

import java.io.Serializable;

public enum MapStorage
{
  INSTANCE;

  public static final int GROUP = -2;
  public static final int COUNTRY = -1;

  // This constants should be equal with storage/storage_defines.hpp, storage::TStatus
  public static final int ON_DISK = 0;
  public static final int NOT_DOWNLOADED = 1;
  public static final int DOWNLOAD_FAILED = 2;
  public static final int DOWNLOADING = 3;
  public static final int IN_QUEUE = 4;
  public static final int UNKNOWN = 5;
  public static final int ON_DISK_OUT_OF_DATE = 6;
  public static final int DOWNLOAD_FAILED_OUF_OF_MEMORY = 7;

  /**
   * Callbacks are called from native code.
   */
  public interface Listener
  {
    void onCountryStatusChanged(Index idx);

    void onCountryProgress(Index idx, long current, long total);
  }

  public static class Index implements Serializable
  {
    private static final long serialVersionUID = 1L;

    private int mGroup;
    private int mCountry;
    private int mRegion;

    public Index()
    {
      mGroup = -1;
      mRegion = -1;
      mCountry = -1;
    }

    public Index(int group, int country, int region)
    {
      mGroup = group;
      mCountry = country;
      mRegion = region;
    }

    @Override
    public String toString()
    {
      return ("Index(" + mGroup + ", " + getCountry() + ", " + getRegion() + ")");
    }

    public int getCountry()
    {
      return mCountry;
    }

    public void setCountry(int mCountry)
    {
      this.mCountry = mCountry;
    }

    public int getRegion()
    {
      return mRegion;
    }

    public void setRegion(int mRegion)
    {
      this.mRegion = mRegion;
    }

    public int getGroup()
    {
      return mGroup;
    }

    public void setGroup(int group)
    {
      mGroup = group;
    }

  }

  public native int countryStatus(Index idx);

  public native long countryRemoteSizeInBytes(Index idx, int options);

  public native String countryName(Index idx);

  public native Index findIndexByFile(String name);

  public native int subscribe(Listener l);

  public native void unsubscribe(int slotId);

  public static native boolean nativeMoveFile(String oldFile, String newFile);
}
