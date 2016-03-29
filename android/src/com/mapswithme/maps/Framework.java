package com.mapswithme.maps;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.Size;
import android.support.annotation.UiThread;

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
  public interface MapObjectListener
  {
    void onMapObjectActivated(MapObject object);

    void onDismiss(boolean switchFullScreenMode);
  }

  @SuppressWarnings("unused")
  public interface RoutingListener
  {
    void onRoutingEvent(int resultCode, String[] missingMaps);
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

  public static native int nativeGetDrawScale();

  @Size(2)
  public static native double[] nativeGetScreenRectCenter();

  public static native DistanceAndAzimut nativeGetDistanceAndAzimuth(double dstMerX, double dstMerY, double srcLat, double srcLon, double north);

  public static native DistanceAndAzimut nativeGetDistanceAndAzimuthFromLatLon(double dstLat, double dstLon, double srcLat, double srcLon, double north);

  public static native String nativeFormatLatLon(double lat, double lon, boolean useDmsFormat);

  @Size(2)
  public static native String[] nativeFormatLatLonToArr(double lat, double lon, boolean useDmsFormat);

  public static native String nativeFormatAltitude(double alt);

  public static native String nativeFormatSpeed(double speed);

  public static native String nativeGetGe0Url(double lat, double lon, double zoomLevel, String name);

  public static native String nativeGetNameAndAddress(double lat, double lon);

  public static native void nativeSetMapObjectListener(MapObjectListener listener);

  public static native void nativeRemoveMapObjectListener();

  @UiThread
  public static native String nativeGetOutdatedCountriesString();

  public static native boolean nativeIsDataVersionChanged();

  public static native void nativeUpdateSavedDataVersion();

  public static native long nativeGetDataVersion();

  public static native void nativeClearApiPoints();

  public static native void nativeDeactivatePopup();

  public static native String[] nativeGetMovableFilesExts();

  public static native String nativeGetBookmarksExt();

  public static native String nativeGetBookmarkDir();

  public static native String nativeGetSettingsDir();

  public static native String nativeGetWritableDir();

  public static native void nativeSetWritableDir(String newPath);

  public static native void nativeLoadBookmarks();

  // Routing.
  public static native boolean nativeIsRoutingActive();

  public static native boolean nativeIsRouteBuilt();

  public static native boolean nativeIsRouteBuilding();

  public static native void nativeCloseRouting();

  public static native void nativeBuildRoute(double startLat, double startLon, double finishLat, double finishLon);

  public static native void nativeFollowRoute();

  public static native void nativeDisableFollowing();

  public static native RoutingInfo nativeGetRouteFollowingInfo();

  // When an end user is going to a turn he gets sound turn instructions.
  // If C++ part wants the client to pronounce an instruction nativeGenerateTurnNotifications returns
  // an array of one of more strings. C++ part assumes that all these strings shall be pronounced by the client's TTS.
  // For example if C++ part wants the client to pronounce "Make a right turn." this method returns
  // an array with one string "Make a right turn.". The next call of the method returns nothing.
  // nativeGenerateTurnNotifications shall be called by the client when a new position is available.
  public static native String[] nativeGenerateTurnNotifications();

  public static native void nativeSetRoutingListener(RoutingListener listener);

  public static native void nativeSetRouteProgressListener(RoutingProgressListener listener);

  public static native void nativeShowCountry(String countryId, boolean zoomToDownloadButton);

  public static native double[] nativePredictLocation(double lat, double lon, double accuracy, double bearing, double speed, double elapsedSeconds);

  public static native void nativeSetMapStyle(int mapStyle);

  /**
   * This method allows to set new map style without immediate applying. It can be used before
   * engine recreation instead of nativeSetMapStyle to avoid huge flow of OpenGL invocations.
   * @param mapStyle style index
   */
  public static native void nativeMarkMapStyle(int mapStyle);

  public static native void nativeSetRouter(int routerType);

  public static native int nativeGetRouter();

  public static native int nativeGetLastUsedRouter();

  /**
   * @return {@link Framework#ROUTER_TYPE_VEHICLE} or {@link Framework#ROUTER_TYPE_PEDESTRIAN}
   */
  public static native int nativeGetBestRouter(double srcLat, double srcLon, double dstLat, double dstLon);

  public static native void nativeSetRouteStartPoint(double lat, double lon, boolean valid);

  public static native void nativeSetRouteEndPoint(double lat, double lon, boolean valid);

  /**
   * Registers all maps(.mwms). Adds them to the models, generates indexes and does all necessary stuff.
   */
  public static native void nativeRegisterMaps();

  public static native void nativeDeregisterMaps();

  /**
   * Determines if currently is day or night at the given location. Used to switch day/night styles.
   * @param utcTimeSeconds Unix time in seconds.
   * @param lat latitude of the current location.
   * @param lon longitude of the current location.
   * @return {@code true} if it is day now or {@code false} otherwise.
   */
  public static native boolean nativeIsDayTime(long utcTimeSeconds, double lat, double lon);

  public static native void nativeGet3dMode(Params3dMode result);

  public static native void nativeSet3dMode(boolean allow3d, boolean allow3dBuildings);

  @NonNull
  public static native MapObject nativeDeleteBookmarkFromMapObject();

  public static native void nativeZoomToPoint(double lat, double lon, int zoom, boolean animate);


  public static native void nativeTurnChoosePositionMode(boolean turnedOn);

  public static native boolean nativeIsDownloadedMapAtScreenCenter();

  public static native String nativeGetActiveObjectFormattedCuisine();
}
