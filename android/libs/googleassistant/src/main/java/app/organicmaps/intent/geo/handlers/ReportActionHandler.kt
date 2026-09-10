package app.organicmaps.intent.geo.handlers

import app.organicmaps.intent.geo.action.AccidentType
import app.organicmaps.intent.geo.action.HazardType
import app.organicmaps.intent.geo.action.LocationOnRoad
import app.organicmaps.intent.geo.action.RoadClosureType
import app.organicmaps.intent.geo.action.RoadDirection
import app.organicmaps.intent.geo.action.TrafficType

interface ReportActionHandler {
    /**
     * Submits a crash report.
     *
     * @param accidentType  crash type, or `null` if not specified.
     * @param roadDirection road direction, or `null` if not specified.
     */
    fun reportCrash(accidentType: AccidentType?, roadDirection: RoadDirection?)

    /**
     * Submits a hazard report.
     *
     * @param hazardType     hazard type, or `null` if not specified.
     * @param locationOnRoad hazard location on road, or `null` if not specified.
     * @param roadDirection  road direction, or `null` if not specified.
     */
    fun reportHazard(hazardType: HazardType?, locationOnRoad: LocationOnRoad?, roadDirection: RoadDirection?)

    /**
     * Submits a police report.
     *
     * @param roadDirection road direction, or `null` if not specified.
     */
    fun reportPolice(roadDirection: RoadDirection?)

    /**
     * Submits a road closure report.
     *
     * @param roadClosureType closure type, or `null` if not specified.
     */
    fun reportRoadClosure(roadClosureType: RoadClosureType?)

    /**
     * Submits a traffic report.
     *
     * @param trafficType   traffic type, or `null` if not specified.
     * @param roadDirection road direction, or `null` if not specified.
     */
    fun reportTraffic(trafficType: TrafficType?, roadDirection: RoadDirection?)
}
