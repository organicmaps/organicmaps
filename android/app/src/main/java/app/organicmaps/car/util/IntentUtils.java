package app.organicmaps.car.util;

import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.ScreenManager;

import app.organicmaps.Framework;
import app.organicmaps.Map;
import app.organicmaps.api.ParsedSearchRequest;
import app.organicmaps.api.ParsingResult;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.screens.hacks.PopToRootHack;
import app.organicmaps.car.screens.search.SearchScreen;
import app.organicmaps.intent.Factory;

public final class IntentUtils
{
  private static final int SEARCH_IN_VIEWPORT_ZOOM = 16;

  // TODO (AndrewShkrob): As of now, the core functionality does not encompass support for the queries 2-3.
  // https://developer.android.com/reference/androidx/car/app/CarContext#startCarApp(android.content.Intent)
  // The data URI scheme must be either a latitude,longitude pair, or a + separated string query as follows:
  //   1) "geo:12.345,14.8767" for a latitude, longitude pair.
  //   2) "geo:0,0?q=123+Main+St,+Seattle,+WA+98101" for an address.
  //   3) "geo:0,0?q=a+place+name" for a place to search for.
  public static void processIntent(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer, @NonNull Intent intent)
  {
    final String action = intent.getAction();
    if (action == null || !intent.getAction().equals(CarContext.ACTION_NAVIGATE))
      return;

    String url = Factory.GeoIntentProcessor.processIntent(intent);
    if (url == null)
      return;

    final ParsingResult result = Framework.nativeParseAndSetApiUrl(url);
    final ScreenManager screenManager = carContext.getCarService(ScreenManager.class);

    screenManager.popToRoot();

    if (result.getUrlType() == ParsingResult.TYPE_INCORRECT)
    {
      Map.showMapForUrl(url);
      return;
    }

    if (!result.isSuccess())
      return;

    switch (result.getUrlType())
    {
    case ParsingResult.TYPE_INCORRECT:
    case ParsingResult.TYPE_MAP:
      Map.showMapForUrl(url);
      return;
    case ParsingResult.TYPE_SEARCH:
      final ParsedSearchRequest request = Framework.nativeGetParsedSearchRequest();
      final double[] latlon = Framework.nativeGetParsedCenterLatLon();
      if (latlon[0] != 0.0 || latlon[1] != 0.0)
      {
        Framework.nativeStopLocationFollow();
        Framework.nativeSetViewportCenter(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
      }
      final SearchScreen.Builder builder = new SearchScreen.Builder(carContext, surfaceRenderer);
      builder.setQuery(request.mQuery);
      if (request.mLocale != null)
        builder.setLocale(request.mLocale);

      screenManager.push(new PopToRootHack.Builder(carContext).setScreenToPush(builder.build()).build());
    default:
    }
  }

  private IntentUtils() {}
}
