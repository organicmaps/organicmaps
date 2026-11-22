package app.organicmaps.car.util;

import android.app.PendingIntent;
import android.content.Intent;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.ScreenManager;
import androidx.car.app.notification.CarPendingIntent;
import app.organicmaps.api.Const;
import app.organicmaps.api.MapRequest;
import app.organicmaps.car.CarAppServiceBase;
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.car.screens.search.SearchScreen;
import app.organicmaps.intent.GoogleAssistantIntentHandler;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.api.ParsedSearchRequest;
import app.organicmaps.sdk.api.RequestType;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.log.Logger;

public final class IntentUtils
{
  private static final String TAG = IntentUtils.class.getSimpleName();

  private static final int SEARCH_IN_VIEWPORT_ZOOM = 16;

  private static final CarGoogleAssistantIntentProcessor ASSISTANT_PROCESSOR = new CarGoogleAssistantIntentProcessor();

  public static void processIntent(@NonNull CarContext carContext, @NonNull OrganicMaps organicMapsContext,
                                   @NonNull Renderer surfaceRenderer, @NonNull Intent intent)
  {
    if (ASSISTANT_PROCESSOR.processIntent(carContext, organicMapsContext, surfaceRenderer, intent))
      return;

    if (isMapRequest(intent))
      processMapRequest(carContext, organicMapsContext, surfaceRenderer, intent);
    else if (isShowNavigationRequest(intent))
      showNavigationScreen(carContext);
  }

  public static boolean requiresCarDisplay(@NonNull Intent intent)
  {
    return isMapRequest(intent) || isShowNavigationRequest(intent);
  }

  private static boolean isMapRequest(@NonNull Intent intent)
  {
    final Uri uri = intent.getData();
    if (uri == null)
      return false;
    if (MapRequest.isNavigationAction(intent.getAction()))
      return true;
    if (!Intent.ACTION_VIEW.equals(intent.getAction()))
      return false;
    final String scheme = uri.getScheme();
    return "geo".equals(scheme) || "geo.offline".equals(scheme)
 || (Const.API_SCHEME.equals(scheme) && "search".equals(uri.getAuthority()));
  }

  private static final class CarGoogleAssistantIntentProcessor extends GoogleAssistantIntentHandler
  {
    boolean processIntent(@NonNull CarContext carContext, @NonNull OrganicMaps organicMapsContext,
                          @NonNull Renderer surfaceRenderer, @NonNull Intent intent)
    {
      return handleIntent(intent, new CarSearchHandler(carContext, organicMapsContext, surfaceRenderer));
    }
  }

  private record CarSearchHandler(CarContext mCarContext, OrganicMaps mOrganicMapsContext, Renderer mSurfaceRenderer)
      implements GoogleAssistantIntentHandler.SearchHandler
  {
    private CarSearchHandler(@NonNull CarContext mCarContext, @NonNull OrganicMaps mOrganicMapsContext,
                             @NonNull Renderer mSurfaceRenderer)
    {
      this.mCarContext = mCarContext;
      this.mOrganicMapsContext = mOrganicMapsContext;
      this.mSurfaceRenderer = mSurfaceRenderer;
    }

    @Override
    public void handleSearch(@NonNull String query, boolean searchOnMap)
    {
      final ScreenManager screenManager = mCarContext.getCarService(ScreenManager.class);
      final SearchScreen.Builder builder = new SearchScreen.Builder(mCarContext, mOrganicMapsContext, mSurfaceRenderer);
      builder.setQuery(query);

      showSearch(screenManager, builder);
    }
  }

  @NonNull
  public static PendingIntent createSearchIntent(@NonNull CarContext context, @NonNull String query)
  {
    final Uri uri =
        new Uri.Builder().scheme(Const.API_SCHEME).authority("search").appendQueryParameter("query", query).build();
    final Intent intent = new Intent(Intent.ACTION_VIEW, uri, context, CarAppServiceBase.getCarServiceClass());
    return CarPendingIntent.getCarApp(context, 0, intent, 0);
  }

  // https://developer.android.com/reference/androidx/car/app/CarContext#startCarApp(android.content.Intent)
  private static void processMapRequest(@NonNull CarContext carContext, @NonNull OrganicMaps organicMapsContext,
                                        @NonNull Renderer surfaceRenderer, @NonNull Intent intent)
  {
    final Uri uri = intent.getData();
    final ScreenManager screenManager = carContext.getCarService(ScreenManager.class);
    switch (Framework.nativeParseAndSetApiUrl(uri.toString()))
    {
    case RequestType.INCORRECT: return;
    case RequestType.MAP:
      if (RoutingController.get().isNavigating())
      {
        if (!MapRequest.isNavigationAction(intent.getAction()))
          return;
        RoutingController.get().cancel();
      }
      screenManager.popToRoot();
      Map.executeMapApiRequest();
      return;
    case RequestType.SEARCH:
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
      final SearchScreen.Builder builder = new SearchScreen.Builder(carContext, organicMapsContext, surfaceRenderer);
      builder.setQuery(request.mQuery);
      if (request.mLocale != null)
        builder.setLocale(request.mLocale);

      showSearch(screenManager, builder);
      return;
    case RequestType.ROUTE: Logger.w(TAG, "Route API is not supported by Android Auto: " + uri); return;
    case RequestType.CROSSHAIR: Logger.w(TAG, "Crosshair API is not supported by Android Auto: " + uri); return;
    case RequestType.MENU: Logger.w(TAG, "Menu API is not supported by Android Auto: " + uri); return;
    case RequestType.SETTINGS: Logger.w(TAG, "Settings API is not supported by Android Auto: " + uri); return;
    case RequestType.OAUTH2: Logger.w(TAG, "OAuth2 API is not supported by Android Auto: " + uri);
    }
  }

  private static void showSearch(@NonNull ScreenManager screens, @NonNull SearchScreen.Builder builder)
  {
    if (RoutingController.get().isNavigating())
      screens.popTo(NavigationScreen.MARKER);
    else
      screens.popToRoot();
    screens.push(builder.build());
  }

  private static boolean isShowNavigationRequest(@NonNull Intent intent)
  {
    final Uri uri = intent.getData();
    return Intent.ACTION_VIEW.equals(intent.getAction()) && uri != null && Const.API_SCHEME.equals(uri.getScheme())
 && CarAppServiceBase.API_CAR_HOST.equals(uri.getSchemeSpecificPart())
 && CarAppServiceBase.ACTION_SHOW_NAVIGATION_SCREEN.equals(uri.getFragment());
  }

  private static void showNavigationScreen(@NonNull CarContext carContext)
  {
    final ScreenManager screens = carContext.getCarService(ScreenManager.class);
    if (!(screens.getTop() instanceof NavigationScreen))
      screens.popTo(NavigationScreen.MARKER);
  }

  private IntentUtils() {}
}
