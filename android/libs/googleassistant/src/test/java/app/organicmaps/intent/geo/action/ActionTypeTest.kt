package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.StringEnumTestBase
import app.organicmaps.intent.geo.enums.Enums

class ActionTypeTest :
    StringEnumTestBase<ActionType>(
        ActionType.entries,
        TESTED_VALUES,
        "invalid_action",
    ) {
    override fun fromRaw(raw: String?): ActionType? = Enums.fromRaw(raw)

    companion object {
        private val TESTED_VALUES = listOf(
            ActionType.AllowFerries to "allow_ferries",
            ActionType.AllowHighways to "allow_highways",
            ActionType.AllowTolls to "allow_tolls",
            ActionType.AvoidFerries to "avoid_ferries",
            ActionType.AvoidHighways to "avoid_highways",
            ActionType.AvoidTolls to "avoid_tolls",
            ActionType.DistanceToDestination to "distance_to_destination",
            ActionType.DistanceToNextTurn to "distance_to_next_turn",
            ActionType.Eta to "eta",
            ActionType.ExitNavigation to "exit_navigation",
            ActionType.FollowMode to "follow_mode",
            ActionType.GoBack to "go_back",
            ActionType.HideSatellite to "hide_satellite",
            ActionType.HideTraffic to "hide_traffic",
            ActionType.Mute to "mute",
            ActionType.QueryCurrentRoad to "query_current_road",
            ActionType.QueryDestination to "query_destination",
            ActionType.QueryNextTurn to "query_next_turn",
            ActionType.ReportCrash to "report_crash",
            ActionType.ReportHazard to "report_hazard",
            ActionType.ReportPolice to "report_police",
            ActionType.ReportRoadClosure to "report_road_closure",
            ActionType.ReportTraffic to "report_traffic",
            ActionType.RouteOverview to "route_overview",
            ActionType.ShowAlternativeRoutes to "show_alternative_routes",
            ActionType.ShowAlternates to "show_alternates",
            ActionType.ShowDirectionsList to "show_directions_list",
            ActionType.ShowSatellite to "show_satellite",
            ActionType.ShowTraffic to "show_traffic",
            ActionType.TimeToDestination to "time_to_destination",
            ActionType.TimeToNextTurn to "time_to_next_turn",
            ActionType.Unmute to "unmute",
            ActionType.MyLocation to "my_location",
            ActionType.ShowMap to "show_map",
            ActionType.TrafficReport to "traffic_report",
            ActionType.ClearSearchResults to "clear_search_results",
            ActionType.ApplyElectricVehicleConnectorFilter to "apply_electric_vehicle_connector_filter",
            ActionType.RemoveElectricVehicleConnectorFilter to "remove_electric_vehicle_connector_filter",
            ActionType.ApplyElectricVehiclePaymentFilter to "apply_electric_vehicle_payment_filter",
            ActionType.RemoveElectricVehiclePaymentFilter to "remove_electric_vehicle_payment_filter",
            ActionType.ApplyElectricVehicleFastChargingFilter to "apply_electric_vehicle_fast_charging_filter",
            ActionType.RemoveElectricVehicleFastChargingFilter to "remove_electric_vehicle_fast_charging_filter",
            ActionType.SelectSearchResult to "select_search_result",
        )
    }
}
