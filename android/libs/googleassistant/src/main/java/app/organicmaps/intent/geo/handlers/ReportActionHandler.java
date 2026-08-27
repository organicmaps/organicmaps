package app.organicmaps.intent.geo.handlers;

import androidx.annotation.Nullable;
import app.organicmaps.intent.geo.action.AccidentType;
import app.organicmaps.intent.geo.action.HazardType;
import app.organicmaps.intent.geo.action.LocationOnRoad;
import app.organicmaps.intent.geo.action.RoadClosureType;
import app.organicmaps.intent.geo.action.RoadDirection;
import app.organicmaps.intent.geo.action.TrafficType;
import app.organicmaps.sdk.util.log.Logger;

public interface ReportActionHandler
{
  String TAG = ReportActionHandler.class.getSimpleName();

  /**
   * Submits a crash report.
   *
   * @param accidentType crash type, or {@code null} if not specified.
   * @param roadDirection road direction, or {@code null} if not specified.
   */
  default void reportCrash(@Nullable AccidentType accidentType, @Nullable RoadDirection roadDirection)
  {
    Logger.d(TAG, "Not implemented: report crash accidentType=" + accidentType + ", roadDirection=" + roadDirection);
  }

  /**
   * Submits a hazard report.
   *
   * @param hazardType hazard type, or {@code null} if not specified.
   * @param locationOnRoad hazard location on road, or {@code null} if not specified.
   * @param roadDirection road direction, or {@code null} if not specified.
   */
  default void reportHazard(@Nullable HazardType hazardType, @Nullable LocationOnRoad locationOnRoad,
                            @Nullable RoadDirection roadDirection)
  {
    Logger.d(TAG, "Not implemented: report hazard hazardType=" + hazardType + ", locationOnRoad=" + locationOnRoad
                      + ", roadDirection=" + roadDirection);
  }

  /**
   * Submits a police report.
   *
   * @param roadDirection road direction, or {@code null} if not specified.
   */
  default void reportPolice(@Nullable RoadDirection roadDirection)
  {
    Logger.d(TAG, "Not implemented: report police roadDirection=" + roadDirection);
  }

  /**
   * Submits a road closure report.
   *
   * @param roadClosureType closure type, or {@code null} if not specified.
   */
  default void reportRoadClosure(@Nullable RoadClosureType roadClosureType)
  {
    Logger.d(TAG, "Not implemented: report road closure roadClosureType=" + roadClosureType);
  }

  /**
   * Submits a traffic report.
   *
   * @param trafficType traffic type, or {@code null} if not specified.
   * @param roadDirection road direction, or {@code null} if not specified.
   */
  default void reportTraffic(@Nullable TrafficType trafficType, @Nullable RoadDirection roadDirection)
  {
    Logger.d(TAG, "Not implemented: report traffic trafficType=" + trafficType + ", roadDirection=" + roadDirection);
  }
}
