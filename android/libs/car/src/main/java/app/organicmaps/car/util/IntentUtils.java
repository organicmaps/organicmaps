package app.organicmaps.car.util;

import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Intent;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.notification.CarPendingIntent;
import app.organicmaps.api.Const;
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

    final String action = intent.getAction();
    if (isNavigationAction(action))
    {
      final Uri uri = intent.getData();
      if (uri != null)
        processNavigationIntent(carContext, organicMapsContext, surfaceRenderer, uri);
    }
    else if (Intent.ACTION_VIEW.equals(action))
      processViewIntent(carContext, intent);
  }

  /** Returns whether the request needs the car display before its navigation/search UI can change. */
  public static boolean requiresCarDisplay(@NonNull Intent intent)
  {
    final String action = intent.getAction();
    if (isNavigationAction(action))
      return intent.getData() != null;
    if (!Intent.ACTION_VIEW.equals(action))
      return false;
    if (isShowNavigationRequest(intent))
      return true;
    final Uri uri = intent.getData();
    if (uri == null || !uri.isHierarchical())
      return false;
    final String scheme = uri.getScheme();
    return ("geo".equals(scheme) || "geo.offline".equals(scheme)) && "assistant".equals(uri.getQueryParameter("entry"));
  }

  private static boolean isNavigationAction(@Nullable String action)
  {
    return CarContext.ACTION_NAVIGATE.equals(action) || "android.intent.action.NAVIGATE".equals(action);
  }

  private static final class CarGoogleAssistantIntentProcessor extends GoogleAssistantIntentHandler
  {
    boolean processIntent(@NonNull CarContext carContext, @NonNull OrganicMaps organicMapsContext,
                          @NonNull Renderer surfaceRenderer, @NonNull Intent intent)
    {
      return handleIntent(intent, new CarSearchHandler(carContext, organicMapsContext, surfaceRenderer));
    }
  }

  private record CarSearchHandler(@NonNull CarContext mCarContext, @NonNull OrganicMaps mOrganicMapsContext,
                                  @NonNull Renderer mSurfaceRenderer)
      implements GoogleAssistantIntentHandler.SearchHandler
  {
    @Override
    public void handleSearch(@NonNull String query, boolean searchOnMap)
    {
      final ScreenManager screenManager = mCarContext.getCarService(ScreenManager.class);
      final SearchScreen.Builder builder = new SearchScreen.Builder(mCarContext, mOrganicMapsContext, mSurfaceRenderer);
      builder.setQuery(query);

      screenManager.popToRoot();
      screenManager.push(builder.build());
    }
  }

  @NonNull
  public static PendingIntent createSearchIntent(@NonNull CarContext context, @NonNull String query)
  {
    final ComponentName component = new ComponentName(context, CarAppServiceBase.getCarServiceClass());
    final Intent intent =
        new Intent(CarContext.ACTION_NAVIGATE, Uri.parse("geo:0,0?q=" + Uri.encode(query))).setComponent(component);
    return CarPendingIntent.getCarApp(context, 0, intent, 0);
  }

  // https://developer.android.com/reference/androidx/car/app/CarContext#startCarApp(android.content.Intent)
  private static void processNavigationIntent(@NonNull CarContext carContext, @NonNull OrganicMaps organicMapsContext,
                                              @NonNull Renderer surfaceRenderer, @NonNull Uri uri)
  {
    // TODO (AndrewShkrob): This logic will need to be revised when we introduce support for adding stops during
    // navigation or route planning. Skip navigation intents during navigation
    if (RoutingController.get().isNavigating())
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
      final SearchScreen.Builder builder = new SearchScreen.Builder(carContext, organicMapsContext, surfaceRenderer);
      builder.setQuery(request.mQuery);
      if (request.mLocale != null)
        builder.setLocale(request.mLocale);

      screenManager.push(builder.build());
      return;
    case RequestType.ROUTE: Logger.w(TAG, "Route API is not supported by Android Auto: " + uri); return;
    case RequestType.CROSSHAIR: Logger.w(TAG, "Crosshair API is not supported by Android Auto: " + uri); return;
    case RequestType.MENU: Logger.w(TAG, "Menu API is not supported by Android Auto: " + uri); return;
    case RequestType.SETTINGS: Logger.w(TAG, "Settings API is not supported by Android Auto: " + uri); return;
    case RequestType.OAUTH2: Logger.w(TAG, "OAuth2 API is not supported by Android Auto: " + uri);
    }
  }

  private static boolean isShowNavigationRequest(@NonNull Intent intent)
  {
    final Uri uri = intent.getData();
    if (uri == null || !Const.API_SCHEME.equals(uri.getScheme()))
      return false;
    if (!CarAppServiceBase.API_CAR_HOST.equals(uri.getSchemeSpecificPart()))
      return false;
    return CarAppServiceBase.ACTION_SHOW_NAVIGATION_SCREEN.equals(uri.getFragment());
  }

  private static void processViewIntent(@NonNull CarContext carContext, @NonNull Intent intent)
  {
    if (!isShowNavigationRequest(intent))
      return;
    final ScreenManager screenManager = carContext.getCarService(ScreenManager.class);
    if (!(screenManager.getTop() instanceof NavigationScreen))
      screenManager.popTo(NavigationScreen.MARKER);
  }

  private IntentUtils() {}
}
