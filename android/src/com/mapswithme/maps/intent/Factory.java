package com.mapswithme.maps.intent;

import android.content.ContentResolver;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import com.mapswithme.maps.DownloadResourcesLegacyActivity;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MapFragment;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.api.ParsedRoutingData;
import com.mapswithme.maps.api.ParsedSearchRequest;
import com.mapswithme.maps.api.ParsingResult;
import com.mapswithme.maps.api.RoutePoint;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.search.SearchActivity;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.util.KeyValue;
import com.mapswithme.util.StorageUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.ThreadPool;

import java.io.File;
import java.util.List;

public class Factory
{
  public static class GeoIntentProcessor implements IntentProcessor
  {
    @Nullable
    @Override
    public MapTask process(@NonNull Intent intent)
    {
      final Uri uri = intent.getData();
      if (uri == null)
        return null;
      final String scheme = intent.getScheme();
      if (!"geo".equals(scheme) && !"ge0".equals(scheme) && !"om".equals(scheme) && !"mapsme".equals(scheme))
        return null;
      SearchEngine.INSTANCE.cancelInteractiveSearch();
      final ParsedMwmRequest request = ParsedMwmRequest.extractFromIntent(intent);
      ParsedMwmRequest.setCurrentRequest(request);
      return new OpenUrlTask(uri.toString());
    }
  }

  public static class HttpGeoIntentProcessor implements IntentProcessor
  {
    @Nullable
    @Override
    public MapTask process(@NonNull Intent intent)
    {
      final Uri uri = intent.getData();
      if (uri == null)
        return null;
      final String scheme = intent.getScheme();
      if (!"http".equalsIgnoreCase(scheme) && !"https".equalsIgnoreCase(scheme))
        return null;
      final String host = uri.getHost();
      if (!"omaps.app".equalsIgnoreCase(host) && !"ge0.me".equalsIgnoreCase(host))
        return null;
      if (uri.getPath() == null)
        return null;
      final String ge0Url = "om:/" + uri.getPath();

      SearchEngine.INSTANCE.cancelInteractiveSearch();
      final ParsedMwmRequest request = ParsedMwmRequest.extractFromIntent(intent);
      ParsedMwmRequest.setCurrentRequest(request);
      return new OpenUrlTask(ge0Url);
    }
  }

  public static class HttpMapsIntentProcessor implements IntentProcessor
  {
    @Nullable
    @Override
    public MapTask process(@NonNull Intent intent)
    {
      final Uri uri = intent.getData();
      if (uri == null)
        return null;
      final String scheme = intent.getScheme();
      if (!"http".equalsIgnoreCase(scheme) && !"https".equalsIgnoreCase(scheme))
        return null;
      if (uri.getHost() == null)
        return null;
      final String host = uri.getHost().toLowerCase();
      if (!host.contains("google") && !host.contains("2gis") && !host.contains("openstreetmap"))
        return null;
      return new OpenHttpMapsUrlTask(uri.toString());
    }
  }

  public static class OpenCountryTaskProcessor implements IntentProcessor
  {
    @Nullable
    @Override
    public MapTask process(@NonNull Intent intent)
    {
      if (!intent.hasExtra(DownloadResourcesLegacyActivity.EXTRA_COUNTRY))
        return null;
      String countryId = intent.getStringExtra(DownloadResourcesLegacyActivity.EXTRA_COUNTRY);
      return new ShowCountryTask(countryId);
    }
  }

  public static class KmzKmlProcessor implements IntentProcessor
  {
    @NonNull
    private final DownloadResourcesLegacyActivity mActivity;

    public KmzKmlProcessor(@NonNull DownloadResourcesLegacyActivity activity)
    {
      mActivity = activity;
    }

    @Nullable
    @Override
    public MapTask process(@NonNull Intent intent)
    {
      // See KML/KMZ/KMB intent filters in manifest.
      final Uri uri;
      if (Intent.ACTION_VIEW.equals(intent.getAction()))
        uri = intent.getData();
      else if (Intent.ACTION_SEND.equals(intent.getAction()))
        uri = intent.getParcelableExtra(Intent.EXTRA_STREAM);
      else
        uri = null;
      if (uri == null)
        return null;

      MwmApplication app = MwmApplication.from(mActivity);
      final File tempDir = new File(StorageUtils.getTempPath(app));
      final ContentResolver resolver = mActivity.getContentResolver();
      ThreadPool.getStorage().execute(() -> {
        BookmarkManager.INSTANCE.importBookmarksFile(resolver, uri, tempDir);
        mActivity.runOnUiThread(mActivity::showMap);
      });
      return null;
    }
  }

  abstract static class UrlTaskWithStatistics implements MapTask
  {
    private static final long serialVersionUID = -8661639898700431066L;
    @NonNull
    private final String mUrl;

    UrlTaskWithStatistics(@NonNull String url)
    {
      Utils.checkNotNull(url);
      mUrl = url;
    }

    @NonNull
    String getUrl()
    {
      return mUrl;
    }
  }

  public static class OpenHttpMapsUrlTask extends UrlTaskWithStatistics
  {
    OpenHttpMapsUrlTask(@NonNull String url)
    {
      super(url);
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      return MapFragment.nativeShowMapForUrl(getUrl());
    }
  }

  public static class OpenUrlTask extends UrlTaskWithStatistics
  {
    private static final long serialVersionUID = -7257820771228127413L;
    private static final int SEARCH_IN_VIEWPORT_ZOOM = 16;

    OpenUrlTask(@NonNull String url)
    {
      super(url);
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      final ParsingResult result = Framework.nativeParseAndSetApiUrl(getUrl());

      // TODO: Kernel recognizes "mapsme://", "mwm://" and "mapswithme://" schemas only!!!
      if (result.getUrlType() == ParsingResult.TYPE_INCORRECT)
        return MapFragment.nativeShowMapForUrl(getUrl());

      if (!result.isSuccess())
        return false;

      final Uri uri = Uri.parse(getUrl());
      final String backUrl = uri.getQueryParameter("backurl");
      if (!TextUtils.isEmpty(backUrl))
      {
        Intent intent = target.getIntent();
        if (intent != null)
          intent.putExtra(MwmActivity.EXTRA_BACK_URL, backUrl);
      }

      switch (result.getUrlType())
      {
        case ParsingResult.TYPE_INCORRECT:
          return false;

        case ParsingResult.TYPE_MAP:
          return MapFragment.nativeShowMapForUrl(getUrl());

        case ParsingResult.TYPE_ROUTE:
          final ParsedRoutingData data = Framework.nativeGetParsedRoutingData();
          RoutingController.get().setRouterType(data.mRouterType);
          final RoutePoint from = data.mPoints[0];
          final RoutePoint to = data.mPoints[1];
          RoutingController.get().prepare(MapObject.createMapObject(FeatureId.EMPTY, MapObject.API_POINT,
                                                                    from.mName, "", from.mLat, from.mLon),
                                          MapObject.createMapObject(FeatureId.EMPTY, MapObject.API_POINT,
                                                                    to.mName, "", to.mLat, to.mLon), true);
          return true;
        case ParsingResult.TYPE_SEARCH:
        {
          final ParsedSearchRequest request = Framework.nativeGetParsedSearchRequest();
          final double[] latlon = Framework.nativeGetParsedCenterLatLon();
          if (latlon[0] != 0.0 || latlon[1] != 0.0)
          {
            Framework.nativeStopLocationFollow();
            Framework.nativeSetViewportCenter(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
            // We need to update viewport for search api manually because of drape engine
            // will not notify subscribers when search activity is shown.
            if (!request.mIsSearchOnMap)
              Framework.nativeSetSearchViewport(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
          }
          SearchActivity.start(target, request.mQuery, request.mLocale, request.mIsSearchOnMap);
          return true;
        }
        case ParsingResult.TYPE_CROSSHAIR:
        {
          target.showPositionChooserForAPI(Framework.nativeGetParsedAppName());

          final double[] latlon = Framework.nativeGetParsedCenterLatLon();
          if (latlon[0] != 0.0 || latlon[1] != 0.0)
          {
            Framework.nativeStopLocationFollow();
            Framework.nativeSetViewportCenter(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
          }

          return true;
        }
      }

      return false;
    }
  }

  public static class ShowCountryTask implements MapTask {
    private static final long serialVersionUID = 256630934543189768L;
    private final String mCountryId;

    public ShowCountryTask(String countryId)
    {
      mCountryId = countryId;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      Framework.nativeShowCountry(mCountryId, false);
      return true;
    }
  }

  public static class ShowBookmarkCategoryTask implements MapTask
  {
    private static final long serialVersionUID = 8285565041410550281L;
    final long mCategoryId;

    public ShowBookmarkCategoryTask(long categoryId)
    {
      mCategoryId = categoryId;
    }

    public boolean run(@NonNull MwmActivity target)
    {
      target.showBookmarkCategoryOnMap(mCategoryId);
      return true;
    }
  }

  static abstract class BaseUserMarkTask implements MapTask
  {
    private static final long serialVersionUID = -3348320422813422144L;
    final long mCategoryId;
    final long mId;

    BaseUserMarkTask(long categoryId, long id)
    {
      mCategoryId = categoryId;
      mId = id;
    }
  }

  public static class ShowBookmarkTask extends BaseUserMarkTask
  {
    private static final long serialVersionUID = 7582931785363515736L;

    public ShowBookmarkTask(long categoryId, long bookmarkId)
    {
      super(categoryId, bookmarkId);
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      target.showBookmarkOnMap(mId);
      return true;
    }
  }

  public static class ShowTrackTask extends BaseUserMarkTask
  {
    private static final long serialVersionUID = 1091286722919338991L;

    public ShowTrackTask(long categoryId, long trackId)
    {
      super(categoryId, trackId);
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      target.showTrackOnMap(mId);
      return true;
    }
  }

  public static class RestoreRouteTask implements MapTask
  {
    private static final long serialVersionUID = 6123893958975977040L;

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      RoutingController.get().restoreRoute();
      return true;
    }
  }

  public static class ShowDialogTask implements MapTask
  {
    private static final long serialVersionUID = 1548931513812565018L;
    @NonNull
    private final String mDialogName;

    public ShowDialogTask(@NonNull String dialogName)
    {
      mDialogName = dialogName;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      Fragment f = target.getSupportFragmentManager().findFragmentByTag(mDialogName);
      if (f != null)
        return true;

      final DialogFragment fragment = (DialogFragment) Fragment.instantiate(target, mDialogName);
      fragment.show(target.getSupportFragmentManager(), mDialogName);
      return true;
    }

    @NonNull
    private static Bundle toDialogArgs(@NonNull List<KeyValue> pairs)
    {
      Bundle bundle = new Bundle();
      for (KeyValue each : pairs)
        bundle.putString(each.getKey(), each.getValue());
      return bundle;
    }
  }
}
