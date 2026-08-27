package app.organicmaps.intent.geo

import app.organicmaps.intent.geo.action.AccidentType
import app.organicmaps.intent.geo.action.HazardType
import app.organicmaps.intent.geo.action.LocationOnRoad
import app.organicmaps.intent.geo.action.RoadClosureType
import app.organicmaps.intent.geo.action.RoadDirection
import app.organicmaps.intent.geo.action.TrafficType
import app.organicmaps.intent.geo.handlers.AvoidActionHandler
import app.organicmaps.intent.geo.handlers.ControlActionHandler
import app.organicmaps.intent.geo.handlers.NavigationActionHandler
import app.organicmaps.intent.geo.handlers.ReportActionHandler
import app.organicmaps.intent.geo.handlers.SearchActionHandler
import app.organicmaps.intent.geo.handlers.VoiceActionHandler
import app.organicmaps.intent.geo.navigation.Avoid
import app.organicmaps.intent.geo.navigation.TravelMode

class FakeAvoidActionHandler : AvoidActionHandler {
    val events = mutableListOf<String>()

    override fun processFerriesAction(allow: Boolean) {
        events.add("ferries:$allow")
    }

    override fun processHighwaysAction(allow: Boolean) {
        events.add("highways:$allow")
    }

    override fun processTollsAction(allow: Boolean) {
        events.add("tolls:$allow")
    }
}

class FakeControlActionHandler : ControlActionHandler {
    val events = mutableListOf<String>()

    override fun exitNavigation() {
        events.add("exitNavigation")
    }

    override fun changeMapViewToFollowMode() {
        events.add("followMode")
    }

    override fun goBack() {
        events.add("goBack")
    }

    override fun processShowSatelliteAction(show: Boolean) {
        events.add("satellite:$show")
    }

    override fun processShowTrafficAction(show: Boolean) {
        events.add("traffic:$show")
    }

    override fun showRouteOverview() {
        events.add("routeOverview")
    }

    override fun showAlternativeRoutes() {
        events.add("alternativeRoutes")
    }

    override fun showDirectionsList() {
        events.add("directionsList")
    }

    override fun showMyLocation() {
        events.add("myLocation")
    }

    override fun showMap() {
        events.add("showMap")
    }
}

class FakeNavigationActionHandler : NavigationActionHandler {
    val events = mutableListOf<String>()

    override fun navigate(lat: Double, lon: Double, query: String?, avoid: List<Avoid>, travelMode: TravelMode?) {
        events.add("navigate:$lat,$lon,$query,$avoid,$travelMode")
    }

    override fun addStop(lat: Double, lon: Double, query: String?) {
        events.add("addStop:$lat,$lon,$query")
    }

    override fun showDirections(
        lat: Double,
        lon: Double,
        query: String?,
        avoid: List<Avoid>?,
        travelMode: TravelMode?,
    ) {
        events.add("directions:$lat,$lon,$query,$avoid,$travelMode")
    }
}

class FakeReportActionHandler : ReportActionHandler {
    val events = mutableListOf<String>()

    override fun reportCrash(accidentType: AccidentType?, roadDirection: RoadDirection?) {
        events.add("crash:$accidentType,$roadDirection")
    }

    override fun reportHazard(hazardType: HazardType?, locationOnRoad: LocationOnRoad?, roadDirection: RoadDirection?) {
        events.add("hazard:$hazardType,$locationOnRoad,$roadDirection")
    }

    override fun reportPolice(roadDirection: RoadDirection?) {
        events.add("police:$roadDirection")
    }

    override fun reportRoadClosure(roadClosureType: RoadClosureType?) {
        events.add("roadClosure:$roadClosureType")
    }

    override fun reportTraffic(trafficType: TrafficType?, roadDirection: RoadDirection?) {
        events.add("traffic:$trafficType,$roadDirection")
    }
}

class FakeSearchActionHandler : SearchActionHandler {
    val events = mutableListOf<String>()

    override fun search(lat: Double, lon: Double, query: String) {
        events.add("search:$lat,$lon,$query")
    }

    override fun clearSearchResults() {
        events.add("clear")
    }

    override fun selectSearchResult(index: Int) {
        events.add("select:$index")
    }

    override fun enableElectricVehicleConnectorFilter(enable: Boolean) {
        events.add("connector:$enable")
    }

    override fun enableElectricVehiclePaymentFilter(enable: Boolean) {
        events.add("payment:$enable")
    }

    override fun enableElectricVehicleFastChargingFilter(enable: Boolean) {
        events.add("fast:$enable")
    }
}

class FakeVoiceActionHandler : VoiceActionHandler {
    val events = mutableListOf<String>()

    override fun processVoiceGuidanceAction(enable: Boolean) {
        events.add("voice:$enable")
    }

    override fun speakTrafficReport() {
        events.add("trafficReport")
    }

    override fun speakCurrentRoad() {
        events.add("currentRoad")
    }

    override fun speakDestination() {
        events.add("destination")
    }

    override fun speakEta() {
        events.add("eta")
    }

    override fun speakDistanceToDestination() {
        events.add("distanceToDestination")
    }

    override fun speakTimeToDestination() {
        events.add("timeToDestination")
    }

    override fun speakNextTurn() {
        events.add("nextTurn")
    }

    override fun speakDistanceToNextTurn() {
        events.add("distanceToNextTurn")
    }

    override fun speakTimeToNextTurn() {
        events.add("timeToNextTurn")
    }
}
