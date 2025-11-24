package app.organicmaps.intent;


import android.content.Intent;
import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.settings.RoadType;
import app.organicmaps.sdk.sound.TtsPlayer;
import app.organicmaps.sdk.util.log.Logger;

public abstract class GoogleAssistantIntentHandler
{
  private static final String TAG = "GAIntentHandler";
  private static final int SEARCH_IN_VIEWPORT_ZOOM = 16;

  public interface SearchHandler
  {
    void handleSearch(@NonNull String query, boolean searchOnMap);
  }

  public boolean handleIntent(@NonNull Intent intent, @NonNull SearchHandler searchHandler)
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
        return handleCustomAction(uri);

      if ("geo".equals(scheme) || "geo.offline".equals(scheme))
      {
        if ("android.intent.action.NAVIGATE".equals(action) || "androidx.car.app.action.NAVIGATE".equals(action))
        {
          return handleNavigationIntent(uri, searchHandler);
        }

        if (Intent.ACTION_VIEW.equals(action))
        {
          final String entry = uri.getQueryParameter("entry");

          if ("assistant".equals(entry))
            return handleAssistantSearchIntent(uri, searchHandler);
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

  private static boolean handleNavigationIntent(@NonNull Uri uri, @NonNull SearchHandler handler)
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
          handler.handleSearch(dst.query, true);
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
    }
    catch (Exception e)
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

  private static boolean handleAssistantSearchIntent(@NonNull Uri uri, @NonNull SearchHandler handler)
  {
    try
    {
      final String query = uri.getQueryParameter("q");
      if (query == null || query.trim().isEmpty())
        return false;

      DestinationData dst = parseDestination(uri);
      SearchEngine.INSTANCE.cancelInteractiveSearch();


      if (dst.hasLatLon() && !(dst.lat == 0.0 && dst.lon == 0.0))
      {
        Framework.nativeStopLocationFollow();
        Framework.nativeSetViewportCenter(dst.lat, dst.lon, SEARCH_IN_VIEWPORT_ZOOM);
        Framework.nativeSetSearchViewport(dst.lat, dst.lon, SEARCH_IN_VIEWPORT_ZOOM);
      }

      handler.handleSearch(query, true);
      return true;
    }
    catch (Exception e)
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

  private static boolean handleCustomAction(@NonNull Uri uri)
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