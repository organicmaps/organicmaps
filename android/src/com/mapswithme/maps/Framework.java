package com.mapswithme.maps;

import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.MapObject.SearchResult;
import com.mapswithme.maps.bookmarks.data.Track;
import com.mapswithme.maps.guides.GuideInfo;
import com.mapswithme.util.Utils;

/**
 *  This class wraps android::Framework.cpp class
 *  via static methods
 */
public class Framework
{
  public interface OnBalloonListener
  {
    public void onApiPointActivated(double lat, double lon, String name, String id);
    public void onPoiActivated(String name, String type, String address, double lat, double lon);
    public void onBookmarkActivated(int category, int bookmarkIndex);
    public void onMyPositionActivated(double lat, double lon);
    public void onAdditionalLayerActivated(long index);
    public void onDismiss();
  }
  
  static public String GetSettingsDir()
  {
    return nativeGetSettingsDir();
  }
  
  static public String GetBookmarksDir()
  {
    return nativeGetBookmarkDir();
  }
  
  static public String GetWritableDir()
  {
    return nativeGetWritableDir();
  }
  
  static public String GetBookmarkFileExt()
  {
    return nativeGetBookmarksExt();
  }
  
  static public String[] GetMovableFilesExt()
  {
    return nativeGetMovablefilesExt();
  }
  
  static public void SetWritableDir(String newPath)
  {
    nativeSetWritableDir(newPath);
  }
  
  static public void ReloadBookmarks()
  {
    nativeLoadbookmarks();
  }

  // Interface

  public static String getGe0Url(double lat, double lon, double zoomLevel, String name)
  {
    return nativeGetGe0Url(lat, lon, zoomLevel, name);
  }

  public static String getHttpGe0Url(double lat, double lon, double zoomLevel, String name)
  {
    return getGe0Url(lat, lon, zoomLevel, name).replaceFirst("ge0://", "http://ge0.me/");
  }

  public static String getNameAndAddress4Point(double lat, double lon)
  {
    return nativeGetNameAndAddress4Point(lat, lon);
  }

  public static void connectBalloonListeners(OnBalloonListener listener)
  {
    nativeConnectBalloonListeners(listener);
  }

  public static void clearBalloonListeners()
  {
    nativeClearBalloonListeners();
  }

  public static void clearApiPoints()
  {
    //nativeClearApiPoints();
  }

  public static DistanceAndAzimut getDistanceAndAzimut(double merX, double merY, double cLat, double cLon, double north)
  {
    return nativeGetDistanceAndAzimut(merX, merY, cLat, cLon, north);
  }

  public static DistanceAndAzimut getDistanceAndAzimutFromLatLon(double lat, double lon, double cLat, double cLon, double north)
  {
    return nativeGetDistanceAndAzimutFromLatLon(lat, lon, cLat, cLon, north);
  }

  public static String latLon2DMS(double lat, double lon)
  {
    return nativeLatLon2DMS(lat, lon);
  }

  public static String getOutdatedCountriesString()
  {
    return nativeGetOutdatedCountriesString();
  }

  public static boolean isDataVersionChanged()
  {
    return nativeIsDataVersionChanged();
  }

  public static void updateSavedDataVersion()
  {
    nativeUpdateSavedDataVersion();
  }

  public static void showTrackRect(Track track)
  {
    nativeShowTrackRect(track.getCategoryId(), track.getTrackId());
  }
  private static native void nativeShowTrackRect(int category, int track);

  public native static GuideInfo getGuideInfoForIndex(Index idx);

  public static GuideInfo getGuideInfoForIndexWithApiCheck(Index idx)
  {
    if (Utils.apiLowerThan(11))
      return null;
    else
      return getGuideInfoForIndex(idx);
  }

  public native static void        setWasAdvertised(String appId);
  public native static boolean     wasAdvertised(String appId);

  public native static int getDrawScale();
  public native static double[] getScreenRectCenter();

  /*
   *  "Implementation" - native methods
   */
  private native static DistanceAndAzimut nativeGetDistanceAndAzimut(double merX, double merY, double cLat, double cLon, double north);
  private native static DistanceAndAzimut nativeGetDistanceAndAzimutFromLatLon(double lat, double lon, double cLat, double cLon, double north);

  private native static String nativeLatLon2DMS(double lat, double lon);

  private native static String nativeGetGe0Url(double lat, double lon, double zoomLevel, String name);
  private native static String nativeGetNameAndAddress4Point(double lat, double lon);

  private native static void nativeConnectBalloonListeners(OnBalloonListener listener);
  private native static void nativeClearBalloonListeners();

  private native static String nativeGetOutdatedCountriesString();
  private native static boolean nativeIsDataVersionChanged();
  private native static void nativeUpdateSavedDataVersion();

  private native static void nativeClearApiPoints();


  // this class is just bridge between Java and C++ worlds, we must not create it
  private Framework() {}

  public native static void injectData(SearchResult searchResult, long index);
  public native static void cleanSearchLayerOnMap();
  public native static void invalidate();
  
  private native static String[] nativeGetMovablefilesExt();
  private native static String   nativeGetBookmarksExt();
  private native static String   nativeGetBookmarkDir();
  private native static String   nativeGetSettingsDir();
  private native static String   nativeGetWritableDir();
  private native static void     nativeSetWritableDir(String newPath);
  private native static void     nativeLoadbookmarks();
}
