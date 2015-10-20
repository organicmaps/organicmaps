package com.mapswithme.maps;

import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.SearchResult;
import com.mapswithme.maps.routing.RoutingInfo;
import com.mapswithme.util.Constants;

/**
 * This class wraps android::Framework.cpp class
 * via static methods
 */
public class Framework
{
  public static final int MAP_STYLE_LIGHT = 0;
  public static final int MAP_STYLE_DARK = 1;
  public static final int MAP_STYLE_CLEAR = 2;

  public static final int ROUTER_TYPE_VEHICLE = 0;
  public static final int ROUTER_TYPE_PEDESTRIAN = 1;

  // should correspond to values from 'information_display.hpp' in core
  public static final int MAP_WIDGET_RULER = 0;
  public static final int MAP_WIDGET_COPYRIGHT = 1;
  public static final int MAP_WIDGET_COUNTRY_STATUS = 2;
  public static final int MAP_WIDGET_COMPASS = 3;
  public static final int MAP_WIDGET_DEBUG_LABEL = 4;

  @SuppressWarnings("unused")
  public interface OnBalloonListener
  {
    void onApiPointActivated(double lat, double lon, String name, String id);

    void onPoiActivated(String name, String type, String address, double lat, double lon, int[] metaTypes, String[] metaValues);

    void onBookmarkActivated(int category, int bookmarkIndex);

    void onMyPositionActivated(double lat, double lon);

    void onAdditionalLayerActivated(String name, String type, double lat, double lon, int[] metaTypes, String[] metaValues);

    void onDismiss();
  }

  @SuppressWarnings("unused")
  public interface RoutingListener
  {
    void onRoutingEvent(int resultCode, Index[] missingCountries, Index[] missingRoutes);
  }

  @SuppressWarnings("unused")
  public interface RoutingProgressListener
  {
    void onRouteBuildingProgress(float progress);
  }

  // this class is just bridge between Java and C++ worlds, we must not create it
  private Framework() {}

  public static String getHttpGe0Url(double lat, double lon, double zoomLevel, String name)
  {
    return nativeGetGe0Url(lat, lon, zoomLevel, name).replaceFirst(Constants.Url.GE0_PREFIX, Constants.Url.HTTP_GE0_PREFIX);
  }

  public static native void nativeShowTrackRect(int category, int track);

  public native static int getDrawScale();

  public native static double[] getScreenRectCenter();

  public native static DistanceAndAzimut nativeGetDistanceAndAzimut(double dstMerX, double dstMerY, double srcLat, double srcLon, double north);

  public native static DistanceAndAzimut nativeGetDistanceAndAzimutFromLatLon(double dstLat, double dstLon, double srcLat, double srcLon, double north);

  public native static String nativeFormatLatLon(double lat, double lon, boolean useDMSFormat);

  public native static String[] nativeFormatLatLonToArr(double lat, double lon, boolean useDMSFormat);

  public native static String nativeFormatAltitude(double alt);

  public native static String nativeFormatSpeed(double speed);

  public native static String nativeGetGe0Url(double lat, double lon, double zoomLevel, String name);

  public native static String nativeGetNameAndAddress4Point(double lat, double lon);

  public native static MapObject nativeGetMapObjectForPoint(double lat, double lon);

  public native static void nativeActivateUserMark(double lat, double lon);

  public native static void nativeSetBalloonListener(OnBalloonListener listener);

  public native static void nativeRemoveBalloonListener();

  public native static String nativeGetOutdatedCountriesString();

  public native static boolean nativeIsDataVersionChanged();

  public native static void nativeUpdateSavedDataVersion();

  public native static void nativeClearApiPoints();

  public native static void injectData(SearchResult searchResult, long index);

  public native static void invalidate();

  public native static void deactivatePopup();

  public native static String[] nativeGetMovableFilesExts();

  public native static String nativeGetBookmarksExt();

  public native static String nativeGetBookmarkDir();

  public native static String nativeGetSettingsDir();

  public native static String nativeGetWritableDir();

  public native static void nativeSetWritableDir(String newPath);

  public native static void nativeLoadBookmarks();

  // Routing.
  public native static boolean nativeIsRoutingActive();

  public native static boolean nativeIsRouteBuilt();

  public native static boolean nativeIsRouteBuilding();

  public native static void nativeCloseRouting();

  public native static void nativeBuildRoute(double startLat, double startLon, double finishLat, double finishLon);

  public native static void nativeFollowRoute();

  public native static RoutingInfo nativeGetRouteFollowingInfo();

  // When an end user is going to a turn he gets sound turn instructions.
  // If C++ part wants the client to pronounce an instruction nativeGenerateTurnSound returns
  // an array of one of more strings. C++ part assumes that all these strings shall be pronounced by the client's TTS.
  // For example if C++ part wants the client to pronounce "Make a right turn." this method returns
  // an array with one string "Make a right turn.". The next call of the method returns nothing.
  // nativeGenerateTurnSound shall be called by the client when a new position is available.
  public native static String[] nativeGenerateTurnSound();

  public native static void nativeSetRoutingListener(RoutingListener listener);

  public native static void nativeSetRouteProgressListener(RoutingProgressListener listener);

  // TODO consider implementing other model of listeners connection, and implement methods below then
//  public native static void nativeRemoveRoutingListener();
//
//  public native static void nativeRemoveRouteProgressListener();
  //

  public native static String nativeGetCountryNameIfAbsent(double lat, double lon);

  public native static Index nativeGetCountryIndex(double lat, double lon);

  public native static String nativeGetViewportCountryNameIfAbsent();

  public native static void nativeShowCountry(Index idx, boolean zoomToDownloadButton);

  // TODO consider removal of that methods
  public native static void downloadCountry(Index idx);

  public native static double[] predictLocation(double lat, double lon, double accuracy, double bearing, double speed, double elapsedSeconds);

  public native static void setMapStyle(int mapStyle);

  public native static int getMapStyle();

  public native static void setRouter(int routerType);

  public native static int getRouter();

  /**
   * @return {@link Framework#ROUTER_TYPE_VEHICLE} or {@link Framework#ROUTER_TYPE_PEDESTRIAN}
   */
  public native static int nativeGetBestRouter(double srcLat, double srcLon, double dstLat, double dstLon);

  public native static void setWidgetPivot(int widget, int pivotX, int pivotY);

  /**
   * Registers all maps(.mwms). Adds them to the models, generates indexes and does all necessary stuff.
   */
  public native static void nativeRegisterMaps();

  public native static void nativeDeregisterMaps();
}
