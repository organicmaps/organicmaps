package app.organicmaps.intent;

import android.content.Intent;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.api.MapRequest;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.api.RequestType;
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

  private static boolean handleNavigationIntent(@NonNull Uri uri, @NonNull Uri parameters,
                                                @NonNull SearchHandler handler)
  {
    final int type = Framework.parseAndSetApiUrl(uri);
    if (type != RequestType.MAP && type != RequestType.SEARCH)
      return false;
    final RoutingController routing = RoutingController.get();
    final double[] point = Framework.nativeGetParsedCenterLatLon();
    final Router router = handler.navigationRouter(parseRoutingMode(parameters.getQueryParameter("mode")));
    if (point == null)
    {
      // Searching during guidance must keep its profile, itinerary and recovery file.
      if (!routing.isNavigating())
      {
        applyAvoidOptions(parameters, router);
        routing.setRouterType(router);
      }
      SearchEngine.INSTANCE.cancelInteractiveSearch();
      handler.handleSearch(Framework.nativeGetParsedSearchRequest().mQuery, true);
      return true;
    }

    final String name = parameters.getQueryParameter("q");
    final MapObject destination =
        MapObject.createMapObject(MapObject.API_POINT, name == null ? "" : name, "", point[0], point[1]);
    if ("add_a_stop".equals(parameters.getQueryParameter("intent")) && (routing.isPlanning() || routing.isNavigating()))
    {
      applyAvoidOptions(parameters, routing.getLastRouterType());
      routing.addStop(destination);
    }
    else
    {
      applyAvoidOptions(parameters, router);
      SearchEngine.INSTANCE.cancelInteractiveSearch();
      handler.handleDestination(destination, router, type == RequestType.MAP);
    }
    return true;
  }

  @NonNull
  private static Router parseRoutingMode(@Nullable String mode)
  {
    if (mode == null)
      return Router.Vehicle;
    return switch (mode)
    {
      case "w" -> Router.Pedestrian;
      case "b" -> Router.Bicycle;
      case "r" -> Router.Transit;
      default -> Router.Vehicle;
    };
  }

  private static void applyAvoidOptions(@NonNull Uri parameters, @NonNull Router router)
  {
    final String avoid = parameters.getQueryParameter("avoid");
    if (avoid == null)
      return;
    if (avoid.indexOf('t') >= 0)
      RoutingOptions.addOption(router, RoadType.Toll);
    if (avoid.indexOf('f') >= 0)
      RoutingOptions.addOption(router, RoadType.Ferry);
    if (avoid.indexOf('h') >= 0)
      RoutingOptions.addOption(router, RoadType.Motorway);
  }

  private static boolean handleAssistantSearch(@NonNull Uri uri, @NonNull Uri parameters,
                                               @NonNull SearchHandler handler)
  {
    final String query = parameters.getQueryParameter("q");
    if (!"assistant".equals(parameters.getQueryParameter("entry")) || query == null || query.trim().isEmpty())
      return false;
    final int type = Framework.parseAndSetApiUrl(uri);
    final double[] point =
        type == RequestType.SEARCH || type == RequestType.MAP ? Framework.nativeGetParsedCenterLatLon() : null;
    SearchEngine.INSTANCE.cancelInteractiveSearch();
    if (point != null)
    {
      Framework.nativeStopLocationFollow();
      Framework.nativeSetViewportCenter(point[0], point[1], SEARCH_IN_VIEWPORT_ZOOM);
      Framework.nativeSetSearchViewport(point[0], point[1], SEARCH_IN_VIEWPORT_ZOOM);
    }
    handler.handleSearch(query, true);
    return true;
  }

  private static boolean handleCustomAction(@Nullable String act)
  {
    if (act == null)
      return false;
    switch (act)
    {
    case "exit_navigation": RoutingController.get().cancel(); return true;

    case "resume_navigation": RoutingController.get().start(); return true;

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

    case "mute": TtsPlayer.setEnabled(false); return true;

    case "unmute": TtsPlayer.setEnabled(true); return true;

    case "avoid_tolls":
      RoutingOptions.addOption(Router.Vehicle, RoadType.Toll);
      onCarOptionsChanged();
      return true;

    case "avoid_ferries":
      RoutingOptions.addOption(Router.Vehicle, RoadType.Ferry);
      onCarOptionsChanged();
      return true;

    case "avoid_highways":
      RoutingOptions.addOption(Router.Vehicle, RoadType.Motorway);
      onCarOptionsChanged();
      return true;

    case "allow_tolls":
      RoutingOptions.removeOption(Router.Vehicle, RoadType.Toll);
      onCarOptionsChanged();
      return true;

    case "allow_ferries":
      RoutingOptions.removeOption(Router.Vehicle, RoadType.Ferry);
      onCarOptionsChanged();
      return true;

    case "allow_highways":
      RoutingOptions.removeOption(Router.Vehicle, RoadType.Motorway);
      onCarOptionsChanged();
      return true;

    case "show_traffic":
    case "hide_traffic": Logger.d(TAG, "Traffic actions not supported"); return false;

    case "follow_mode": Framework.nativeFollowRoute(); return true;

    case "report_crash":
    case "report_hazard":
    case "report_police":
    case "report_road_closure":
    case "report_traffic": Logger.d(TAG, "Report actions not supported"); return false;

    default: Logger.d(TAG, "Unknown action: " + act); return false;
    }
  }

  /**
   * Only driving plans use these options; the controller handles deferred changes and navigation state.
   */
  private static void onCarOptionsChanged()
  {
    final RoutingController controller = RoutingController.get();
    if (controller.getLastRouterType() == Router.Vehicle)
      controller.onRoutingOptionsChanged();
  }

  public boolean handleIntent(@NonNull Intent intent, @NonNull SearchHandler handler)
  {
    final Uri uri = intent.getData();
    final String action = intent.getAction();
    if (uri == null || action == null)
      return false;
    final String scheme = uri.getScheme();
    if ("geo.action".equals(scheme) || "geo.action.offline".equals(scheme))
      return handleCustomAction(MapRequest.parameters(uri).getQueryParameter("act"));
    if (!"geo".equals(scheme) && !"geo.offline".equals(scheme))
      return false;
    final Uri parameters = MapRequest.parameters(uri);
    if (MapRequest.isNavigationAction(action))
      return handleNavigationIntent(uri, parameters, handler);
    return Intent.ACTION_VIEW.equals(action) && handleAssistantSearch(uri, parameters, handler);
  }

  public interface SearchHandler
  {
    void handleSearch(@NonNull String query, boolean searchOnMap);

    default Router navigationRouter(@NonNull Router requested)
    {
      return requested;
    }

    default void handleDestination(@NonNull MapObject destination, @NonNull Router router, boolean parsedMap)
    {
      RoutingController.get().prepare(null, destination, router);
    }
  }
}
