package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.enums.StringEnum

/**
 * @see <a href="https://developer.android.com/develop/devices/assistant/intents-assistant-nav-app.action_type">ActionType</a>
 *
 * @see <a href="https://developer.android.com/training/cars/platforms/automotive-os/android-intents-automotive.action-intents">Android Automotive Action Intents</a>
 */
enum class ActionType(override val raw: String) : StringEnum {
    AllowFerries("allow_ferries"),
    AllowHighways("allow_highways"),
    AllowTolls("allow_tolls"),
    AvoidFerries("avoid_ferries"),
    AvoidHighways("avoid_highways"),
    AvoidTolls("avoid_tolls"),
    DistanceToDestination("distance_to_destination"),
    DistanceToNextTurn("distance_to_next_turn"),
    Eta("eta"),
    ExitNavigation("exit_navigation"),
    FollowMode("follow_mode"),
    GoBack("go_back"),
    HideSatellite("hide_satellite"),
    HideTraffic("hide_traffic"),
    Mute("mute"),
    QueryCurrentRoad("query_current_road"),
    QueryDestination("query_destination"),
    QueryNextTurn("query_next_turn"),
    ReportCrash("report_crash"),
    ReportHazard("report_hazard"),
    ReportPolice("report_police"),
    ReportRoadClosure("report_road_closure"),
    ReportTraffic("report_traffic"),
    RouteOverview("route_overview"),
    ShowAlternativeRoutes("show_alternative_routes"),
    ShowAlternates("show_alternates"),
    ShowDirectionsList("show_directions_list"),
    ShowSatellite("show_satellite"),
    ShowTraffic("show_traffic"),
    TimeToDestination("time_to_destination"),
    TimeToNextTurn("time_to_next_turn"),
    Unmute("unmute"),
    MyLocation("my_location"),
    ShowMap("show_map"),
    TrafficReport("traffic_report"),
    ClearSearchResults("clear_search_results"),
    ApplyElectricVehicleConnectorFilter("apply_electric_vehicle_connector_filter"),
    RemoveElectricVehicleConnectorFilter("remove_electric_vehicle_connector_filter"),
    ApplyElectricVehiclePaymentFilter("apply_electric_vehicle_payment_filter"),
    RemoveElectricVehiclePaymentFilter("remove_electric_vehicle_payment_filter"),
    ApplyElectricVehicleFastChargingFilter("apply_electric_vehicle_fast_charging_filter"),
    RemoveElectricVehicleFastChargingFilter("remove_electric_vehicle_fast_charging_filter"),
    SelectSearchResult("select_search_result"),
    ;

    companion object {
        const val KEY: String = "act"
    }
}
