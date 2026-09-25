package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.enums.StringEnum

enum class ActionGroup { Avoid, Control, Voice, Report, Search }

/**
 * @see <a href="https://developer.android.com/develop/devices/assistant/intents-assistant-nav-app.action_type">
 *     ActionType</a>
 *
 * @see <a href="https://developer.android.com/training/cars/platforms/automotive-os/android-intents-automotive.action-intents">
 *     Android Automotive Action Intents</a>
 */
@Suppress("MaxLineLength") // Long @see URLs cannot be wrapped without breaking the link.
enum class ActionType(override val raw: String, val group: ActionGroup) : StringEnum {
    AllowFerries("allow_ferries", ActionGroup.Avoid),
    AllowHighways("allow_highways", ActionGroup.Avoid),
    AllowTolls("allow_tolls", ActionGroup.Avoid),
    AvoidFerries("avoid_ferries", ActionGroup.Avoid),
    AvoidHighways("avoid_highways", ActionGroup.Avoid),
    AvoidTolls("avoid_tolls", ActionGroup.Avoid),
    DistanceToDestination("distance_to_destination", ActionGroup.Voice),
    DistanceToNextTurn("distance_to_next_turn", ActionGroup.Voice),
    Eta("eta", ActionGroup.Voice),
    ExitNavigation("exit_navigation", ActionGroup.Control),
    FollowMode("follow_mode", ActionGroup.Control),
    GoBack("go_back", ActionGroup.Control),
    HideSatellite("hide_satellite", ActionGroup.Control),
    HideTraffic("hide_traffic", ActionGroup.Control),
    Mute("mute", ActionGroup.Voice),
    QueryCurrentRoad("query_current_road", ActionGroup.Voice),
    QueryDestination("query_destination", ActionGroup.Voice),
    QueryNextTurn("query_next_turn", ActionGroup.Voice),
    ReportCrash("report_crash", ActionGroup.Report),
    ReportHazard("report_hazard", ActionGroup.Report),
    ReportPolice("report_police", ActionGroup.Report),
    ReportRoadClosure("report_road_closure", ActionGroup.Report),
    ReportTraffic("report_traffic", ActionGroup.Report),
    RouteOverview("route_overview", ActionGroup.Control),
    ShowAlternativeRoutes("show_alternative_routes", ActionGroup.Control),
    ShowAlternates("show_alternates", ActionGroup.Control),
    ShowDirectionsList("show_directions_list", ActionGroup.Control),
    ShowSatellite("show_satellite", ActionGroup.Control),
    ShowTraffic("show_traffic", ActionGroup.Control),
    TimeToDestination("time_to_destination", ActionGroup.Voice),
    TimeToNextTurn("time_to_next_turn", ActionGroup.Voice),
    Unmute("unmute", ActionGroup.Voice),
    MyLocation("my_location", ActionGroup.Control),
    ShowMap("show_map", ActionGroup.Control),
    TrafficReport("traffic_report", ActionGroup.Voice),
    ClearSearchResults("clear_search_results", ActionGroup.Search),
    ApplyElectricVehicleConnectorFilter("apply_electric_vehicle_connector_filter", ActionGroup.Search),
    RemoveElectricVehicleConnectorFilter("remove_electric_vehicle_connector_filter", ActionGroup.Search),
    ApplyElectricVehiclePaymentFilter("apply_electric_vehicle_payment_filter", ActionGroup.Search),
    RemoveElectricVehiclePaymentFilter("remove_electric_vehicle_payment_filter", ActionGroup.Search),
    ApplyElectricVehicleFastChargingFilter("apply_electric_vehicle_fast_charging_filter", ActionGroup.Search),
    RemoveElectricVehicleFastChargingFilter("remove_electric_vehicle_fast_charging_filter", ActionGroup.Search),
    SelectSearchResult("select_search_result", ActionGroup.Search),
    ;

    companion object {
        const val KEY: String = "act"
    }
}
