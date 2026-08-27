package app.organicmaps.intent.geo;

import android.content.Intent;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import app.organicmaps.intent.IntentProcessor;
import app.organicmaps.intent.geo.navigation.GeoIntent;
import app.organicmaps.sdk.util.log.Logger;

public class GeoIntentProcessor
{
  private static final String TAG = GeoIntentProcessor.class.getSimpleName();

  private final IntentProcessor mIntentProcessor;

  public GeoIntentProcessor(@NonNull IntentProcessor intentProcessor)
  {
    mIntentProcessor = intentProcessor;
  }

  public boolean processIntent(@NonNull Intent intent)
  {
    if (GeoNavigationIntent.isNavigationIntent(intent))
      processNavigationIntent(intent);
    else if (GeoViewIntent.isSearchIntent(intent))
      processSearchIntent(intent);
    else if (GeoActionIntent.isActionIntent(intent))
      processActionIntent(intent);
    else
      return false;
    return true;
  }

  private void processNavigationIntent(@NonNull Intent intent)
  {
    final GeoNavigationIntent navigationIntent = GeoNavigationIntent.fromIntent(intent);
    final double lat = navigationIntent.getLat();
    final double lon = navigationIntent.getLon();
    final String query = navigationIntent.getQuery();
    final boolean areCoordinatesValid = !Double.isNaN(lat);
    final boolean hasQuery = !TextUtils.isEmpty(query);
    final GeoIntent geoIntent = navigationIntent.getIntent();
    if (geoIntent == GeoIntent.Navigation)
    {
      if (areCoordinatesValid && hasQuery)
        mIntentProcessor.navigate(lat, lon, query, navigationIntent.getAvoidList(), navigationIntent.getTravelMode());
      else if (areCoordinatesValid)
        mIntentProcessor.navigate(lat, lon, navigationIntent.getAvoidList(), navigationIntent.getTravelMode());
      else if (hasQuery)
        mIntentProcessor.navigate(query, navigationIntent.getAvoidList(), navigationIntent.getTravelMode());
    }
    else if (geoIntent == GeoIntent.Directions)
    {
      if (areCoordinatesValid && hasQuery)
        mIntentProcessor.showDirections(lat, lon, query, navigationIntent.getAvoidList(),
                                        navigationIntent.getTravelMode());
      else if (areCoordinatesValid)
        mIntentProcessor.showDirections(lat, lon, navigationIntent.getAvoidList(), navigationIntent.getTravelMode());
      else if (hasQuery)
        mIntentProcessor.showDirections(query, navigationIntent.getAvoidList(), navigationIntent.getTravelMode());
    }
    else if (geoIntent == GeoIntent.AddStop)
    {
      if (areCoordinatesValid && hasQuery)
        mIntentProcessor.addStop(lat, lon, query);
      else if (areCoordinatesValid)
        mIntentProcessor.addStop(lat, lon);
      else if (hasQuery)
        mIntentProcessor.addStop(query);
    }
  }

  private void processSearchIntent(@NonNull Intent intent)
  {
    final GeoViewIntent searchIntent = GeoViewIntent.fromIntent(intent);
    final String query = searchIntent.getQuery();
    final double lat = searchIntent.getLat();
    final double lon = searchIntent.getLon();
    if (!TextUtils.isEmpty(query))
    {
      if (!Double.isNaN(lat))
        mIntentProcessor.search(lat, lon, query);
      else
        mIntentProcessor.search(query);
    }
  }

  private void processActionIntent(@NonNull Intent intent)
  {
    final GeoActionIntent actionIntent = GeoActionIntent.fromIntent(intent);
    switch (actionIntent.getActionType())
    {
    case AllowFerries -> mIntentProcessor.processFerriesAction(true);
    case AllowHighways -> mIntentProcessor.processHighwaysAction(true);
    case AllowTolls -> mIntentProcessor.processTollsAction(true);
    case AvoidFerries -> mIntentProcessor.processFerriesAction(false);
    case AvoidHighways -> mIntentProcessor.processHighwaysAction(false);
    case AvoidTolls -> mIntentProcessor.processTollsAction(false);
    case DistanceToDestination -> mIntentProcessor.speakDistanceToDestination();
    case DistanceToNextTurn -> mIntentProcessor.speakDistanceToNextTurn();
    case Eta -> mIntentProcessor.speakEta();
    case ExitNavigation -> mIntentProcessor.exitNavigation();
    case FollowMode -> mIntentProcessor.changeMapViewToFollowMode();
    case GoBack -> mIntentProcessor.goBack();
    case HideSatellite -> mIntentProcessor.processShowSatelliteAction(false);
    case HideTraffic -> mIntentProcessor.processShowTrafficAction(false);
    case Mute -> mIntentProcessor.processVoiceGuidanceAction(false);
    case QueryCurrentRoad -> mIntentProcessor.speakCurrentRoad();
    case QueryDestination -> mIntentProcessor.speakDestination();
    case QueryNextTurn -> mIntentProcessor.speakNextTurn();
    case ReportCrash -> mIntentProcessor.reportCrash(actionIntent.getAccidentType(), actionIntent.getRoadDirection());
    case ReportHazard ->
      mIntentProcessor.reportHazard(actionIntent.getHazardType(), actionIntent.getLocationOnRoad(),
                                    actionIntent.getRoadDirection());
    case ReportPolice -> mIntentProcessor.reportPolice(actionIntent.getRoadDirection());
    case ReportRoadClosure -> mIntentProcessor.reportRoadClosure(actionIntent.getRoadClosureType());
    case ReportTraffic ->
      mIntentProcessor.reportTraffic(actionIntent.getTrafficType(), actionIntent.getRoadDirection());
    case RouteOverview -> mIntentProcessor.showRouteOverview();
    case ShowAlternativeRoutes, ShowAlternates -> mIntentProcessor.showAlternativeRoutes();
    case ShowDirectionsList -> mIntentProcessor.showDirectionsList();
    case ShowSatellite -> mIntentProcessor.processShowSatelliteAction(true);
    case ShowTraffic -> mIntentProcessor.processShowTrafficAction(true);
    case TimeToDestination -> mIntentProcessor.speakTimeToDestination();
    case TimeToNextTurn -> mIntentProcessor.speakTimeToNextTurn();
    case Unmute -> mIntentProcessor.processVoiceGuidanceAction(true);
    case MyLocation -> mIntentProcessor.showMyLocation();
    case ShowMap -> mIntentProcessor.showMap();
    case TrafficReport -> mIntentProcessor.speakTrafficReport();
    case ClearSearchResults -> mIntentProcessor.clearSearchResults();
    case SelectSearchResult -> mIntentProcessor.selectSearchResult(actionIntent.getSearchId());
    case ApplyElectricVehicleConnectorFilter -> mIntentProcessor.enableElectricVehicleConnectorFilter(true);
    case RemoveElectricVehicleConnectorFilter -> mIntentProcessor.enableElectricVehicleConnectorFilter(false);
    case ApplyElectricVehiclePaymentFilter -> mIntentProcessor.enableElectricVehiclePaymentFilter(true);
    case RemoveElectricVehiclePaymentFilter -> mIntentProcessor.enableElectricVehiclePaymentFilter(false);
    case ApplyElectricVehicleFastChargingFilter -> mIntentProcessor.enableElectricVehicleFastChargingFilter(true);
    case RemoveElectricVehicleFastChargingFilter -> mIntentProcessor.enableElectricVehicleFastChargingFilter(false);
    default -> Logger.w(TAG, "Unknown action type: " + actionIntent.getActionType());
    }
  }
}
