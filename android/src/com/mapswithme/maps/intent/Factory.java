package com.mapswithme.maps.intent;

import android.content.ContentResolver;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.Toolbar;
import androidx.coordinatorlayout.widget.CoordinatorLayout;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import com.google.android.material.appbar.AppBarLayout;
import com.mapswithme.maps.DownloadResourcesLegacyActivity;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MapFragment;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.Const;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.api.ParsedRoutingData;
import com.mapswithme.maps.api.ParsedSearchRequest;
import com.mapswithme.maps.api.ParsedUrlMwmRequest;
import com.mapswithme.maps.api.ParsingResult;
import com.mapswithme.maps.api.RoutePoint;
import com.mapswithme.maps.background.NotificationCandidate;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.bookmarks.BookmarksPageFactory;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkInfo;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.onboarding.IntroductionScreenFactory;
import com.mapswithme.maps.purchase.BookmarksAllSubscriptionActivity;
import com.mapswithme.maps.purchase.BookmarksSightsSubscriptionActivity;
import com.mapswithme.maps.purchase.PurchaseUtils;
import com.mapswithme.maps.purchase.SubscriptionType;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.search.SearchActivity;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.maps.tips.Tutorial;
import com.mapswithme.maps.ugc.EditParams;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.maps.ugc.UGCEditorActivity;
import com.mapswithme.util.Constants;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.KeyValue;
import com.mapswithme.util.StorageUtils;
import com.mapswithme.util.UTM;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Objects;

public class Factory
{
  public static final String EXTRA_IS_FIRST_LAUNCH = "extra_is_first_launch";

  @NonNull
  public static IntentProcessor createBuildRouteProcessor()
  {
    return new BuildRouteProcessor();
  }

  @NonNull
  public static IntentProcessor createShowOnMapProcessor()
  {
    return new ShowOnMapProcessor();
  }

  @NonNull
  public static IntentProcessor createKmzKmlProcessor(@NonNull DownloadResourcesLegacyActivity activity)
  {
    return new KmzKmlProcessor(activity);
  }

  @NonNull
  public static IntentProcessor createOpenCountryTaskProcessor()
  {
    return new OpenCountryTaskProcessor();
  }

  @NonNull
  public static IntentProcessor createOldCoreLinkAdapterProcessor()
  {
    return new OldCoreLinkAdapterProcessor();
  }

  @NonNull
  public static IntentProcessor createDlinkBookmarkCatalogueProcessor()
  {
    return new DlinkBookmarkCatalogueIntentProcessor();
  }

  @NonNull
  public static IntentProcessor createDlinkBookmarkGuidesPageProcessor()
  {
    return new DlinkGuidesPageIntentProcessor();
  }

  @NonNull
  public static IntentProcessor createDlinkBookmarksSubscriptionProcessor()
  {
    return new DlinkBookmarksSubscriptionIntentProcessor();
  }

  @NonNull
  public static IntentProcessor createOldLeadUrlProcessor()
  {
    return new OldLeadUrlIntentProcessor();
  }

  @NonNull
  public static IntentProcessor createGoogleMapsIntentProcessor()
  {
    return new GoogleMapsIntentProcessor();
  }

  @NonNull
  public static IntentProcessor createMapsWithMeIntentProcessor()
  {
    return new MapsWithMeIntentProcessor();
  }

  @NonNull
  public static IntentProcessor createGe0IntentProcessor()
  {
    return new Ge0IntentProcessor();
  }

  @NonNull
  public static IntentProcessor createHttpGe0IntentProcessor()
  {
    return new HttpGe0IntentProcessor();
  }

  @NonNull
  public static IntentProcessor createMapsmeBookmarkCatalogueProcessor()
  {
    return new MapsmeBookmarkCatalogueProcessor();
  }

  @NonNull
  public static IntentProcessor createGeoIntentProcessor()
  {
    return new GeoIntentProcessor();
  }

  @NonNull
  public static IntentProcessor createMapsmeProcessor()
  {
    return new MapsmeProcessor();
  }

  @NonNull
  private static String convertUrlToGuidesPageDeeplink(@NonNull String url)
  {
    String baseCatalogUrl = BookmarkManager.INSTANCE.getCatalogFrontendUrl(UTM.UTM_NONE);
    String relativePath = Uri.parse(url).getQueryParameter("url");
    return Uri.parse(baseCatalogUrl)
              .buildUpon().appendEncodedPath(relativePath).toString();
  }

  private static abstract class LogIntentProcessor implements IntentProcessor
  {
    private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
    private boolean mFirstLaunch;
    @NonNull
    @Override
    public final MapTask process(@NonNull Intent intent)
    {
      mFirstLaunch = intent.getBooleanExtra(Factory.EXTRA_IS_FIRST_LAUNCH, false);
      Uri data = intent.getData();
      if (data == null)
        throw new AssertionError("Data must be non-null!");

      final String uri = data.toString();
      String msg = this.getClass().getSimpleName() + ": incoming intent uri: " + uri;
      LOGGER.i(this.getClass().getSimpleName(), msg);
      org.alohalytics.Statistics.logEvent(msg);
      CrashlyticsUtils.INSTANCE.log(Log.INFO, getClass().getSimpleName(), msg);
      return createMapTask(uri);
    }

    final boolean isFirstLaunch()
    {
      return mFirstLaunch;
    }

    @NonNull
    abstract MapTask createMapTask(@NonNull String uri);
  }

  private static abstract class BaseOpenUrlProcessor extends LogIntentProcessor
  {
    @NonNull
    @Override
    MapTask createMapTask(@NonNull String uri)
    {
      return BackUrlMapTaskWrapper.wrap(new OpenUrlTask(uri, Statistics.ParamValue.UNKNOWN), uri);
    }
  }

  private static class GeoIntentProcessor extends BaseOpenUrlProcessor
  {
    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      return (intent.getData() != null && "geo".equals(intent.getScheme()));
    }

    @NonNull
    @Override
    MapTask createMapTask(@NonNull String uri)
    {
      return StatisticMapTaskWrapper.wrap(new OpenUrlTask(uri, "geo"));
    }
  }

  private static class Ge0IntentProcessor extends BaseOpenUrlProcessor
  {
    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      return (intent.getData() != null && "ge0".equals(intent.getScheme()));
    }

    @NonNull
    @Override
    MapTask createMapTask(@NonNull String uri)
    {
      return StatisticMapTaskWrapper.wrap(new OpenUrlTask(uri, "ge0"));
    }
  }

  private static class MapsmeProcessor extends BaseOpenUrlProcessor
  {
    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      return "mapsme".equals(intent.getScheme());
    }

    @NonNull
    @Override
    MapTask createMapTask(@NonNull String uri)
    {
      return StatisticMapTaskWrapper.wrap(new OpenUrlTask(uri, "mapsme"));
    }
  }

  private static class HttpGe0IntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      if ("http".equalsIgnoreCase(intent.getScheme()))
      {
        final Uri data = intent.getData();
        if (data != null)
          return "ge0.me".equals(data.getHost());
      }

      return false;
    }

    @NonNull
    @Override
    public MapTask process(@NonNull Intent intent)
    {
      final Uri data = intent.getData();
      final String ge0Url = "ge0:/" + data.getPath();
      org.alohalytics.Statistics.logEvent("HttpGe0IntentProcessor::process", ge0Url);
      return StatisticMapTaskWrapper.wrap(new OpenUrlTask(ge0Url, "http_ge0_me"));
    }
  }

  /**
   * Use this to invoke API task.
   */
  private static class MapsWithMeIntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      return Const.ACTION_MWM_REQUEST.equals(intent.getAction());
    }

    @NonNull
    @Override
    public MapTask process(@NonNull final Intent intent)
    {
      final String apiUrl = intent.getStringExtra(Const.EXTRA_URL);
      org.alohalytics.Statistics.logEvent("MapsWithMeIntentProcessor::process", apiUrl == null ? "null" : apiUrl);
      if (apiUrl != null)
      {
        SearchEngine.INSTANCE.cancelInteractiveSearch();

        final ParsedMwmRequest request = ParsedMwmRequest.extractFromIntent(intent);
        ParsedMwmRequest.setCurrentRequest(request);
        Statistics.INSTANCE.trackApiCall(request);

        if (!ParsedMwmRequest.isPickPointMode())
          return StatisticMapTaskWrapper.wrap(new OpenUrlTask(apiUrl, "action_api_request"));
      }

      throw new AssertionError("Url must be provided!");
    }
  }

  private static class GoogleMapsIntentProcessor extends BaseOpenUrlProcessor
  {
    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      final Uri data = intent.getData();
      return (data != null && "maps.google.com".equals(data.getHost()));
    }

    @NonNull
    @Override
    MapTask createMapTask(@NonNull String uri)
    {
      return StatisticMapTaskWrapper.wrap(new OpenUrlTask(uri, "maps_google_com"));
    }
  }

  private static class OldLeadUrlIntentProcessor extends BaseOpenUrlProcessor
  {
    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      final Uri data = intent.getData();

      if (data == null)
        return false;

      String scheme = intent.getScheme();
      String host = data.getHost();
      if (TextUtils.isEmpty(scheme) || TextUtils.isEmpty(host))
        return false;

      return (scheme.equals("mapsme") || scheme.equals("mapswithme")) && "lead".equals(host);
    }

    @NonNull
    @Override
    MapTask createMapTask(@NonNull String uri)
    {
      return StatisticMapTaskWrapper.wrap(new OpenUrlTask(uri, "old_lead"));
    }
  }

  private static class DlinkBookmarkCatalogueIntentProcessor extends DlinkIntentProcessor
  {
    static final String CATALOGUE = "catalogue";

    @Override
    boolean isLinkSupported(@NonNull Uri data)
    {
      return (File.separator + CATALOGUE).equals(data.getPath());
    }

    @Nullable
    @Override
    MapTask createIntroductionTask(@NonNull String url)
    {
      return new FreeGuideReadyToDownloadIntroductionTask(url);
    }

    @NonNull
    @Override
    MapTask createTargetTask(@NonNull String url)
    {
      return StatisticMapTaskWrapper.wrap(new ImportBookmarkCatalogueTask(url));
    }
  }

  public static class DlinkGuidesPageIntentProcessor extends DlinkIntentProcessor
  {
    static final String GUIDES_PAGE = "guides_page";

    @Override
    boolean isLinkSupported(@NonNull Uri data)
    {
      return (File.separator + GUIDES_PAGE).equals(data.getPath());
    }

    @Nullable
    @Override
    MapTask createIntroductionTask(@NonNull String url)
    {
      return new GuidesPageToOpenIntroductionTask(url);
    }

    @NonNull
    @Override
    MapTask createTargetTask(@NonNull String url)
    {
      return StatisticMapTaskWrapper.wrap(new GuidesPageToOpenTask(url));
    }
  }

  public static class DlinkBookmarksSubscriptionIntentProcessor extends DlinkIntentProcessor
  {
    static final String SUBSCRIPTION = "subscription";

    @Override
    boolean isLinkSupported(@NonNull Uri data)
    {
      return (File.separator + SUBSCRIPTION).equals(data.getPath());
    }

    @Nullable
    @Override
    MapTask createIntroductionTask(@NonNull String url)
    {
      // In release 9.5 the introduction screen for this deeplink is forgotten.
      return null;
    }

    @NonNull
    @Override
    MapTask createTargetTask(@NonNull String url)
    {
      return StatisticMapTaskWrapper.wrap(new BookmarksSubscriptionTask(url));
    }
  }

  private static class MapsmeBookmarkCatalogueProcessor extends MapsmeProcessor
  {
    @NonNull
    @Override
    MapTask createMapTask(@NonNull String uri)
    {
      String url = Uri.parse(uri).buildUpon()
                      .scheme(DlinkIntentProcessor.SCHEME_HTTPS)
                      .authority(DlinkIntentProcessor.HOST)
                      .path(DlinkBookmarkCatalogueIntentProcessor.CATALOGUE)
                      .build().toString();
      return StatisticMapTaskWrapper.wrap(new ImportBookmarkCatalogueTask(url));
    }

    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      if (!super.isSupported(intent))
        return false;

      Uri data = intent.getData();
      if (data == null)
        return false;

      String host = data.getHost();
      return DlinkBookmarkCatalogueIntentProcessor.CATALOGUE.equals(host);
    }
  }

  private static class OldCoreLinkAdapterProcessor extends DlinkIntentProcessor
  {
    private static final String SCHEME_CORE = "mapsme";

    @NonNull
    @Override
    protected MapTask createTargetTask(@NonNull String url)
    {
      // Transform deeplink to the core expected format,
      // i.e https://host/path?query -> mapsme:///path?query.
      Uri uri = Uri.parse(url);
      Uri coreUri = uri.buildUpon()
                       .scheme(SCHEME_CORE)
                       .authority("").build();

      String query = coreUri.getLastPathSegment();
      MapTask statisticTask = StatisticMapTaskWrapper.wrap(new OpenUrlTask(coreUri.toString(),
                                                                             TextUtils.isEmpty(query)
                                                                             ? Statistics.ParamValue.UNKNOWN :
                                                                             query));
      return BackUrlMapTaskWrapper.wrap(statisticTask, url);
    }

    @Override
    boolean isLinkSupported(@NonNull Uri data)
    {
      return true;
    }

    @Nullable
    @Override
    MapTask createIntroductionTask(@NonNull String url)
    {
      return null;
    }
  }

  private static abstract class DlinkIntentProcessor extends LogIntentProcessor
  {
    static final String SCHEME_HTTPS = "https";
    static final String HOST = "dlink.maps.me";
    static final String HOST_DEV = "dlink.mapsme.devmail.ru";

    @Override
    public final boolean isSupported(@NonNull Intent intent)
    {
      final Uri data = intent.getData();

      if (data == null)
        return false;

      String scheme = intent.getScheme();
      String host = data.getHost();

      return SCHEME_HTTPS.equals(scheme) && (HOST.equals(host) || HOST_DEV.equals(host)) &&
          isLinkSupported(data);
    }

    abstract boolean isLinkSupported(@NonNull Uri data);

    @NonNull
    @Override
    final MapTask createMapTask(@NonNull String url)
    {
      if (isFirstLaunch())
      {
        MapTask introductionTask = createIntroductionTask(url);
        if (introductionTask != null)
          return introductionTask;
      }

      return createTargetTask(url);
    }

    @Nullable
    abstract MapTask createIntroductionTask(@NonNull String url);
    @NonNull
    abstract MapTask createTargetTask(@NonNull String url);
  }

  private static class OpenCountryTaskProcessor implements IntentProcessor
  {
    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      return intent.hasExtra(DownloadResourcesLegacyActivity.EXTRA_COUNTRY);
    }

    @NonNull
    @Override
    public MapTask process(@NonNull Intent intent)
    {
      String countryId = intent.getStringExtra(DownloadResourcesLegacyActivity.EXTRA_COUNTRY);

      org.alohalytics.Statistics.logEvent("OpenCountryTaskProcessor::process",
                                          new String[] { "autoDownload", "false" },
                                          LocationHelper.INSTANCE.getSavedLocation());
      return StatisticMapTaskWrapper.wrap(new ShowCountryTask(countryId));
    }
  }

  private static class KmzKmlProcessor implements IntentProcessor
  {
    private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
    private static final String TAG = KmzKmlProcessor.class.getSimpleName();
    private Uri mData;
    @NonNull
    private final DownloadResourcesLegacyActivity mActivity;

    KmzKmlProcessor(@NonNull DownloadResourcesLegacyActivity activity)
    {
      mActivity = activity;
    }

    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      mData = intent.getData();
      return mData != null;
    }

    @Nullable
    @Override
    public MapTask process(@NonNull Intent intent)
    {
      ThreadPool.getStorage().execute(() -> {
        readKmzFromIntent();
        mActivity.runOnUiThread(mActivity::showMap);
      });
      return null;
    }

    private void readKmzFromIntent()
    {
      String path = null;
      boolean isTemporaryFile = false;
      final String scheme = mData.getScheme();
      if (scheme != null && !scheme.equalsIgnoreCase(ContentResolver.SCHEME_FILE))
      {
        // scheme is "content" or "http" - need to download or read file first
        InputStream input = null;
        OutputStream output = null;

        try
        {
          final ContentResolver resolver = mActivity.getContentResolver();
          final String ext = getExtensionFromMime(resolver.getType(mData));
          if (ext != null)
          {
            final String filePath = StorageUtils.getTempPath(mActivity.getApplication())
                                    + "Attachment" + ext;

            File tmpFile = new File(filePath);
            output = new FileOutputStream(tmpFile);
            input = resolver.openInputStream(mData);

            final byte buffer[] = new byte[Constants.MB / 2];
            int read;
            while ((read = input.read(buffer)) != -1)
              output.write(buffer, 0, read);
            output.flush();

            path = filePath;
            isTemporaryFile = true;
          }
        } catch (final Exception ex)
        {
          LOGGER.w(TAG, "Attachment not found or io error: " + ex, ex);
        } finally
        {
          Utils.closeSafely(input);
          Utils.closeSafely(output);
        }
      }
      else
        path = mData.getPath();

      if (!TextUtils.isEmpty(path))
      {
        LOGGER.d(TAG, "Loading bookmarks file from: " + path);
        loadKmzFile(path, isTemporaryFile);
      }
      else
      {
        LOGGER.w(TAG, "Can't get bookmarks file from URI: " + mData);

      }
    }

    private void loadKmzFile(final @NonNull String path, final boolean isTemporaryFile)
    {
      mActivity.runOnUiThread(() -> BookmarkManager.INSTANCE.loadKmzFile(path, isTemporaryFile));
    }

    private String getExtensionFromMime(String mime)
    {
      final int i = mime.lastIndexOf('.');
      if (i == -1)
        return null;

      mime = mime.substring(i + 1);
      if (mime.equalsIgnoreCase("kmz"))
        return ".kmz";
      else if (mime.equalsIgnoreCase("kml+xml"))
        return ".kml";
      else
        return null;
    }
  }

  private static class ShowOnMapProcessor implements IntentProcessor
  {
    private static final String ACTION_SHOW_ON_MAP = "com.mapswithme.maps.pro.action.SHOW_ON_MAP";
    private static final String EXTRA_LAT = "lat";
    private static final String EXTRA_LON = "lon";

    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      return ACTION_SHOW_ON_MAP.equals(intent.getAction());
    }

    @NonNull
    @Override
    public MapTask process(@NonNull Intent intent)
    {
      if (!intent.hasExtra(EXTRA_LAT) || !intent.hasExtra(EXTRA_LON))
        throw new AssertionError("Extra lat/lon must be provided!");

      double lat = getCoordinateFromIntent(intent, EXTRA_LAT);
      double lon = getCoordinateFromIntent(intent, EXTRA_LON);

      return StatisticMapTaskWrapper.wrap(new ShowPointTask(lat, lon));
    }
  }

  private static class BuildRouteProcessor implements IntentProcessor
  {
    private static final String ACTION_BUILD_ROUTE = "com.mapswithme.maps.pro.action.BUILD_ROUTE";
    private static final String EXTRA_LAT_TO = "lat_to";
    private static final String EXTRA_LON_TO = "lon_to";
    private static final String EXTRA_LAT_FROM = "lat_from";
    private static final String EXTRA_LON_FROM = "lon_from";
    private static final String EXTRA_SADDR = "saddr";
    private static final String EXTRA_DADDR = "daddr";
    private static final String EXTRA_ROUTER = "router";

    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      return ACTION_BUILD_ROUTE.equals(intent.getAction());
    }

    @NonNull
    @Override
    public MapTask process(@NonNull Intent intent)
    {
      if (!intent.hasExtra(EXTRA_LAT_TO) || !intent.hasExtra(EXTRA_LON_TO))
        throw new AssertionError("Extra lat/lon must be provided!");

      String saddr = intent.getStringExtra(EXTRA_SADDR);
      String daddr = intent.getStringExtra(EXTRA_DADDR);
      double latTo = getCoordinateFromIntent(intent, EXTRA_LAT_TO);
      double lonTo = getCoordinateFromIntent(intent, EXTRA_LON_TO);
      boolean hasFrom = intent.hasExtra(EXTRA_LAT_FROM) && intent.hasExtra(EXTRA_LON_FROM);
      boolean hasRouter = intent.hasExtra(EXTRA_ROUTER);

      MapTask mapTaskToForward;
      if (hasFrom && hasRouter)
      {
        double latFrom = getCoordinateFromIntent(intent, EXTRA_LAT_FROM);
        double lonFrom = getCoordinateFromIntent(intent, EXTRA_LON_FROM);
        mapTaskToForward = new BuildRouteTask(latTo, lonTo, saddr, latFrom,lonFrom,
                                                           daddr, intent.getStringExtra(EXTRA_ROUTER));
      }
      else if (hasFrom)
      {
        double latFrom = getCoordinateFromIntent(intent, EXTRA_LAT_FROM);
        double lonFrom = getCoordinateFromIntent(intent, EXTRA_LON_FROM);
        mapTaskToForward = new BuildRouteTask(latTo, lonTo, saddr,
                                                           latFrom,lonFrom, daddr);
      }
      else
      {
        mapTaskToForward = new BuildRouteTask(latTo, lonTo,
                                                           intent.getStringExtra(EXTRA_ROUTER));
      }

      return mapTaskToForward;
    }
  }

  private static double getCoordinateFromIntent(@NonNull Intent intent, @NonNull String key)
  {
    double value = intent.getDoubleExtra(key, 0.0);
    if (Double.compare(value, 0.0) == 0)
      value = intent.getFloatExtra(key, 0.0f);

    return value;
  }

  public static class BookmarksSubscriptionTask extends UrlTaskWithStatistics
  {
    private static final long serialVersionUID = 8378582625122063605L;

    BookmarksSubscriptionTask(@NonNull String url)
    {
      super(url);
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      Uri uri = Uri.parse(getUrl());
      String serverId = uri.getQueryParameter(PurchaseUtils.GROUPS);
      if (TextUtils.isEmpty(serverId))
        return false;

      SubscriptionType type = SubscriptionType.getTypeByBookmarksGroup(serverId);

      if (type.equals(SubscriptionType.BOOKMARKS_ALL))
      {
        BookmarksAllSubscriptionActivity.startForResult(target);
        return true;
      }

      if (type.equals(SubscriptionType.BOOKMARKS_SIGHTS))
      {
        BookmarksSightsSubscriptionActivity.startForResult(target);
        return true;
      }

      return false;
    }

    @NonNull
    @Override
    public String toStatisticValue()
    {
      return "subscription";
    }
  }

  public static class ImportBookmarkCatalogueTask extends UrlTaskWithStatistics
  {
    private static final long serialVersionUID = 5363722491377575159L;

    ImportBookmarkCatalogueTask(@NonNull String url)
    {
      super(url);
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      BookmarkCategoriesActivity.startForResult(target, BookmarksPageFactory.DOWNLOADED.ordinal(), getUrl());
      return true;
    }

    @NonNull
    @Override
    public String toStatisticValue()
    {
      return "catalogue";
    }
  }

  public static class GuidesPageToOpenTask extends UrlTaskWithStatistics
  {
    private static final long serialVersionUID = 8388101038319062165L;

    GuidesPageToOpenTask(@NonNull String url)
    {
      super(url);
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      String deeplink = convertUrlToGuidesPageDeeplink(getUrl());
      BookmarksCatalogActivity.startForResult(target,
                                              BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY,
                                              deeplink);
      return true;
    }

    @NonNull
    @Override
    public String toStatisticValue()
    {
      return "guides_page";
    }
  }

  static class FreeGuideReadyToDownloadIntroductionTask extends BaseUrlTask
  {
    private static final long serialVersionUID = -6851782210156017186L;

    FreeGuideReadyToDownloadIntroductionTask(@NonNull String url)
    {
      super(url);
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      target.showIntroductionScreenForDeeplink(getUrl(), IntroductionScreenFactory.FREE_GUIDE);
      return true;
    }
  }

  public static class GuidesPageToOpenIntroductionTask extends BaseUrlTask
  {
    private static final long serialVersionUID = 8388101038319062165L;

    GuidesPageToOpenIntroductionTask(@NonNull String url)
    {
      super(url);
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      String deeplink = convertUrlToGuidesPageDeeplink(getUrl());
      target.showIntroductionScreenForDeeplink(deeplink, IntroductionScreenFactory.GUIDES_PAGE);
      return true;
    }
  }

  abstract static class BaseUrlTask implements MapTask
  {
    private static final long serialVersionUID = 9077126080900672394L;
    @NonNull
    private final String mUrl;

    BaseUrlTask(@NonNull String url)
    {
      mUrl = url;
    }

    @NonNull
    String getUrl()
    {
      return mUrl;
    }
  }

  abstract static class UrlTaskWithStatistics extends MapTaskWithStatistics
  {
    private static final long serialVersionUID = -8661639898700431066L;
    @NonNull
    private final String mUrl;

    UrlTaskWithStatistics(@NonNull String url)
    {
      Utils.checkNotNull(url);
      mUrl = url;
    }

    @Override
    @NonNull
    public Statistics.ParameterBuilder toStatisticParams()
    {
      return Statistics.makeParametersFromTypeAndUrl(toStatisticValue(), mUrl);
    }

    @NonNull
    String getUrl()
    {
      return mUrl;
    }
  }

  public static class OpenUrlTask extends UrlTaskWithStatistics
  {
    private static final long serialVersionUID = -7257820771228127413L;
    private static final int SEARCH_IN_VIEWPORT_ZOOM = 16;

    @NonNull
    private final String mStatisticValue;

    OpenUrlTask(@NonNull String url, @NonNull String statisticValue)
    {
      super(url);
      mStatisticValue = statisticValue;
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

      switch (result.getUrlType())
      {
        case ParsingResult.TYPE_INCORRECT:
        case ParsingResult.TYPE_CATALOGUE:
        case ParsingResult.TYPE_CATALOGUE_PATH:
        case ParsingResult.TYPE_SUBSCRIPTION:
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
          final ParsedSearchRequest request = Framework.nativeGetParsedSearchRequest();
          if (request.mLat != 0.0 || request.mLon != 0.0)
          {
            Framework.nativeStopLocationFollow();
            Framework.nativeSetViewportCenter(request.mLat, request.mLon, SEARCH_IN_VIEWPORT_ZOOM,
                                              false);
            // We need to update viewport for search api manually because of drape engine
            // will not notify subscribers when search activity is shown.
            if (!request.mIsSearchOnMap)
              Framework.nativeSetSearchViewport(request.mLat, request.mLon, SEARCH_IN_VIEWPORT_ZOOM);
          }
          SearchActivity.start(target, request.mQuery, request.mLocale, request.mIsSearchOnMap,
                               null, null);
          return true;
        case ParsingResult.TYPE_LEAD:
          return true;
      }

      return false;
    }

    @NonNull
    @Override
    public String toStatisticValue()
    {
      return mStatisticValue;
    }
  }

  public static class ShowCountryTask extends MapTaskWithStatistics
  {
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

    @NonNull
    @Override
    public String toStatisticValue()
    {
      return "open_country";
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

  public static class ShowPointTask extends MapTaskWithStatistics
  {
    private static final long serialVersionUID = -2467635346469323664L;
    private final double mLat;
    private final double mLon;

    ShowPointTask(double lat, double lon)
    {
      mLat = lat;
      mLon = lon;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      MapFragment.nativeShowMapForUrl(String.format(Locale.US,
                                                    "mapsme://map?ll=%f,%f", mLat, mLon));
      return true;
    }

    @NonNull
    @Override
    public String toStatisticValue()
    {
      return "show_on_map_intent";
    }
  }

  public static class BuildRouteTask extends MapTaskWithStatistics
  {
    private static final long serialVersionUID = 5301468481040195957L;
    private final double mLatTo;
    private final double mLonTo;
    @Nullable
    private final Double mLatFrom;
    @Nullable
    private final Double mLonFrom;
    @Nullable
    private final String mSaddr;
    @Nullable
    private final String mDaddr;
    private final String mRouter;

    @NonNull
    private static MapObject fromLatLon(double lat, double lon, @Nullable String addr)
    {
      return MapObject.createMapObject(FeatureId.EMPTY, MapObject.API_POINT,
                                       TextUtils.isEmpty(addr) ? "" : addr, "", lat, lon);
    }

    BuildRouteTask(double latTo, double lonTo, @Nullable String router)
    {
      this(latTo, lonTo, null, null, null, null, router);
    }

    BuildRouteTask(double latTo, double lonTo, @Nullable String saddr,
                   @Nullable Double latFrom, @Nullable Double lonFrom, @Nullable String daddr)
    {
      this(latTo, lonTo, saddr, latFrom, lonFrom, daddr, null);
    }

    BuildRouteTask(double latTo, double lonTo, @Nullable String saddr,
                   @Nullable Double latFrom, @Nullable Double lonFrom, @Nullable String daddr,
                   @Nullable String router)
    {
      mLatTo = latTo;
      mLonTo = lonTo;
      mLatFrom = latFrom;
      mLonFrom = lonFrom;
      mSaddr = saddr;
      mDaddr = daddr;
      mRouter = router;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      @Framework.RouterType int routerType = -1;
      if (!TextUtils.isEmpty(mRouter))
      {
        switch (mRouter)
        {
          case "vehicle":
            routerType = Framework.ROUTER_TYPE_VEHICLE;
            break;
          case "pedestrian":
            routerType = Framework.ROUTER_TYPE_PEDESTRIAN;
            break;
          case "bicycle":
            routerType = Framework.ROUTER_TYPE_BICYCLE;
            break;
          case "taxi":
            routerType = Framework.ROUTER_TYPE_TAXI;
            break;
          case "transit":
            routerType = Framework.ROUTER_TYPE_TRANSIT;
            break;
        }
      }

      if (mLatFrom != null && mLonFrom != null && routerType >= 0)
      {
        RoutingController.get().prepare(fromLatLon(mLatFrom, mLonFrom, mSaddr),
                                        fromLatLon(mLatTo, mLonTo, mDaddr), routerType,
                                        true /* fromApi */);
      }
      else if (mLatFrom != null && mLonFrom != null)
      {
        RoutingController.get().prepare(fromLatLon(mLatFrom, mLonFrom, mSaddr),
                                        fromLatLon(mLatTo, mLonTo, mDaddr), true /* fromApi */);
      }
      else if (routerType > 0)
      {
        RoutingController.get().prepare(true /* canUseMyPositionAsStart */,
                                        fromLatLon(mLatTo, mLonTo, mDaddr), routerType,
                                        true /* fromApi */);
      }
      else
      {
        RoutingController.get().prepare(true /* canUseMyPositionAsStart */,
                                        fromLatLon(mLatTo, mLonTo, mDaddr), true /* fromApi */);
      }
      return true;
    }

    @NonNull
    @Override
    public String toStatisticValue()
    {
      return "build_route_intent";
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

  public static class ShowUGCEditorTask implements MapTask
  {
    private static final long serialVersionUID = 1636712824900113568L;
    // Nullable because of possible serialization from previous incompatible version of class.
    @Nullable
    private final NotificationCandidate.UgcReview mNotificationCandidate;

    public ShowUGCEditorTask(@Nullable NotificationCandidate.UgcReview notificationCandidate)
    {
      mNotificationCandidate = notificationCandidate;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      if (mNotificationCandidate == null)
        return false;

      MapObject mapObject = Framework.nativeGetMapObject(mNotificationCandidate);

      if (mapObject == null)
        return false;

      EditParams.Builder builder = EditParams.Builder.fromMapObject(mapObject)
                                                     .setDefaultRating(UGC.RATING_NONE)
                                                     .setFromNotification(true);
      UGCEditorActivity.start(target, builder.build());
      return true;
    }
  }

  public static class ShowDialogTask implements MapTask
  {
    private static final long serialVersionUID = 1548931513812565018L;
    @NonNull
    private final String mDialogName;
    @NonNull
    private final ArrayList<KeyValue> mKeyValues;

    public ShowDialogTask(@NonNull String dialogName, @NonNull ArrayList<KeyValue> keyValues)
    {
      mDialogName = dialogName;
      mKeyValues = keyValues;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      Fragment f = target.getSupportFragmentManager().findFragmentByTag(mDialogName);
      if (f != null)
        return true;

      final DialogFragment fragment = (DialogFragment) Fragment.instantiate(target, mDialogName);
      fragment.setArguments(toDialogArgs(mKeyValues));
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

  public static class ShowTutorialTask implements MapTask
  {
    private static final long serialVersionUID = -7565474616748655191L;

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      Tutorial tutorial = Tutorial.requestCurrent(target, target.getClass());
      if (tutorial == Tutorial.STUB)
        return false;

      if (target.getTutorial() != null)
        return false;

      target.setTutorial(tutorial);
      tutorial.show(target, target);

      Statistics.INSTANCE.trackTipsEvent(Statistics.EventName.TIPS_TRICKS_SHOW,
                                         tutorial.ordinal());
      return true;
    }
  }
}
