package app.organicmaps.intent.geo.handlers

import app.organicmaps.intent.geo.action.AccidentType
import app.organicmaps.intent.geo.action.HazardType
import app.organicmaps.intent.geo.action.LocationOnRoad
import app.organicmaps.intent.geo.action.RoadClosureType
import app.organicmaps.intent.geo.action.RoadDirection
import app.organicmaps.intent.geo.action.TrafficType
import app.organicmaps.sdk.util.log.Logger

class DefaultReportActionHandler : ReportActionHandler {
    override fun reportCrash(accidentType: AccidentType?, roadDirection: RoadDirection?) {
        Logger.d(TAG, "Not implemented: report crash accidentType=$accidentType, roadDirection=$roadDirection")
    }

    override fun reportHazard(hazardType: HazardType?, locationOnRoad: LocationOnRoad?, roadDirection: RoadDirection?) {
        Logger.d(
            TAG,
            "Not implemented: report hazard hazardType=$hazardType, locationOnRoad=$locationOnRoad, roadDirection=$roadDirection",
        )
    }

    override fun reportPolice(roadDirection: RoadDirection?) {
        Logger.d(TAG, "Not implemented: report police roadDirection=$roadDirection")
    }

    override fun reportRoadClosure(roadClosureType: RoadClosureType?) {
        Logger.d(TAG, "Not implemented: report road closure roadClosureType=$roadClosureType")
    }

    override fun reportTraffic(trafficType: TrafficType?, roadDirection: RoadDirection?) {
        Logger.d(TAG, "Not implemented: report traffic trafficType=$trafficType, roadDirection=$roadDirection")
    }

    companion object {
        private val TAG: String = DefaultReportActionHandler::class.java.simpleName
    }
}
