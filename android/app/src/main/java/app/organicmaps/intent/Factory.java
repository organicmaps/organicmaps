package app.organicmaps.intent;

import static app.organicmaps.api.Const.EXTRA_PICK_POINT;

import android.content.ContentResolver;
import android.content.Intent;
import android.net.Uri;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.IntentCompat;

import app.organicmaps.Framework;
import app.organicmaps.Map;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.api.ParsedRoutingData;
import app.organicmaps.api.ParsedSearchRequest;
import app.organicmaps.api.ParsingResult;
import app.organicmaps.api.RoutePoint;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.bookmarks.data.FeatureId;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.search.SearchActivity;
import app.organicmaps.search.SearchEngine;
import app.organicmaps.util.StorageUtils;
import app.organicmaps.util.StringUtils;
import app.organicmaps.util.concurrency.ThreadPool;

import java.io.File;

public class Factory
{
  public static boolean isStartedForApiResult(@NonNull Intent intent)
  {
    return intent.getBooleanExtra(EXTRA_PICK_POINT, false);
  }

  public static class GeoIntentProcessor implements IntentProcessor
  {
    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity activity)
    {
      final String uri = processIntent(intent);
      if (uri == null)
        return false;
      return OpenUrlTask.run(uri, activity);
    }

    @Nullable
    public static String processIntent(@NonNull Intent intent)
    {
      final Uri uri = intent.getData();
      if (uri == null)
        return null;
      final String scheme = intent.getScheme();
      if (!"geo".equals(scheme) && !"ge0".equals(scheme) && !"om".equals(scheme) && !"mapsme".equals(scheme))
        return null;
      return uri.toString();
    }
  }

  public static class HttpGeoIntentProcessor implements IntentProcessor
  {
    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity activity)
    {
      final Uri uri = intent.getData();
      if (uri == null)
        return false;
      final String scheme = intent.getScheme();
      if (!"http".equalsIgnoreCase(scheme) && !"https".equalsIgnoreCase(scheme))
        return false;
      final String host = uri.getHost();
      if (!"omaps.app".equalsIgnoreCase(host) && !"ge0.me".equalsIgnoreCase(host))
        return false;
      if (uri.getPath() == null)
        return false;
      final String ge0Url = "om:/" + uri.getPath();
      return OpenUrlTask.run(ge0Url, activity);
    }
  }

  public static class HttpMapsIntentProcessor implements IntentProcessor
  {
    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity activity)
    {
      final Uri uri = intent.getData();
      if (uri == null)
        return false;
      final String scheme = intent.getScheme();
      if (!"http".equalsIgnoreCase(scheme) && !"https".equalsIgnoreCase(scheme))
        return false;
      if (uri.getHost() == null)
        return false;
      final String host = StringUtils.toLowerCase(uri.getHost());
      if (!host.contains("google") && !host.contains("2gis") && !host.contains("openstreetmap"))
        return false;
      return Map.showMapForUrl(uri.toString());
    }
  }

  public static class KmzKmlProcessor implements IntentProcessor
  {
    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity activity)
    {
      // See KML/KMZ/KMB intent filters in manifest.
      final Uri uri;
      if (Intent.ACTION_VIEW.equals(intent.getAction()))
        uri = intent.getData();
      else if (Intent.ACTION_SEND.equals(intent.getAction()))
        uri = IntentCompat.getParcelableExtra(intent, Intent.EXTRA_STREAM, Uri.class);
      else
        uri = null;
      if (uri == null)
        return false;

      MwmApplication app = MwmApplication.from(activity);
      final File tempDir = new File(StorageUtils.getTempPath(app));
      final ContentResolver resolver = activity.getContentResolver();
      ThreadPool.getStorage().execute(() -> {
        BookmarkManager.INSTANCE.importBookmarksFile(resolver, uri, tempDir);
      });
      return false;
    }
  }

  private static class OpenUrlTask
  {
    private static final int SEARCH_IN_VIEWPORT_ZOOM = 16;

    // TODO: will be merged into HttpMapsIntentProcessor later.
    public static boolean run(@NonNull String url, @NonNull MwmActivity target)
    {
      SearchEngine.INSTANCE.cancelInteractiveSearch();

      final ParsingResult result = Framework.nativeParseAndSetApiUrl(url);

      // TODO: Kernel recognizes "om://", "mapsme://", "mwm://" and "mapswithme://" schemas only!!!
      if (result.getUrlType() == ParsingResult.TYPE_INCORRECT)
        return Map.showMapForUrl(url);

      if (!result.isSuccess())
        return false;

      switch (result.getUrlType())
      {
        case ParsingResult.TYPE_INCORRECT:
          return false;

        case ParsingResult.TYPE_MAP:
          return Map.showMapForUrl(url);

        case ParsingResult.TYPE_ROUTE:
          final ParsedRoutingData data = Framework.nativeGetParsedRoutingData();
          RoutingController.get().setRouterType(data.mRouterType);
          final RoutePoint from = data.mPoints[0];
          final RoutePoint to = data.mPoints[1];
          RoutingController.get().prepare(MapObject.createMapObject(FeatureId.EMPTY, MapObject.API_POINT,
                                                                    from.mName, "", from.mLat, from.mLon),
                                          MapObject.createMapObject(FeatureId.EMPTY, MapObject.API_POINT,
                                                                    to.mName, "", to.mLat, to.mLon), true);
          return true;
        case ParsingResult.TYPE_SEARCH:
        {
          final ParsedSearchRequest request = Framework.nativeGetParsedSearchRequest();
          final double[] latlon = Framework.nativeGetParsedCenterLatLon();
          if (latlon[0] != 0.0 || latlon[1] != 0.0)
          {
            Framework.nativeStopLocationFollow();
            Framework.nativeSetViewportCenter(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
            // We need to update viewport for search api manually because of drape engine
            // will not notify subscribers when search activity is shown.
            if (!request.mIsSearchOnMap)
              Framework.nativeSetSearchViewport(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
          }
          SearchActivity.start(target, request.mQuery, request.mLocale, request.mIsSearchOnMap);
          return true;
        }
        case ParsingResult.TYPE_CROSSHAIR:
        {
          target.showPositionChooserForAPI(Framework.nativeGetParsedAppName());

          final double[] latlon = Framework.nativeGetParsedCenterLatLon();
          if (latlon[0] != 0.0 || latlon[1] != 0.0)
          {
            Framework.nativeStopLocationFollow();
            Framework.nativeSetViewportCenter(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
          }

          return true;
        }
      }

      return false;
    }
  }
}
