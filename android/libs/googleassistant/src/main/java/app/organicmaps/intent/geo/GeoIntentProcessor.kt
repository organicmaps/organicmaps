package app.organicmaps.intent.geo

import android.content.Intent
import app.organicmaps.intent.geo.action.ActionType
import app.organicmaps.intent.geo.handlers.AvoidActionHandler
import app.organicmaps.intent.geo.handlers.ControlActionHandler
import app.organicmaps.intent.geo.handlers.NavigationActionHandler
import app.organicmaps.intent.geo.handlers.ReportActionHandler
import app.organicmaps.intent.geo.handlers.SearchActionHandler
import app.organicmaps.intent.geo.handlers.VoiceActionHandler
import app.organicmaps.intent.geo.navigation.UserIntent

class GeoIntentProcessor(
    private val mAvoidActionHandler: AvoidActionHandler,
    private val mControlActionHandler: ControlActionHandler,
    private val mNavigationActionHandler: NavigationActionHandler,
    private val mReportActionHandler: ReportActionHandler,
    private val mSearchActionHandler: SearchActionHandler,
    private val mVoiceActionHandler: VoiceActionHandler,
) {
    fun processIntent(intent: Intent): Boolean {
        val navigationIntent = GeoNavigationIntent.fromIntent(intent)
        if (navigationIntent != null) {
            processNavigationIntent(navigationIntent)
            return true
        }

        val viewIntent = GeoViewIntent.fromIntent(intent)
        if (viewIntent != null) {
            processViewIntent(viewIntent)
            return true
        }

        val actionIntent = GeoActionIntent.fromIntent(intent)
        if (actionIntent != null) {
            processActionIntent(actionIntent)
            return true
        }

        return false
    }

    private fun processNavigationIntent(navigationIntent: GeoNavigationIntent) {
        val lat = navigationIntent.lat
        val lon = navigationIntent.lon
        val query = navigationIntent.query
        val geoIntent = navigationIntent.intent
        when (geoIntent) {
            UserIntent.Navigation -> mNavigationActionHandler.navigate(
                lat,
                lon,
                query,
                navigationIntent.avoidList,
                navigationIntent.travelMode,
            )

            UserIntent.Directions -> mNavigationActionHandler.showDirections(
                lat,
                lon,
                query,
                navigationIntent.avoidList,
                navigationIntent.travelMode,
            )

            UserIntent.AddStop -> mNavigationActionHandler.addStop(lat, lon, query)
        }
    }

    private fun processViewIntent(viewIntent: GeoViewIntent) {
        val query = viewIntent.query
        val lat = viewIntent.lat
        val lon = viewIntent.lon

        mSearchActionHandler.search(lat, lon, query)
    }

    private fun processActionIntent(actionIntent: GeoActionIntent) {
        when (actionIntent.actionType) {
            ActionType.AllowFerries -> mAvoidActionHandler.processFerriesAction(true)

            ActionType.AllowHighways -> mAvoidActionHandler.processHighwaysAction(true)

            ActionType.AllowTolls -> mAvoidActionHandler.processTollsAction(true)

            ActionType.AvoidFerries -> mAvoidActionHandler.processFerriesAction(false)

            ActionType.AvoidHighways -> mAvoidActionHandler.processHighwaysAction(false)

            ActionType.AvoidTolls -> mAvoidActionHandler.processTollsAction(false)

            ActionType.ExitNavigation -> mControlActionHandler.exitNavigation()

            ActionType.FollowMode -> mControlActionHandler.changeMapViewToFollowMode()

            ActionType.GoBack -> mControlActionHandler.goBack()

            ActionType.HideSatellite -> mControlActionHandler.processShowSatelliteAction(false)

            ActionType.HideTraffic -> mControlActionHandler.processShowTrafficAction(false)

            ActionType.RouteOverview -> mControlActionHandler.showRouteOverview()

            ActionType.ShowAlternativeRoutes, ActionType.ShowAlternates -> mControlActionHandler.showAlternativeRoutes()

            ActionType.ShowDirectionsList -> mControlActionHandler.showDirectionsList()

            ActionType.ShowSatellite -> mControlActionHandler.processShowSatelliteAction(true)

            ActionType.ShowTraffic -> mControlActionHandler.processShowTrafficAction(true)

            ActionType.MyLocation -> mControlActionHandler.showMyLocation()

            ActionType.ShowMap -> mControlActionHandler.showMap()

            ActionType.DistanceToDestination -> mVoiceActionHandler.speakDistanceToDestination()

            ActionType.DistanceToNextTurn -> mVoiceActionHandler.speakDistanceToNextTurn()

            ActionType.Eta -> mVoiceActionHandler.speakEta()

            ActionType.Mute -> mVoiceActionHandler.processVoiceGuidanceAction(false)

            ActionType.QueryCurrentRoad -> mVoiceActionHandler.speakCurrentRoad()

            ActionType.QueryDestination -> mVoiceActionHandler.speakDestination()

            ActionType.QueryNextTurn -> mVoiceActionHandler.speakNextTurn()

            ActionType.TimeToDestination -> mVoiceActionHandler.speakTimeToDestination()

            ActionType.TimeToNextTurn -> mVoiceActionHandler.speakTimeToNextTurn()

            ActionType.Unmute -> mVoiceActionHandler.processVoiceGuidanceAction(true)

            ActionType.TrafficReport -> mVoiceActionHandler.speakTrafficReport()

            ActionType.ReportCrash -> mReportActionHandler.reportCrash(
                actionIntent.accidentType,
                actionIntent.roadDirection,
            )

            ActionType.ReportHazard -> mReportActionHandler.reportHazard(
                actionIntent.hazardType,
                actionIntent.locationOnRoad,
                actionIntent.roadDirection,
            )

            ActionType.ReportPolice -> mReportActionHandler.reportPolice(actionIntent.roadDirection)

            ActionType.ReportRoadClosure -> mReportActionHandler.reportRoadClosure(actionIntent.roadClosureType)

            ActionType.ReportTraffic -> mReportActionHandler.reportTraffic(
                actionIntent.trafficType,
                actionIntent.roadDirection,
            )

            ActionType.ClearSearchResults -> mSearchActionHandler.clearSearchResults()

            ActionType.SelectSearchResult -> mSearchActionHandler.selectSearchResult(actionIntent.searchId)

            ActionType.ApplyElectricVehicleConnectorFilter -> mSearchActionHandler.enableElectricVehicleConnectorFilter(
                true,
            )

            ActionType.RemoveElectricVehicleConnectorFilter ->
                mSearchActionHandler.enableElectricVehicleConnectorFilter(false)

            ActionType.ApplyElectricVehiclePaymentFilter -> mSearchActionHandler.enableElectricVehiclePaymentFilter(
                true,
            )

            ActionType.RemoveElectricVehiclePaymentFilter -> mSearchActionHandler.enableElectricVehiclePaymentFilter(
                false,
            )

            ActionType.ApplyElectricVehicleFastChargingFilter ->
                mSearchActionHandler.enableElectricVehicleFastChargingFilter(true)

            ActionType.RemoveElectricVehicleFastChargingFilter ->
                mSearchActionHandler.enableElectricVehicleFastChargingFilter(false)
        }
    }
}
