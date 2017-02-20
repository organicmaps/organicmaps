package com.mapswithme.maps;

import android.graphics.Bitmap;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.Size;
import android.support.annotation.UiThread;

import com.mapswithme.maps.api.ParsedRoutingData;
import com.mapswithme.maps.api.ParsedUrlMwmRequest;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.routing.RoutingInfo;
import com.mapswithme.util.Constants;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * This class wraps android::Framework.cpp class
 * via static methods
 */
public class Framework
{
  public static final int MAP_STYLE_CLEAR = 0;
  public static final int MAP_STYLE_DARK = 1;
  public static final int MAP_STYLE_VEHICLE_CLEAR = 3;
  public static final int MAP_STYLE_VEHICLE_DARK = 4;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ROUTER_TYPE_VEHICLE, ROUTER_TYPE_PEDESTRIAN, ROUTER_TYPE_BICYCLE, ROUTER_TYPE_TAXI})

  public @interface RouterType {}

  public static final int ROUTER_TYPE_VEHICLE = 0;
  public static final int ROUTER_TYPE_PEDESTRIAN = 1;
  public static final int ROUTER_TYPE_BICYCLE = 2;
  public static final int ROUTER_TYPE_TAXI = 3;

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

  public static class RouteAltitudeLimits
  {
    public int minRouteAltitude;
    public int maxRouteAltitude;
    public boolean isMetricUnits;
  }

  // this class is just bridge between Java and C++ worlds, we must not create it
  private Framework() {}

  public static String getHttpGe0Url(double lat, double lon, double zoomLevel, String name)
  {
    return nativeGetGe0Url(lat, lon, zoomLevel, name).replaceFirst(Constants.Url.GE0_PREFIX, Constants.Url.HTTP_GE0_PREFIX);
  }

  /**
   * Generates Bitmap with route altitude image chart taking into account current map style.
   * @param width is width of the image.
   * @param height is height of the image.
   * @return Bitmap if there's pedestrian or bicycle route and null otherwise.
   */
  @Nullable
  public static Bitmap generateRouteAltitudeChart(int width, int height,
                                                  @NonNull RouteAltitudeLimits limits)
  {
    if (width <= 0 || height <= 0)
      return null;

    final int[] altitudeChartBits = Framework.nativeGenerateRouteAltitudeChartBits(width, height,
                                                                                   limits);
    if (altitudeChartBits == null)
      return null;

    return Bitmap.createBitmap(altitudeChartBits, width, height, Bitmap.Config.ARGB_8888);
  }

  public static native void nativeShowTrackRect(int category, int track);

  public static native int nativeGetDrawScale();
  
  public static native int nativeUpdateUserViewportChanged();

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
  @ParsedUrlMwmRequest.ParsingResult
  public static native int nativeParseAndSetApiUrl(String url);
  public static native ParsedRoutingData nativeGetParsedRoutingData();

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

  public static native void nativeBuildRoute(double startLat, double startLon, double finishLat, double finishLon, boolean isP2P);

  public static native void nativeFollowRoute();

  public static native void nativeDisableFollowing();

  @Nullable
  public static native RoutingInfo nativeGetRouteFollowingInfo();

  @Nullable
  public static native final int[] nativeGenerateRouteAltitudeChartBits(int width, int height, RouteAltitudeLimits routeAltitudeLimits);

  // When an end user is going to a turn he gets sound turn instructions.
  // If C++ part wants the client to pronounce an instruction nativeGenerateTurnNotifications returns
  // an array of one of more strings. C++ part assumes that all these strings shall be pronounced by the client's TTS.
  // For example if C++ part wants the client to pronounce "Make a right turn." this method returns
  // an array with one string "Make a right turn.". The next call of the method returns nothing.
  // nativeGenerateTurnNotifications shall be called by the client when a new position is available.
  @Nullable
  public static native String[] nativeGenerateTurnNotifications();

  public static native void nativeSetRoutingListener(RoutingListener listener);

  public static native void nativeSetRouteProgressListener(RoutingProgressListener listener);

  public static native void nativeShowCountry(String countryId, boolean zoomToDownloadButton);

  public static native void nativeSetMapStyle(int mapStyle);

  /**
   * This method allows to set new map style without immediate applying. It can be used before
   * engine recreation instead of nativeSetMapStyle to avoid huge flow of OpenGL invocations.
   * @param mapStyle style index
   */
  public static native void nativeMarkMapStyle(int mapStyle);

  public static native void nativeSetRouter(@RouterType int routerType);
  @RouterType
  public static native int nativeGetRouter();
  @RouterType
  public static native int nativeGetLastUsedRouter();
  @RouterType
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

  public static native boolean nativeGetAutoZoomEnabled();

  public static native void nativeSetAutoZoomEnabled(boolean enabled);

  public static native boolean nativeGetSimplifiedTrafficColorsEnabled();

  public static native void nativeSetSimplifiedTrafficColorsEnabled(boolean enabled);

  @NonNull
  public static native MapObject nativeDeleteBookmarkFromMapObject();

  // TODO remove that hack after bookmarks will be refactored completely
  public static native void nativeOnBookmarkCategoryChanged(int cat, int bmk);

  public static native void nativeZoomToPoint(double lat, double lon, int zoom, boolean animate);

  /**
   * @param isBusiness selection area will be bounded by building borders, if its true(eg. true for businesses in buildings).
   * @param applyPosition if true, map'll be animated to currently selected object.
   */
  public static native void nativeTurnOnChoosePositionMode(boolean isBusiness, boolean applyPosition);
  public static native void nativeTurnOffChoosePositionMode();
  public static native boolean nativeIsInChoosePositionMode();
  public static native boolean nativeIsDownloadedMapAtScreenCenter();
  public static native String nativeGetActiveObjectFormattedCuisine();

  public static native void nativeSetVisibleRect(int left, int top, int right, int bottom);

  // Navigation.
  public static native boolean nativeIsRouteFinished();
}
