package app.organicmaps.intent;

import static app.organicmaps.api.Const.EXTRA_PICK_POINT;

import android.content.ContentResolver;
import android.content.Intent;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.IntentCompat;

import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.editor.OsmLoginActivity;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.api.ParsedRoutingData;
import app.organicmaps.sdk.api.ParsedSearchRequest;
import app.organicmaps.sdk.api.RequestType;
import app.organicmaps.sdk.api.RoutePoint;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.settings.RoadType;
import app.organicmaps.sdk.sound.TtsPlayer;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.search.SearchActivity;

import java.io.File;
import java.util.Collections;
import java.util.List;

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
        uris = Collections.singletonList(intent.getData());
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
      case RequestType.INCORRECT:
        return false;

      case RequestType.MAP:
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        Map.executeMapApiRequest();
        return true;

      case RequestType.ROUTE:
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        final ParsedRoutingData data = Framework.nativeGetParsedRoutingData();
        RoutingController.get().setRouterType(data.mRouterType);
        final RoutePoint from = data.mPoints[0];
        final RoutePoint to = data.mPoints[1];
        RoutingController.get().prepare(
            MapObject.createMapObject(MapObject.API_POINT, from.mName, "", from.mLat, from.mLon),
            MapObject.createMapObject(MapObject.API_POINT, to.mName, "", to.mLat, to.mLon));
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

  public static class AssistantIntentProcessor implements IntentProcessor
  {
    private static final String TAG = "AssistantIntentProc";
    private static final int SEARCH_IN_VIEWPORT_ZOOM = 16;

    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity activity)
    {
      try
      {
        final Uri uri = intent.getData();
        if (uri == null)
          return false;

        final String scheme = uri.getScheme();
        final String action = intent.getAction();
        if (scheme == null || action == null)
          return false;

        if ("geo.action".equals(scheme) || "geo.action.offline".equals(scheme))
          return handleCustomAction(intent, uri, activity);

        if ("geo".equals(scheme) || "geo.offline".equals(scheme))
        {
          if ("android.intent.action.NAVIGATE".equals(action) || "androidx.car.app.action.NAVIGATE".equals(action))
          {
            return handleNavigationIntent(uri, activity);
          }

          if (Intent.ACTION_VIEW.equals(action))
          {
            final String entry = uri.getQueryParameter("entry");
            if ("assistant".equals(entry))
              return handleAssistantSearchIntent(uri, activity);
            return false;
          }
        }

        return false;
      } catch (Exception e)
      {
        Logger.e(TAG, "Failed to process assistant intent", e);
        return false;
      }
    }

    /**
     * Destination data holder
     */
    private static class DestinationData
    {
      public double lat = 0.0;
      public double lon = 0.0;
      @Nullable
      public String query = null;

      public boolean hasLatLon()
      {
        return lat != 0.0 || lon != 0.0;
      }
    }

    private static boolean handleNavigationIntent(@NonNull Uri uri, @NonNull MwmActivity activity)
    {
      try
      {
        final DestinationData dst = parseDestination(uri);
        if (!dst.hasLatLon() && (dst.query == null || dst.query.trim().isEmpty()))
          return false;

        String intentParam = uri.getQueryParameter("intent");
        if (intentParam == null)
        {
          intentParam = "navigation";
        }

        Router routingMode = parseRoutingMode(uri);

        applyAvoidOptions(uri);

        RoutingController.get().setRouterType(routingMode);

        if (!dst.hasLatLon())
        {
          if (dst.query != null && !dst.query.isEmpty())
          {
            SearchEngine.INSTANCE.cancelInteractiveSearch();
            SearchActivity.start(activity, dst.query, "", true);
            return true;
          }
          return false;
        }

        final String name = dst.query != null ? dst.query : "";
        final MapObject dest = MapObject.createMapObject(
            MapObject.API_POINT, name, "", dst.lat, dst.lon);

        switch (intentParam)
        {
        case "add_a_stop":
          if (RoutingController.get().isNavigating() || RoutingController.get().isPlanning())
          {
            RoutingController.get().addStop(dest);
          }
          else
          {
            startNavigationToDestination(dest);
          }
          break;

        case "directions":
          RoutingController.get().prepare(null, dest);
          break;

        case "navigation":
        default:
          startNavigationToDestination(dest);
          break;
        }

        return true;
      } catch (Exception e)
      {
        return false;
      }
    }

    /**
     * Start navigation to destination
     */
    private static void startNavigationToDestination(@NonNull MapObject dest)
    {
      try
      {
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        RoutingController.get().prepare(null, dest);
      } catch (Exception e)
      {
        Logger.e(TAG, "Failed to start navigation", e);
      }
    }

    private static DestinationData parseDestination(@NonNull Uri uri)
    {
      DestinationData dst = new DestinationData();

      try
      {
        final String ssp = uri.getSchemeSpecificPart();
        if (ssp != null)
        {
          int qIndex = ssp.indexOf('?');
          String beforeQuery = (qIndex >= 0 ? ssp.substring(0, qIndex) : ssp);
          String[] parts = beforeQuery.split(",");
          if (parts.length >= 2)
          {
            try
            {
              dst.lat = Double.parseDouble(parts[0]);
              dst.lon = Double.parseDouble(parts[1]);
            } catch (NumberFormatException ignore)
            {
              dst.lat = 0.0;
              dst.lon = 0.0;
            }
          }
        }

        dst.query = uri.getQueryParameter("q");
      } catch (Exception e)
      {
        Logger.e(TAG, "Failed to parse destination", e);
      }

      return dst;
    }

    private static Router parseRoutingMode(@NonNull Uri uri)
    {
      try
      {
        final String mode = uri.getQueryParameter("mode");
        if (mode == null)
          return Router.Vehicle;

        return switch (mode)
        {
          case "w" -> Router.Pedestrian;
          case "b" -> Router.Bicycle;
          case "r" -> Router.Transit;
          default -> Router.Vehicle;
        };
      } catch (Exception e)
      {
        return Router.Vehicle;
      }
    }

    /**
     * Apply avoid options: t=tolls, f=ferries, h=highways
     */
    private static void applyAvoidOptions(@NonNull Uri uri)
    {
      try
      {
        final String avoid = uri.getQueryParameter("avoid");
        if (avoid == null || avoid.isEmpty())
          return;

        if (avoid.indexOf('t') >= 0)
          RoutingOptions.addOption(RoadType.Toll);
        if (avoid.indexOf('f') >= 0)
          RoutingOptions.addOption(RoadType.Ferry);
        if (avoid.indexOf('h') >= 0)
          RoutingOptions.addOption(RoadType.Motorway);
      } catch (Exception e)
      {
        Logger.e(TAG, "Failed to apply avoid options", e);
      }
    }

    private static boolean handleAssistantSearchIntent(@NonNull Uri uri, @NonNull MwmActivity activity)
    {
      try
      {
        final String q = uri.getQueryParameter("q");
        if (q == null || q.trim().isEmpty())
          return false;

        DestinationData dst = parseDestination(uri);
        SearchEngine.INSTANCE.cancelInteractiveSearch();

        final boolean isSearchOnMap = true;
        final String locale = "";

        if (dst.hasLatLon() && !(dst.lat == 0.0 && dst.lon == 0.0))
        {
          Framework.nativeStopLocationFollow();
          Framework.nativeSetViewportCenter(dst.lat, dst.lon, SEARCH_IN_VIEWPORT_ZOOM);
          Framework.nativeSetSearchViewport(dst.lat, dst.lon, SEARCH_IN_VIEWPORT_ZOOM);
        }

        SearchActivity.start(activity, q, locale, isSearchOnMap);
        return true;
      } catch (Exception e)
      {
        return false;
      }
    }

    /**
     * Parse action parameter from geo.action: URIs (handles both hierarchical and non-hierarchical URIs)
     */
    private static String parseActionParameter(@NonNull Uri uri)
    {
      try
      {
        if (uri.isHierarchical())
        {
          return uri.getQueryParameter("act");
        }
        final String ssp = uri.getSchemeSpecificPart();
        if (ssp != null && ssp.startsWith("?"))
        {
          final String query = ssp.substring(1); // Remove leading '?'
          final String[] params = query.split("&");
          for (String param : params)
          {
            final String[] keyValue = param.split("=", 2);
            if (keyValue.length == 2 && "act".equals(keyValue[0]))
            {
              return Uri.decode(keyValue[1]);
            }
          }
        }
        return null;
      } catch (Exception e)
      {
        Logger.e(TAG, "Failed to parse action parameter", e);
        return null;
      }
    }

    private static boolean handleCustomAction(@NonNull Intent intent, @NonNull Uri uri,
                                              @NonNull MwmActivity activity)
    {
      try
      {
        final String act = parseActionParameter(uri);
        if (act == null)
          return false;

        switch (act)
        {
        case "exit_navigation":
          RoutingController.get().cancel();
          return true;

        case "resume_navigation":
          if (RoutingController.get().isBuilt() && !RoutingController.get().isNavigating())
            RoutingController.get().start();
          return true;

        case "route_overview":
          if (RoutingController.get().isNavigating())
            RoutingController.get().resetToPlanningStateIfNavigating();
          return true;

        case "show_directions_list":
          if (RoutingController.get().isBuilt())
          {
            RoutingController.get().resetToPlanningStateIfNavigating();
          }
          return true;

        case "mute":
          TtsPlayer.setEnabled(false);
          return true;

        case "unmute":
          TtsPlayer.setEnabled(true);
          return true;

        case "avoid_tolls":
          RoutingOptions.addOption(RoadType.Toll);
          rebuildRouteIfNeeded();
          return true;

        case "avoid_ferries":
          RoutingOptions.addOption(RoadType.Ferry);
          rebuildRouteIfNeeded();
          return true;

        case "avoid_highways":
          RoutingOptions.addOption(RoadType.Motorway);
          rebuildRouteIfNeeded();
          return true;

        case "allow_tolls":
          RoutingOptions.removeOption(RoadType.Toll);
          rebuildRouteIfNeeded();
          return true;

        case "allow_ferries":
          RoutingOptions.removeOption(RoadType.Ferry);
          rebuildRouteIfNeeded();
          return true;

        case "allow_highways":
          RoutingOptions.removeOption(RoadType.Motorway);
          rebuildRouteIfNeeded();
          return true;

        case "show_traffic":
        case "hide_traffic":
          Logger.d(TAG, "Traffic actions not supported");
          return false;

        case "follow_mode":
          Framework.nativeFollowRoute();
          return true;

        case "report_crash":
        case "report_hazard":
        case "report_police":
        case "report_road_closure":
        case "report_traffic":
          Logger.d(TAG, "Report actions not supported");
          return false;

        default:
          Logger.d(TAG, "Unknown action: " + act);
          return false;
        }
      } catch (Exception e)
      {
        Logger.e(TAG, "Failed to handle custom action", e);
        return false;
      }
    }

    /**
     * Rebuild route when options change
     */
    private static void rebuildRouteIfNeeded()
    {
      try
      {
        final RoutingController controller = RoutingController.get();
        if (controller.isPlanning() && controller.getStartPoint() != null && controller.getEndPoint() != null)
        {
          controller.rebuildLastRoute();
        }
      } catch (Exception e)
      {
        Logger.e(TAG, "Failed to rebuild route", e);
      }
    }
  }
}
