package com.mapswithme.maps.Ads;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.text.TextUtils;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Constants;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class AdsManager
{
  private static final String ROOT_MENU_ITEMS_KEY = "AppFeatureBottomMenuItems";
  private static final String ROOT_BANNERS_KEY = "AppFeatureBanners";
  private static final String MENU_ITEMS_KEY = "Items";
  private static final String DEFAULT_KEY = "*";
  private static final String ID_KEY = "Id";
  private static final String APP_URL_KEY = "AppURL";
  private static final String ICON_KEY = "IconURLs";
  private static final String TITLE_KEY = "Titles";
  private static final String COLOR_KEY = "Color";
  private static final String WEB_URL_KEY = "WebURLs";
  private static final String CACHE_FILE = "menu_ads.json";
  private static final String ID_APP_PACKAGE = "AppPackage";
  private static final String SHOW_LITE_KEY = "ShowInLite";
  private static final String SHOW_PRO_KEY = "ShowInPro";
  private static final String FG_TIME_KEY = "ForegroundTime";
  private static final String LAUNCH_NUM_KEY = "LaunchNumber";
  private static final String APP_VERSION_KEY = "AppVersion";
  private static final String BANNER_URL_KEY = "Url";

  private static List<MenuAd> sMenuAds;
  private static List<Banner> sBanners;

  public static List<MenuAd> getMenuAds()
  {
    return sMenuAds;
  }

  public static List<Banner> getBanners()
  {
    return sBanners;
  }

  public static void updateFeatures()
  {
    String featuresString = null;
    if (ConnectionState.isConnected())
    {
      featuresString = getJsonAdsFromServer();
      cacheFeatures(featuresString);
    }

    if (featuresString == null)
      featuresString = getCachedJsonString();

    if (featuresString == null)
      return;

    JSONObject featuresJson = null;
    try
    {
      featuresJson = new JSONObject(featuresString);
      sMenuAds = parseMenuAds(featuresJson);
    } catch (JSONException e)
    {
      e.printStackTrace();
    }

    try
    {
      sBanners = parseBanners(featuresJson);
    } catch (JSONException e)
    {
      e.printStackTrace();
    }
  }

  private static void cacheFeatures(String featuresString)
  {
    if (featuresString == null)
      return;

    final File cacheFile = new File(MWMApplication.get().getDataStoragePath(), CACHE_FILE);
    FileOutputStream fileOutputStream = null;
    try
    {
      fileOutputStream = new FileOutputStream(cacheFile);
      fileOutputStream.write(featuresString.getBytes());
      fileOutputStream.close();
    } catch (IOException e)
    {
      e.printStackTrace();
    } finally
    {
      Utils.closeStream(fileOutputStream);
    }
  }

  private static String getCachedJsonString()
  {
    String menuAdsString = null;
    final File cacheFile = new File(MWMApplication.get().getDataStoragePath(), CACHE_FILE);
    BufferedReader reader = null;
    try
    {
      reader = new BufferedReader(new FileReader(cacheFile));
      final StringBuilder stringBuilder = new StringBuilder();
      String line;
      while ((line = reader.readLine()) != null)
        stringBuilder.append(line);

      menuAdsString = stringBuilder.toString();
    } catch (IOException e)
    {
      e.printStackTrace();
    } finally
    {
      Utils.closeStream(reader);
    }

    return menuAdsString;
  }

  private static List<MenuAd> parseMenuAds(JSONObject adsJson) throws JSONException
  {
    if (adsJson == null)
      return null;

    final List<MenuAd> ads = new ArrayList<>();

    final String countryCode = Locale.getDefault().getCountry();
    final JSONArray menuItemsJson = getJsonObjectByKeyOrDefault(adsJson.getJSONObject(ROOT_MENU_ITEMS_KEY), countryCode).
        getJSONArray(MENU_ITEMS_KEY);

    final String localeKey = Locale.getDefault().getLanguage();
    final String density = UiUtils.getDisplayDensityString();
    for (int i = 0; i < menuItemsJson.length(); i++)
    {
      final JSONObject menuItemJson = menuItemsJson.getJSONObject(i);
      final String icon = getStringByKeyOrDefault(menuItemJson.getJSONObject(ICON_KEY), density);
      final String webUrl = getStringByKeyOrDefault(menuItemJson.getJSONObject(WEB_URL_KEY), localeKey);
      final String title = getStringByKeyOrDefault(menuItemJson.getJSONObject(TITLE_KEY), localeKey);
      final String id = menuItemJson.getString(ID_KEY);
      final Bitmap bitmap = loadAdIcon(icon, id);
      final String appPackage = menuItemJson.optString(ID_APP_PACKAGE);

      ads.add(new MenuAd(icon,
          title,
          menuItemJson.getString(COLOR_KEY),
          id,
          menuItemJson.getString(APP_URL_KEY),
          webUrl,
          bitmap,
          appPackage));
    }

    return ads;
  }

  private static List<Banner> parseBanners(JSONObject rootJson) throws JSONException
  {
    if (rootJson == null)
      return null;
    final ArrayList<Banner> banners = new ArrayList<>();

    final String countryCode = Locale.getDefault().getCountry();
    final JSONArray featuresJson = getJsonObjectByKeyOrDefault(rootJson.getJSONObject(ROOT_BANNERS_KEY), countryCode).
        getJSONArray(MENU_ITEMS_KEY);

    for (int i = 0; i < featuresJson.length(); i++)
    {
      final JSONObject featureJson = featuresJson.getJSONObject(i);
      final String url = featureJson.optString(BANNER_URL_KEY);
      final boolean showLite = featureJson.optBoolean(SHOW_LITE_KEY, false);
      final boolean showPro = featureJson.optBoolean(SHOW_PRO_KEY, false);
      final int fgTime = featureJson.optInt(FG_TIME_KEY, 0);
      final int launchNum = featureJson.optInt(LAUNCH_NUM_KEY, 0);
      final int appVersion = featureJson.optInt(APP_VERSION_KEY, 0);
      final String id = featureJson.optString(ID_KEY);

      banners.add(new Banner(url, showLite, showPro, launchNum, fgTime, appVersion, id));
    }

    return banners;
  }

  /**
   * Loads and caches ad icon. If internet isnt connected tries to reuse cached icon.
   *
   * @param urlString url of the icon
   * @param adId      name of cachefile
   * @return
   */
  private static Bitmap loadAdIcon(String urlString, String adId)
  {
    if (!ConnectionState.isConnected())
      return loadCachedBitmap(adId);

    InputStream inputStream = null;
    try
    {
      final URL url = new java.net.URL(urlString);
      final HttpURLConnection connection = (HttpURLConnection) url
          .openConnection();
      final int timeout = 5000;
      connection.setReadTimeout(timeout);
      connection.setConnectTimeout(timeout);
      connection.setDoInput(true);
      connection.connect();
      inputStream = connection.getInputStream();
      final Bitmap bitmap = BitmapFactory.decodeStream(inputStream);
      cacheBitmap(bitmap, adId);
      return bitmap;
    } catch (IOException e)
    {
      e.printStackTrace();
    } finally
    {
      Utils.closeStream(inputStream);
    }

    return null;
  }

  private static void cacheBitmap(Bitmap bitmap, String fileName)
  {
    final File cacheFile = new File(MWMApplication.get().getDataStoragePath(), fileName);
    FileOutputStream fileOutputStream = null;
    try
    {
      fileOutputStream = new FileOutputStream(cacheFile);
      bitmap.compress(Bitmap.CompressFormat.PNG, 100, fileOutputStream);
      fileOutputStream.close();
    } catch (IOException e)
    {
      e.printStackTrace();
    } finally
    {
      Utils.closeStream(fileOutputStream);
    }
  }

  private static Bitmap loadCachedBitmap(String fileName)
  {
    final File cacheFile = new File(MWMApplication.get().getDataStoragePath(), fileName);
    if (!cacheFile.exists())
      return null;

    InputStream inputStream = null;
    try
    {
      inputStream = new FileInputStream(cacheFile);
      return BitmapFactory.decodeStream(inputStream);
    } catch (IOException e)
    {
      e.printStackTrace();
    } finally
    {
      Utils.closeStream(inputStream);
    }

    return null;
  }

  private static String getStringByKeyOrDefault(JSONObject json, String key) throws JSONException
  {
    String res = json.optString(key);
    if (TextUtils.isEmpty(res))
      res = json.optString(DEFAULT_KEY);

    return res;
  }

  private static JSONObject getJsonObjectByKeyOrDefault(JSONObject json, String key) throws JSONException
  {
    JSONObject res = json.optJSONObject(key);
    if (res == null)
      res = json.getJSONObject(DEFAULT_KEY);

    return res;
  }

  private static String getJsonAdsFromServer()
  {
    BufferedReader reader = null;
    HttpURLConnection connection = null;
    try
    {
      final URL url = new URL(Constants.Url.FEATURES_JSON);
      connection = (HttpURLConnection) url.openConnection();
      final int timeout = 30000;
      connection.setReadTimeout(timeout);
      connection.setConnectTimeout(timeout);
      connection.setRequestMethod("GET");
      connection.setDoInput(true);
      // Starts the query
      connection.connect();
      final int response = connection.getResponseCode();
      if (response == HttpURLConnection.HTTP_OK)
      {
        reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
        final StringBuilder builder = new StringBuilder();
        String line;
        while ((line = reader.readLine()) != null)
          builder.append(line).append("\n");

        return builder.toString();
      }
    } catch (java.io.IOException e)
    {
      e.printStackTrace();
    } finally
    {
      Utils.closeStream(reader);
      if (connection != null)
        connection.disconnect();
    }

    return null;
  }

  private static boolean shouldShowBanner(Banner banner)
  {
    final MWMApplication application = MWMApplication.get();
    return ((ConnectionState.isConnected()) &&
        banner.getShowInPro() &&
        (BuildConfig.VERSION_CODE >= banner.getAppVersion()) &&
        (application.nativeGetBoolean("ShouldShow" + banner.getId(), true)) &&
        (application.getForegroundTime() >= banner.getFgTime()) &&
        (application.getLaunchesNumber() >= banner.getLaunchNumber()));
  }

  public static Banner getBannerToShow()
  {
    if (sBanners == null || sBanners.isEmpty())
      return null;

    for (Banner banner : sBanners)
    {
      if (shouldShowBanner(banner))
      {
        MWMApplication.get().nativeSetBoolean("ShouldShow" + banner.getId(), false);
        return banner;
      }
    }

    return null;
  }
}
