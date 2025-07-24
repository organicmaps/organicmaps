package app.organicmaps.car.util;

import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Intent;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.notification.CarPendingIntent;
import app.organicmaps.MwmApplication;
import app.organicmaps.api.Const;
import app.organicmaps.car.CarAppService;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.car.screens.search.SearchScreen;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.api.ParsedSearchRequest;
import app.organicmaps.sdk.api.RequestType;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.log.Logger;

public final class IntentUtils
{
  private static final String TAG = IntentUtils.class.getSimpleName();

  private static final int SEARCH_IN_VIEWPORT_ZOOM = 16;

  public static void processIntent(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer,
                                   @NonNull Intent intent)
  {
    final String action = intent.getAction();
    if (CarContext.ACTION_NAVIGATE.equals(action))
      IntentUtils.processNavigationIntent(carContext, surfaceRenderer, intent);
    else if (Intent.ACTION_VIEW.equals(action))
      processViewIntent(carContext, intent);
  }

  @NonNull
  public static PendingIntent createSearchIntent(@NonNull CarContext context, @NonNull String query)
  {
    final String uri = "geo:0,0?q=" + query.replace(" ", "+");
    final ComponentName component = new ComponentName(context, CarAppService.class);
    final Intent intent = new Intent().setComponent(component).setData(Uri.parse(uri));
    return CarPendingIntent.getCarApp(context, 0, intent, 0);
  }

  // https://developer.android.com/reference/androidx/car/app/CarContext#startCarApp(android.content.Intent)
  private static void processNavigationIntent(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer,
                                              @NonNull Intent intent)
  {
    // TODO (AndrewShkrob): This logic will need to be revised when we introduce support for adding stops during
    // navigation or route planning. Skip navigation intents during navigation
    if (RoutingController.get().isNavigating())
      return;

    final Uri uri = intent.getData();
    if (uri == null)
      return;

    final ScreenManager screenManager = carContext.getCarService(ScreenManager.class);
    switch (Framework.nativeParseAndSetApiUrl(uri.toString()))
    {
    case RequestType.INCORRECT: return;
    case RequestType.MAP:
      screenManager.popToRoot();
      Map.executeMapApiRequest();
      return;
    case RequestType.SEARCH:
      screenManager.popToRoot();
      final ParsedSearchRequest request = Framework.nativeGetParsedSearchRequest();
      final double[] latlon = Framework.nativeGetParsedCenterLatLon();
      if (latlon != null)
      {
        Framework.nativeStopLocationFollow();
        Framework.nativeSetViewportCenter(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
        // We need to update viewport for search api manually because of drape engine
        // will not notify subscribers when search activity is shown.
        if (!request.mIsSearchOnMap)
          Framework.nativeSetSearchViewport(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
      }
      final SearchScreen.Builder builder = new SearchScreen.Builder(carContext, surfaceRenderer);
      builder.setQuery(request.mQuery);
      if (request.mLocale != null)
        builder.setLocale(request.mLocale);

      screenManager.popToRoot();
      screenManager.push(builder.build());
      return;
    case RequestType.ROUTE: Logger.e(TAG, "Route API is not supported by Android Auto: " + uri); return;
    case RequestType.CROSSHAIR: Logger.e(TAG, "Crosshair API is not supported by Android Auto: " + uri); return;
    case RequestType.MENU: Logger.e(TAG, "Menu API is not supported by Android Auto: " + uri); return;
    case RequestType.SETTINGS: Logger.e(TAG, "Settings API is not supported by Android Auto: " + uri);
    }
  }

  private static void processViewIntent(@NonNull CarContext carContext, @NonNull Intent intent)
  {
    final Uri uri = intent.getData();
    if (uri != null && Const.API_SCHEME.equals(uri.getScheme())
        && CarAppService.API_CAR_HOST.equals(uri.getSchemeSpecificPart())
        && CarAppService.ACTION_SHOW_NAVIGATION_SCREEN.equals(uri.getFragment()))
    {
      final ScreenManager screenManager = carContext.getCarService(ScreenManager.class);
      final Screen top = screenManager.getTop();
      final DisplayManager displayManager = MwmApplication.from(carContext).getDisplayManager();
      if (!displayManager.isCarDisplayUsed())
        displayManager.changeDisplay(DisplayType.Car);
      if (!(top instanceof NavigationScreen))
        screenManager.popTo(NavigationScreen.MARKER);
    }
  }

  private IntentUtils() {}
}
