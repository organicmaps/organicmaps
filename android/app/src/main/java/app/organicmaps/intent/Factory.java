package app.organicmaps.intent;

import static app.organicmaps.api.Const.EXTRA_PICK_POINT;

import android.content.ContentResolver;
import android.content.Intent;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.core.content.IntentCompat;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.editor.OsmLoginActivity;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.api.ParsedRoutingData;
import app.organicmaps.sdk.api.ParsedSearchRequest;
import app.organicmaps.sdk.api.RequestType;
import app.organicmaps.sdk.api.RoutePoint;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.search.SearchActivity;
import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

public class Factory
{
  public static boolean isStartedForApiResult(@NonNull Intent intent)
  {
    // Previously, we relied on the implicit FORWARD_RESULT_FLAG to detect if the caller was
    // waiting for a result. However, this approach proved to be less reliable than using
    // the explicit EXTRA_PICK_POINT flag.
    // https://github.com/organicmaps/organicmaps/pull/8910
    return intent.getBooleanExtra(EXTRA_PICK_POINT, false);
  }

  public static class KmzKmlProcessor implements IntentProcessor
  {
    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity activity)
    {
      // See KML/KMZ/KMB intent filters in manifest.
      final List<Uri> uris;
      if (Intent.ACTION_VIEW.equals(intent.getAction()))
      {
        final Uri uri = intent.getData();
        if (uri == null || !shouldImportActionViewUri(uri.getScheme(), intent.getType()))
          return false;
        uris = Collections.singletonList(uri);
      }
      else if (Intent.ACTION_SEND.equals(intent.getAction()))
        uris = Collections.singletonList(IntentCompat.getParcelableExtra(intent, Intent.EXTRA_STREAM, Uri.class));
      else if (Intent.ACTION_SEND_MULTIPLE.equals(intent.getAction()))
        uris = intent.getParcelableArrayListExtra(Intent.EXTRA_STREAM);
      else
        uris = null;
      if (uris == null)
        return false;

      MwmApplication app = MwmApplication.from(activity);
      final File tempDir = new File(StorageUtils.getTempPath(app));
      final ContentResolver resolver = activity.getContentResolver();
      ThreadPool.getStorage().execute(() -> BookmarkManager.INSTANCE.importBookmarksFiles(resolver, uris, tempDir));
      return false;
    }

    static boolean shouldImportActionViewUri(String scheme, String mimeType)
    {
      if (scheme == null)
        return false;

      switch (scheme.toLowerCase(Locale.ROOT))
      {
      case "content":
      case "file":
      case "data":
        return true;
      case "http":
      case "https":
        return isBookmarksMimeType(mimeType);
      default:
        return false;
      }
    }

    private static boolean isBookmarksMimeType(String mimeType)
    {
      if (mimeType == null)
        return false;

      switch (mimeType.toLowerCase(Locale.ROOT))
      {
      case "application/vnd.google-earth.kml+xml":
      case "application/vnd.google-earth.kmz":
      case "application/gpx":
      case "application/gpx+xml":
      case "application/vnd.google-earth.kmz+xml":
      case "application/geo+json":
      case "application/vnd.geo+json":
      case "application/json":
        return true;
      default:
        return false;
      }
    }
  }

  public static class UrlProcessor implements IntentProcessor
  {
    private static final int SEARCH_IN_VIEWPORT_ZOOM = 16;

    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity target)
    {
      final Uri uri = intent.getData();
      if (uri == null)
        return false;

      switch (Framework.nativeParseAndSetApiUrl(uri.toString()))
      {
      case RequestType.INCORRECT: return false;

      case RequestType.MAP:
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        Map.executeMapApiRequest();
        return true;

      case RequestType.ROUTE:
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        final ParsedRoutingData data = Framework.nativeGetParsedRoutingData();
        final List<MapObject> routePoints = new ArrayList<>(data.mPoints.length);
        final List<String> callbacks = new ArrayList<>(data.mPoints.length);
        for (RoutePoint point : data.mPoints)
        {
          final String pointName = point.mIsMyPosition && point.mName.isEmpty()
                                     ? target.getString(app.organicmaps.sdk.R.string.core_my_position)
                                     : point.mName;
          routePoints.add(MapObject.createMapObject(point.mIsMyPosition ? MapObject.MY_POSITION : MapObject.API_POINT,
                                                    pointName, "", point.mLat, point.mLon));
          callbacks.add(point.mCallback);
        }
        RoutingController.get().prepare(routePoints, data.mRouterType, data.mOptimizeRoutePoints,
                                        data.mStartRouteNavigation, callbacks, data.mStartDirectionX,
                                        data.mStartDirectionY);
        return true;
      case RequestType.SEARCH:
      {
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        final ParsedSearchRequest request = Framework.nativeGetParsedSearchRequest();
        final double[] latlon = Framework.nativeGetParsedCenterLatLon();
        if (latlon != null)
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
      case RequestType.CROSSHAIR:
      {
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        target.showPositionChooserForAPI(Framework.nativeGetParsedAppName());

        final double[] latlon = Framework.nativeGetParsedCenterLatLon();
        if (latlon != null)
        {
          Framework.nativeStopLocationFollow();
          Framework.nativeSetViewportCenter(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
        }

        return true;
      }
      case RequestType.OAUTH2:
      {
        SearchEngine.INSTANCE.cancelInteractiveSearch();

        final String oauth2code = Framework.nativeGetParsedOAuth2Code();
        OsmLoginActivity.OAuth2Callback(target, oauth2code);

        return true;
      }

      // Menu and Settings url types should be implemented to support deeplinking.
      case RequestType.MENU:
      case RequestType.SETTINGS:
      }

      return false;
    }
  }

  public static class GoggleAssistanceIntentProcessor extends GoogleAssistantIntentHandler implements IntentProcessor
  {
    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity activity)
    {
      return handleIntent(intent,
                          (query, searchOnMap) -> { SearchActivity.start(activity, query, null, searchOnMap); });
    }
  }
}
