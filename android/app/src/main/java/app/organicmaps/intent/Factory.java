package app.organicmaps.intent;

import static app.organicmaps.api.Const.EXTRA_PICK_POINT;

import android.content.ContentResolver;
import android.content.Intent;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.core.content.IntentCompat;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.editor.OsmLoginActivity;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.api.ParsedRoutingData;
import app.organicmaps.sdk.api.ParsedSearchRequest;
import app.organicmaps.sdk.api.RequestType;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import java.io.File;
import java.util.Collections;
import java.util.List;

public class Factory
{
  public static boolean isStartedForApiResult(@NonNull Intent intent)
  {
    // The explicit pick-point marker distinguishes result requests from ordinary navigation intents.
    return intent.getBooleanExtra(EXTRA_PICK_POINT, false);
  }

  public static class KmzKmlProcessor implements IntentProcessor
  {
    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity activity)
    {
      final List<Uri> uris;
      if (Intent.ACTION_VIEW.equals(intent.getAction()))
        uris = Collections.singletonList(intent.getData());
      else if (Intent.ACTION_SEND.equals(intent.getAction()))
        uris = Collections.singletonList(IntentCompat.getParcelableExtra(intent, Intent.EXTRA_STREAM, Uri.class));
      else if (Intent.ACTION_SEND_MULTIPLE.equals(intent.getAction()))
        uris = IntentCompat.getParcelableArrayListExtra(intent, Intent.EXTRA_STREAM, Uri.class);
      else
        return false;
      if (uris == null || uris.isEmpty())
        return false;

      // Other deep links reach this fallback, and filters do not constrain shared streams or explicit intents.
      // IntentCompat does not check list element types on Android 13 and earlier.
      for (Object stream : uris)
      {
        if (!(stream instanceof Uri uri))
          return false;
        final String scheme = uri.getScheme();
        if (!ContentResolver.SCHEME_CONTENT.equals(scheme) && !ContentResolver.SCHEME_FILE.equals(scheme))
          return false;
      }

      MwmApplication app = MwmApplication.from(activity);
      final File tempDir = new File(StorageUtils.getTempPath(app));
      final ContentResolver resolver = activity.getContentResolver();
      ThreadPool.getStorage().execute(() -> BookmarkManager.INSTANCE.importBookmarksFiles(resolver, uris, tempDir));
      return true;
    }
  }

  public static class UrlProcessor implements IntentProcessor
  {
    private static final int SEARCH_IN_VIEWPORT_ZOOM = 16;

    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity target)
    {
      final Uri uri = intent.getData();
      if (uri == null)
        return false;

      switch (Framework.nativeParseAndSetApiUrl(uri.toString()))
      {
      case RequestType.INCORRECT: return false;

      case RequestType.MAP:
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        target.forceCloseSearchFragment();
        Map.executeMapApiRequest();
        return true;

      case RequestType.ROUTE:
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        target.forceCloseSearchFragment();
        final ParsedRoutingData data = Framework.nativeGetParsedRoutingData();
        RoutingController.get().prepareApiRoute(data.mRouterType, data.mStartRouteNavigation);
        return true;
      case RequestType.SEARCH:
      {
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        final ParsedSearchRequest request = Framework.nativeGetParsedSearchRequest();
        final double[] latlon = Framework.nativeGetParsedCenterLatLon();
        if (latlon != null)
        {
          Framework.nativeStopLocationFollow();
          Framework.nativeSetViewportCenter(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
          // We need to update viewport for search api manually because of drape engine
          // will not notify subscribers when search is shown.
          if (!request.mIsSearchOnMap)
            Framework.nativeSetSearchViewport(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
        }
        target.showSearch(request.mQuery, request.mLocale, request.mIsSearchOnMap);
        return true;
      }
      case RequestType.CROSSHAIR:
      {
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        target.forceCloseSearchFragment();
        target.showPositionChooserForAPI(Framework.nativeGetParsedAppName());

        final double[] latlon = Framework.nativeGetParsedCenterLatLon();
        if (latlon != null)
        {
          Framework.nativeStopLocationFollow();
          Framework.nativeSetViewportCenter(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
        }

        return true;
      }
      case RequestType.OAUTH2:
      {
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        target.forceCloseSearchFragment();

        final String oauth2code = Framework.nativeGetParsedOAuth2Code();
        OsmLoginActivity.OAuth2Callback(target, oauth2code);

        return true;
      }

      // Menu and Settings url types should be implemented to support deeplinking.
      case RequestType.MENU:
      case RequestType.SETTINGS:
      }

      return false;
    }
  }

  public static class GoggleAssistanceIntentProcessor extends GoogleAssistantIntentHandler implements IntentProcessor
  {
    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity activity)
    {
      return handleIntent(intent, (query, searchOnMap) -> activity.showSearch(query, null, searchOnMap));
    }
  }
}
