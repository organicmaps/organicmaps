package com.mapswithme.maps;

import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;

/**
 *  This class wraps android::Framework.cpp class
 *  via static methods
 */
public class Framework
{

  // API
  public interface OnApiPointActivatedListener
  {
    public void onApiPointActivated(double lat, double lon, String name, String id);
  }


  // POI
  public interface OnPoiActivatedListener
  {
    public void onPoiActivated(String name, String type, String address, double lat, double lon);
  }


  // Bookmark
  public interface OnBookmarkActivatedListener
  {
    public void onBookmarkActivated(int category, int bookmarkIndex);
  }

  //My Position
  public interface OnMyPositionActivatedListener
  {
    public void onMyPositionActivated(double lat, double lon);
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

  // API
  public static void setOnApiPointActivatedListener(OnApiPointActivatedListener listener)
  {
    nativeSetApiPointActivatedListener(listener);
  }

  public static void clearOnApiPointActivatedListener()
  {
    nativeClearApiPointActivatedListener();
  }


  // POI
  public static void setOnPoiActivatedListener(OnApiPointActivatedListener listener)
  {
    nativeSetOnPoiActivatedListener(listener);
  }

  public static void clearOnPoiActivatedListener()
  {
    nativeClearOnPoiActivatedListener();
  }

  // Bookmark
  public static void setOnBookmarkActivatedListener(OnBookmarkActivatedListener listener)
  {
    nativeSetOnBookmarkActivatedListener(listener);
  }

  public static void clearOnBookmarkActivatedListener()
  {
    nativeClearOnBookmarkActivatedListener();
  }

  // My Position
  public static void setOnMyPositionActivatedListener(OnMyPositionActivatedListener listener)
  {
    nativeSetOnMyPositionActivatedListener(listener);
  }

  public static void clearOnMyPositionActivatedListener()
  {
    nativeClearOnMyPositionActivatedListener();
  }

  public static void clearApiPoints()
  {
    nativeClearApiPoints();
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

  /*
   *  "Implementation" - native methods
   */
  private native static DistanceAndAzimut nativeGetDistanceAndAzimut(double merX, double merY, double cLat, double cLon, double north);
  private native static DistanceAndAzimut nativeGetDistanceAndAzimutFromLatLon(double lat, double lon, double cLat, double cLon, double north);

  private native static String nativeLatLon2DMS(double lat, double lon);

  private native static String nativeGetGe0Url(double lat, double lon, double zoomLevel, String name);
  private native static String nativeGetNameAndAddress4Point(double lat, double lon);

  // API point
  private native static void nativeSetApiPointActivatedListener(OnApiPointActivatedListener listener);
  private native static void nativeClearApiPointActivatedListener();
  // POI
  private native static void nativeSetOnPoiActivatedListener(OnApiPointActivatedListener listener);
  private native static void nativeClearOnPoiActivatedListener();
  // Bookmark
  private native static void nativeSetOnBookmarkActivatedListener(OnBookmarkActivatedListener listener);
  private native static void nativeClearOnBookmarkActivatedListener();
  // My Position
  private native static void nativeSetOnMyPositionActivatedListener(OnMyPositionActivatedListener listener);
  private native static void nativeClearOnMyPositionActivatedListener();

  private native static void nativeClearApiPoints();

  // this class is just bridge between Java and C++ worlds, we must not create it
  private Framework() {}
}
