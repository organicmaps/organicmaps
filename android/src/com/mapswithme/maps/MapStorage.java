package com.mapswithme.maps;

public class MapStorage
{
  public static final int ON_DISK = 0;
  public static final int NOT_DOWNLOADED = 1;
  public static final int DOWNLOAD_FAILED = 2;
  public static final int DOWNLOADING = 3;
  public static final int IN_QUEUE = 4;
  public static final int UNKNOWN = 5;
  public static final int GENERATING_INDEX = 6;
  
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
    
    public boolean isValid()
    {
      return !((mGroup == -1) && (mCountry == -1) && (mRegion == -1));
    }
  }
  
  public native int countriesCount(Index idx);
  public native int countryStatus(Index idx);
  public native long countryLocalSizeInBytes(Index idx);
  public native long countryRemoteSizeInBytes(Index idx);
  public native String countryName(Index idx);
  
  public native void downloadCountry(Index idx);
  public native void deleteCountry(Index idx);
  public native Index findIndexByName(String name);

  private MapStorage() 
  {}
  
  private static MapStorage mInstance = null;
  
  public static MapStorage getInstance()
  {
    if (mInstance == null)
      mInstance = new MapStorage();
    
    return mInstance;
  }
  
  public native int subscribe(Listener l);
  public native void unsubscribe(int slotId);
}
