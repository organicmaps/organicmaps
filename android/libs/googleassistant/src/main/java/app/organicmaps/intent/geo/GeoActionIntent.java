package app.organicmaps.intent.geo;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.intent.IntentUtils;
import app.organicmaps.intent.geo.action.AccidentType;
import app.organicmaps.intent.geo.action.ActionType;
import app.organicmaps.intent.geo.action.HazardType;
import app.organicmaps.intent.geo.action.LocationOnRoad;
import app.organicmaps.intent.geo.action.RoadClosureType;
import app.organicmaps.intent.geo.action.RoadDirection;
import app.organicmaps.intent.geo.action.TrafficType;
import app.organicmaps.sdk.util.Assert;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

/// Use a custom intent for custom actions like reporting accidents and ending navigation.
/// The main action type is defined by the act query parameter.
/// You can set additional parameters depending on the action type.
///
/// @see <a
/// href="https://developer.android.com/develop/devices/assistant/intents-assistant-nav-app#custom-action-intent-format">Intent
/// format</a>
/// @see <a
/// href="https://developer.android.com/training/cars/platforms/automotive-os/android-intents-automotive#action-intents">Android
/// automotive intent parameters</a>
public class GeoActionIntent
{
  private static final Set<String> sSupportedSchemes = Set.of("geo.action", "geo.action.offline");
  private static final String SEARCH_ID_KEY = "id";
  private static final String DEFAULT_SEARCH_ID_STR = "-1";

  public static final int DEFAULT_SEARCH_ID = -1;

  @NonNull
  private final ActionType mActionType;

  /// Supported action types:
  /// @see ActionType#ReportCrash
  @Nullable
  private final AccidentType mAccidentType;

  /// Supported action types:
  /// @see ActionType#ReportHazard
  @Nullable
  private final HazardType mHazardType;

  /// Supported action types:
  /// @see ActionType#ReportHazard
  @Nullable
  private final LocationOnRoad mLocationOnRoad;

  /// Supported action types:
  /// @see ActionType#ReportRoadClosure
  @Nullable
  private final RoadClosureType mRoadClosureType;

  /// Supported action types:
  /// @see ActionType#ReportTraffic
  @Nullable
  private final TrafficType mTrafficType;

  /// Supported action types:
  /// @see ActionType#ReportCrash
  /// @see ActionType#ReportHazard
  /// @see ActionType#ReportPolice
  /// @see ActionType#ReportTraffic
  @Nullable
  private final RoadDirection mRoadDirection;

  private final int mSearchId;

  private GeoActionIntent(@NonNull ActionType actionType, @Nullable AccidentType accidentType,
                          @Nullable HazardType hazardType, @Nullable LocationOnRoad locationOnRoad,
                          @Nullable RoadClosureType roadClosureType, @Nullable TrafficType trafficType,
                          @Nullable RoadDirection roadDirection, int searchId)
  {
    mActionType = actionType;
    mAccidentType = accidentType;
    mHazardType = hazardType;
    mLocationOnRoad = locationOnRoad;
    mRoadClosureType = roadClosureType;
    mTrafficType = trafficType;
    mRoadDirection = roadDirection;
    mSearchId = searchId;
  }

  @NonNull
  public ActionType getActionType()
  {
    return mActionType;
  }

  @Nullable
  public AccidentType getAccidentType()
  {
    return mAccidentType;
  }

  @Nullable
  public HazardType getHazardType()
  {
    return mHazardType;
  }

  @Nullable
  public LocationOnRoad getLocationOnRoad()
  {
    return mLocationOnRoad;
  }

  @Nullable
  public RoadClosureType getRoadClosureType()
  {
    return mRoadClosureType;
  }

  @Nullable
  public TrafficType getTrafficType()
  {
    return mTrafficType;
  }

  @Nullable
  public RoadDirection getRoadDirection()
  {
    return mRoadDirection;
  }

  public int getSearchId()
  {
    return mSearchId;
  }

  public static boolean isActionIntent(@NonNull final Intent intent)
  {
    if (!Intent.ACTION_VIEW.equals(intent.getAction()))
      return false;

    return sSupportedSchemes.contains(intent.getScheme());
  }

  @NonNull
  public static GeoActionIntent fromIntent(@NonNull final Intent intent)
  {
    Assert.debug(isActionIntent(intent), "Not an action intent");

    final Map<String, String> queryParams = IntentUtils.getQueryParameters(intent);

    final ActionType actionType = ActionType.fromRaw(queryParams.get(ActionType.getKey()));
    final AccidentType accidentType = AccidentType.fromRaw(queryParams.get(AccidentType.getKey()));
    final HazardType hazardType = HazardType.fromRaw(queryParams.get(HazardType.getKey()));
    final LocationOnRoad locationOnRoad = LocationOnRoad.fromRaw(queryParams.get(LocationOnRoad.getKey()));
    final RoadClosureType roadClosureType = RoadClosureType.fromRaw(queryParams.get(RoadClosureType.getKey()));
    final TrafficType trafficType = TrafficType.fromRaw(queryParams.get(TrafficType.getKey()));
    final RoadDirection roadDirection = RoadDirection.fromRaw(queryParams.get(RoadDirection.getKey()));
    final String searchId = Objects.requireNonNullElse(queryParams.get(SEARCH_ID_KEY), DEFAULT_SEARCH_ID_STR);
    final int id = Integer.parseInt(searchId);

    return new GeoActionIntent(Objects.requireNonNull(actionType), accidentType, hazardType, locationOnRoad,
                               roadClosureType, trafficType, roadDirection, id);
  }
}
