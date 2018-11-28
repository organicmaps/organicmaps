package com.mapswithme.maps.intent;

import android.content.ContentResolver;
import android.content.Intent;
import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import com.crashlytics.android.Crashlytics;
import com.mapswithme.maps.DownloadResourcesLegacyActivity;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.api.Const;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.util.Constants;
import com.mapswithme.util.StorageUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

public class Factory
{
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

  private static abstract class LogIntentProcessor implements IntentProcessor
  {
    private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
    @NonNull
    @Override
    public final MwmActivity.MapTask process(@NonNull Intent intent)
    {
      Uri data = intent.getData();
      if (data == null)
        throw new AssertionError("Data must be non-null!");
      final String uri = data.toString();
      String msg = this.getClass().getSimpleName() + ": incoming intent uri: " + uri;
      LOGGER.i(this.getClass().getSimpleName(), msg);
      org.alohalytics.Statistics.logEvent(msg);
      Crashlytics.log(msg);
      return createMapTask(uri);
    }

    @NonNull
    abstract MwmActivity.MapTask createMapTask(@NonNull String uri);
  }

  private static abstract class BaseOpenUrlProcessor extends LogIntentProcessor
  {
    @NonNull
    @Override
    MwmActivity.MapTask createMapTask(@NonNull String uri)
    {
      return new MwmActivity.OpenUrlTask(uri);
    }
  }

  private static class GeoIntentProcessor extends BaseOpenUrlProcessor
  {
    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      return (intent.getData() != null && "geo".equals(intent.getScheme()));
    }
  }

  private static class Ge0IntentProcessor extends BaseOpenUrlProcessor
  {
    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      return (intent.getData() != null && "ge0".equals(intent.getScheme()));
    }
  }

  private static class MapsmeProcessor extends BaseOpenUrlProcessor
  {
    @Override
    public boolean isSupported(@NonNull Intent intent)
    {
      return "mapsme".equals(intent.getScheme());
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
    public MwmActivity.MapTask process(@NonNull Intent intent)
    {
      final Uri data = intent.getData();
      final String ge0Url = "ge0:/" + data.getPath();
      org.alohalytics.Statistics.logEvent("HttpGe0IntentProcessor::process", ge0Url);
      return new MwmActivity.OpenUrlTask(ge0Url);
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
    public MwmActivity.MapTask process(@NonNull final Intent intent)
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
          return new MwmActivity.OpenUrlTask(apiUrl);
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
  }

  private static class DlinkBookmarkCatalogueIntentProcessor extends DlinkIntentProcessor
  {
    static final String CATALOGUE = "catalogue";

    @Override
    boolean isLinkSupported(@NonNull Uri data)
    {
      return (File.separator + CATALOGUE).equals(data.getPath());
    }

    @NonNull
    @Override
    MwmActivity.MapTask createMapTask(@NonNull String url)
    {
      return new MwmActivity.ImportBookmarkCatalogueTask(url);
    }
  }

  private static class MapsmeBookmarkCatalogueProcessor extends MapsmeProcessor
  {
    @NonNull
    @Override
    MwmActivity.MapTask createMapTask(@NonNull String uri)
    {
      String url = Uri.parse(uri).buildUpon()
                      .scheme(DlinkIntentProcessor.SCHEME_HTTPS)
                      .authority(DlinkIntentProcessor.HOST)
                      .path(DlinkBookmarkCatalogueIntentProcessor.CATALOGUE)
                      .build().toString();
      return new MwmActivity.ImportBookmarkCatalogueTask(url);
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

    @Override
    protected boolean isLinkSupported(@NonNull Uri data)
    {
      return true;
    }

    @NonNull
    @Override
    protected MwmActivity.MapTask createMapTask(@NonNull String url)
    {
      // Transform deeplink to the core expected format,
      // i.e https://host/path?query -> mapsme:///path?query.
      Uri uri = Uri.parse(url);
      Uri coreUri = uri.buildUpon()
                       .scheme(SCHEME_CORE)
                       .authority("").build();
      return new MwmActivity.OpenUrlTask(coreUri.toString());
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
    public MwmActivity.MapTask process(@NonNull Intent intent)
    {
      String countryId = intent.getStringExtra(DownloadResourcesLegacyActivity.EXTRA_COUNTRY);

      org.alohalytics.Statistics.logEvent("OpenCountryTaskProcessor::process",
                                          new String[] { "autoDownload", "false" },
                                          LocationHelper.INSTANCE.getSavedLocation());
      return new MwmActivity.ShowCountryTask(countryId);
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
    public MwmActivity.MapTask process(@NonNull Intent intent)
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
    public MwmActivity.MapTask process(@NonNull Intent intent)
    {
      if (!intent.hasExtra(EXTRA_LAT) || !intent.hasExtra(EXTRA_LON))
        throw new AssertionError("Extra lat/lon must be provided!");

      double lat = getCoordinateFromIntent(intent, EXTRA_LAT);
      double lon = getCoordinateFromIntent(intent, EXTRA_LON);

      return  new MwmActivity.ShowPointTask(lat, lon);
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
    public MwmActivity.MapTask process(@NonNull Intent intent)
    {
      if (!intent.hasExtra(EXTRA_LAT_TO) || !intent.hasExtra(EXTRA_LON_TO))
        throw new AssertionError("Extra lat/lon must be provided!");

      String saddr = intent.getStringExtra(EXTRA_SADDR);
      String daddr = intent.getStringExtra(EXTRA_DADDR);
      double latTo = getCoordinateFromIntent(intent, EXTRA_LAT_TO);
      double lonTo = getCoordinateFromIntent(intent, EXTRA_LON_TO);
      boolean hasFrom = intent.hasExtra(EXTRA_LAT_FROM) && intent.hasExtra(EXTRA_LON_FROM);
      boolean hasRouter = intent.hasExtra(EXTRA_ROUTER);

      MwmActivity.MapTask mapTaskToForward;
      if (hasFrom && hasRouter)
      {
        double latFrom = getCoordinateFromIntent(intent, EXTRA_LAT_FROM);
        double lonFrom = getCoordinateFromIntent(intent, EXTRA_LON_FROM);
        mapTaskToForward = new MwmActivity.BuildRouteTask(latTo, lonTo, saddr, latFrom,lonFrom,
                                                           daddr, intent.getStringExtra(EXTRA_ROUTER));
      }
      else if (hasFrom)
      {
        double latFrom = getCoordinateFromIntent(intent, EXTRA_LAT_FROM);
        double lonFrom = getCoordinateFromIntent(intent, EXTRA_LON_FROM);
        mapTaskToForward = new MwmActivity.BuildRouteTask(latTo, lonTo, saddr,
                                                           latFrom,lonFrom, daddr);
      }
      else
      {
        mapTaskToForward = new MwmActivity.BuildRouteTask(latTo, lonTo,
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
}
