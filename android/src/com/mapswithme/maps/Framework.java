package com.mapswithme.maps;

import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.MapObject;
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

  public static class Params3dMode
  {
    public boolean enabled;
    public boolean buildings;
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

  public native static void nativeSetBalloonListener(OnBalloonListener listener);

  public native static void nativeRemoveBalloonListener();

  public native static String nativeGetOutdatedCountriesString();

  public native static boolean nativeIsDataVersionChanged();

  public native static void nativeUpdateSavedDataVersion();

  public native static long nativeGetDataVersion();

  public native static void nativeClearApiPoints();

  public native static void injectData(MapObject.SearchResult searchResult, long index);

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

  public native static void nativeDisableFollowing();

  public native static RoutingInfo nativeGetRouteFollowingInfo();

  // When an end user is going to a turn he gets sound turn instructions.
  // If C++ part wants the client to pronounce an instruction nativeGenerateTurnNotifications returns
  // an array of one of more strings. C++ part assumes that all these strings shall be pronounced by the client's TTS.
  // For example if C++ part wants the client to pronounce "Make a right turn." this method returns
  // an array with one string "Make a right turn.". The next call of the method returns nothing.
  // nativeGenerateTurnNotifications shall be called by the client when a new position is available.
  public native static String[] nativeGenerateTurnNotifications();

  public native static void nativeSetRoutingListener(RoutingListener listener);

  public native static void nativeSetRouteProgressListener(RoutingProgressListener listener);

  public native static String nativeGetCountryNameIfAbsent(double lat, double lon);

  public native static Index nativeGetCountryIndex(double lat, double lon);

  public native static String nativeGetViewportCountryNameIfAbsent();

  public native static void nativeShowCountry(Index idx, boolean zoomToDownloadButton);

  // TODO consider removal of that methods
  public native static void downloadCountry(Index idx);

  public native static double[] predictLocation(double lat, double lon, double accuracy, double bearing, double speed, double elapsedSeconds);

  public native static void nativeSetMapStyle(int mapStyle);

  /**
   * This method allows to set new map style without immediate applying. It can be used before
   * engine recreation instead of nativeSetMapStyle to avoid huge flow of OpenGL invocations.
   * @param mapStyle style index
   */
  public native static void nativeMarkMapStyle(int mapStyle);

  public native static void nativeSetRouter(int routerType);

  public native static int nativeGetRouter();

  public native static int nativeGetLastUsedRouter();

  /**
   * @return {@link Framework#ROUTER_TYPE_VEHICLE} or {@link Framework#ROUTER_TYPE_PEDESTRIAN}
   */
  public native static int nativeGetBestRouter(double srcLat, double srcLon, double dstLat, double dstLon);

  public native static void nativeSetRouteStartPoint(double lat, double lon, boolean valid);

  public native static void nativeSetRouteEndPoint(double lat, double lon, boolean valid);

  /**
   * Registers all maps(.mwms). Adds them to the models, generates indexes and does all necessary stuff.
   */
  public native static void nativeRegisterMaps();

  public native static void nativeDeregisterMaps();

  /**
   * Determines if currently is day or night at the given location. Used to switch day/night styles.
   * @param utcTimeSeconds Unix time in seconds.
   * @param lat latitude of the current location.
   * @param lon longitude of the current location.
   * @return {@code true} if it is day now or {@code false} otherwise.
   */
  public static native boolean nativeIsDayTime(long utcTimeSeconds, double lat, double lon);

  public native static void nativeGet3dMode(Params3dMode result);

  public native static void nativeSet3dMode(boolean allow3d, boolean allow3dBuildings);
}
