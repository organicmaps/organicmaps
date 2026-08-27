package app.organicmaps.intent.geo.action;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;

/// Defines the type of custom action a user wants to perform.
public enum ActionType
{
  /// Change route preference to allow ferries.
  AllowFerries("allow_ferries"),
  /// Change route preference to allow highways.
  AllowHighways("allow_highways"),
  /// Change route preference to allow tolls.
  AllowTolls("allow_tolls"),
  /// Change route preference to avoid ferries.
  AvoidFerries("avoid_ferries"),
  /// Change route preference to avoid highways.
  AvoidHighways("avoid_highways"),
  /// Change route preference to avoid tolls.
  AvoidTolls("avoid_tolls"),
  /// Show distance to the destination.
  DistanceToDestination("distance_to_destination"),
  /// Show distance to the next turn.
  DistanceToNextTurn("distance_to_next_turn"),
  /// Show ETA to the destination.
  Eta("eta"),
  /// Exit or cancel navigation.
  ExitNavigation("exit_navigation"),
  /// Change map view to follow mode.
  FollowMode("follow_mode"),
  /// Go back to previous map action.
  GoBack("go_back"),
  /// Change map setting to hide satellite info.
  HideSatellite("hide_satellite"),
  /// Change map setting to hide traffic info.
  HideTraffic("hide_traffic"),
  /// Mute voice guidance.
  Mute("mute"),
  /// Show what is the current road the user is on.
  QueryCurrentRoad("query_current_road"),
  /// Show what is the destination.
  QueryDestination("query_destination"),
  /// Show what is the next turn (while navigation). Voice
  QueryNextTurn("query_next_turn"),
  /// Report crashes.
  ReportCrash("report_crash"),
  /// Report hazards.
  ReportHazard("report_hazard"),
  /// Report police.
  ReportPolice("report_police"),
  /// Report road closures.
  ReportRoadClosure("report_road_closure"),
  /// Report traffic.
  ReportTraffic("report_traffic"),
  /// Show route overview (while navigation).
  RouteOverview("route_overview"),
  /// Show alternative routes (while navigation).
  ShowAlternativeRoutes("show_alternative_routes"),
  /// Show alternative routes (while navigation).
  ShowAlternates("show_alternates"),
  /// Show turn-by-turn instructions.
  ShowDirectionsList("show_directions_list"),
  /// Show satellite info on the map.
  ShowSatellite("show_satellite"),
  /// Show traffic on the map.
  ShowTraffic("show_traffic"),
  /// Show ETA to destination.
  TimeToDestination("time_to_destination"),
  /// Show ETA to next turn.
  TimeToNextTurn("time_to_next_turn"),
  /// Unmute voice guidance.
  Unmute("unmute"),
  /// Recenter the map on the user's location.
  MyLocation("my_location"),
  /// Closes all open cards and go back to the map.
  ShowMap("show_map"),
  /// Speak the traffic report.
  TrafficReport("traffic_report"),
  /// Close the search results screen (if it's open).
  ClearSearchResults("clear_search_results"),
  /// Applies connector type filter for electric vehicle charging station search results.
  ApplyElectricVehicleConnectorFilter("apply_electric_vehicle_connector_filter"),
  /// Removes connector type filter for electric vehicle charging station search results.
  RemoveElectricVehicleConnectorFilter("remove_electric_vehicle_connector_filter"),
  /// Applies payment filter for electric vehicle charging station search results.
  ApplyElectricVehiclePaymentFilter("apply_electric_vehicle_payment_filter"),
  /// Removes payment filter for electric vehicle charging station search results.
  RemoveElectricVehiclePaymentFilter("remove_electric_vehicle_payment_filter"),
  /// Applies fast charging filter for electric vehicle charging station search results.
  ApplyElectricVehicleFastChargingFilter("apply_electric_vehicle_fast_charging_filter"),
  /// Removes fast charging filter for electric vehicle charging station search results.
  RemoveElectricVehicleFastChargingFilter("remove_electric_vehicle_fast_charging_filter"),
  /// If search results are shown on screen, this action starts navigation to
  /// the `n`th result based on the ID parameter provided.
  /// @note the index is 0-based (that is, `geo.action:?act=select_search_result&id=0` will select the first result in
  /// the list).
  SelectSearchResult("select_search_result");

  private static final String TAG = ActionType.class.getSimpleName();

  @NonNull
  private final String mRaw;

  ActionType(@NonNull String raw)
  {
    mRaw = raw;
  }

  @NonNull
  public static String getKey()
  {
    return "act";
  }

  @Nullable
  public static ActionType fromRaw(@Nullable String raw)
  {
    for (ActionType actionType : values())
    {
      if (actionType.mRaw.equals(raw))
        return actionType;
    }
    Logger.w(TAG, "Unknown action type: " + raw);
    return null;
  }
}
