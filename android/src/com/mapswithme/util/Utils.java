package com.mapswithme.util;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.provider.Settings;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.AbsoluteSizeSpan;
import android.util.AndroidRuntimeException;
import android.view.View;
import android.view.Window;
import android.widget.Toast;

import androidx.annotation.DimenRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AlertDialog;
import androidx.core.app.NavUtils;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import com.google.android.material.snackbar.Snackbar;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.analytics.ExternalLibrariesMediator;
import com.mapswithme.maps.base.CustomNavigateUpListener;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.AlohaHelper;

import java.io.Closeable;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.net.NetworkInterface;
import java.security.MessageDigest;
import java.text.DateFormat;
import java.text.NumberFormat;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collections;
import java.util.Currency;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.TimeZone;

public class Utils
{
  @StringRes
  public static final int INVALID_ID = 0;
  public static final String UTF_8 = "utf-8";
  public static final String BASE_64 = "base64";
  public static final String TEXT_HTML = "text/html;";
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = "Utils";

  private Utils() {}

  public static boolean isJellyBeanOrLater()
  {
    return isTargetOrLater(Build.VERSION_CODES.JELLY_BEAN_MR1);
  }

  public static boolean isLollipopOrLater()
  {
    return isTargetOrLater(Build.VERSION_CODES.LOLLIPOP);
  }

  public static boolean isMarshmallowOrLater()
  {
    return isTargetOrLater(Build.VERSION_CODES.M);
  }

  public static boolean isNougatOrLater()
  {
    return isTargetOrLater(Build.VERSION_CODES.N);
  }

  public static boolean isOreoOrLater()
  {
    return isTargetOrLater(Build.VERSION_CODES.O);
  }

  private static boolean isTargetOrLater(int target)
  {
    return Build.VERSION.SDK_INT >= target;
  }

  public static boolean isAmazonDevice()
  {
    return "Amazon".equalsIgnoreCase(Build.MANUFACTURER);
  }

  /**
   * Enable to keep screen on.
   * Disable to let system turn it off automatically.
   */
  public static void keepScreenOn(boolean enable, Window w)
  {
    if (enable)
      w.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    else
      w.clearFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
  }

  public static void showSnackbar(@NonNull View view, @NonNull String message)
  {
    Snackbar.make(view, message, Snackbar.LENGTH_LONG).show();
  }

  public static void showSnackbarAbove(@NonNull View view, @NonNull View viewAbove, @NonNull String message)
  {
    Snackbar.make(view, message, Snackbar.LENGTH_LONG)
            .setAnchorView(viewAbove).show();
  }

  public static void showSnackbar(@NonNull Context context, @NonNull View view, int messageResId)
  {
    showSnackbar(context, view, null, messageResId);
  }

  public static void showSnackbar(@NonNull Context context, @NonNull View view,
                                  @Nullable View viewAbove, int messageResId)
  {
    final String message = context.getString(messageResId);
    if (viewAbove == null)
      showSnackbar(view, message);
    else
      showSnackbarAbove(view, viewAbove, message);
  }

  public static boolean isIntentSupported(Context context, Intent intent)
  {
    return context.getPackageManager().resolveActivity(intent, 0) != null;
  }

  public static void checkNotNull(Object object)
  {
    if (null == object) throw new NullPointerException("Argument here must not be NULL");
  }

  public static void copyTextToClipboard(Context context, String text)
  {
    final android.content.ClipboardManager clipboard =
        (android.content.ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
    final ClipData clip = ClipData.newPlainText("maps.me: " + text, text);
    clipboard.setPrimaryClip(clip);
  }

  public static <K, V> String mapPrettyPrint(Map<K, V> map)
  {
    if (map == null)
      return "[null]";
    if (map.isEmpty())
      return "[]";


    String joined = "";
    for (final K key : map.keySet())
    {
      final String keyVal = key + "=" + map.get(key);
      if (joined.length() > 0)
        joined = TextUtils.join(",", new Object[]{joined, keyVal});
      else
        joined = keyVal;
    }

    return "[" + joined + "]";
  }

  public static boolean isPackageInstalled(@NonNull Context context, String packageUri)
  {
    PackageManager pm = context.getPackageManager();
    boolean installed;
    try
    {
      pm.getPackageInfo(packageUri, PackageManager.GET_ACTIVITIES);
      installed = true;
    } catch (PackageManager.NameNotFoundException e)
    {
      installed = false;
    }
    return installed;
  }

  public static Uri buildMailUri(String to, String subject, String body)
  {
    String uriString = Constants.Url.MAILTO_SCHEME + Uri.encode(to) +
        Constants.Url.MAIL_SUBJECT + Uri.encode(subject) +
        Constants.Url.MAIL_BODY + Uri.encode(body);

    return Uri.parse(uriString);
  }

  public static String getFullDeviceModel()
  {
    String model = Build.MODEL;
    if (!model.startsWith(Build.MANUFACTURER))
      model = Build.MANUFACTURER + " " + model;

    return model;
  }

  public static void openAppInMarket(Activity activity, String url)
  {
    final Intent marketIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
    marketIntent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
      marketIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
    else
      marketIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

    try
    {
      activity.startActivity(marketIntent);
    } catch (ActivityNotFoundException e)
    {
      AlohaHelper.logException(e);
    }
  }

  public static void showFacebookPage(Activity activity)
  {
    try
    {
      // Exception is thrown if we don't have installed Facebook application.
      activity.getPackageManager().getPackageInfo(Constants.Package.FB_PACKAGE, 0);
      activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.FB_MAPSME_COMMUNITY_NATIVE)));
    } catch (final Exception e)
    {
      activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.FB_MAPSME_COMMUNITY_HTTP)));
    }
  }

  public static void showTwitterPage(Activity activity)
  {
    activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.TWITTER_MAPSME_HTTP)));
  }

  public static void openUrl(@NonNull Context context, @Nullable String url)
  {
    if (TextUtils.isEmpty(url))
      return;

    final Intent intent = new Intent(Intent.ACTION_VIEW);
    Uri uri = isHttpOrHttpsScheme(url)
               ? Uri.parse(url)
               : new Uri.Builder().scheme("http").appendEncodedPath(url).build();
    intent.setData(uri);
    try
    {
      context.startActivity(intent);
    }
    catch (ActivityNotFoundException e)
    {
      CrashlyticsUtils.INSTANCE.logException(e);
    }
    catch (AndroidRuntimeException e)
    {
      CrashlyticsUtils.INSTANCE.logException(e);
      intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      context.startActivity(intent);
    }
  }

  public static boolean openUri(@NonNull Context context, @NonNull Uri uri)
  {
    final Intent intent = new Intent(Intent.ACTION_VIEW);
    intent.setData(uri);
    try
    {
      context.startActivity(intent);
      return true;
    }
    catch (ActivityNotFoundException e)
    {
      CrashlyticsUtils.INSTANCE.logException(e);
      return false;
    }
    catch (AndroidRuntimeException e)
    {
      CrashlyticsUtils.INSTANCE.logException(e);
      intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      context.startActivity(intent);
      return false;
    }
  }

  private static boolean isHttpOrHttpsScheme(@NonNull String url)
  {
    return url.startsWith("http://") || url.startsWith("https://");
  }

  @NonNull
  public static <T> T castTo(@NonNull Object instance)
  {
    // Noinspection unchecked
    return (T) instance;
  }

  public static void closeSafely(@NonNull Closeable... closeable)
  {
    for (Closeable each : closeable)
    {
      if (each != null)
      {
        try
        {
          each.close();
        }
        catch (IOException e)
        {
          LOGGER.e(TAG, "Failed to close '" + each + "'" , e);
        }
      }
    }
  }

  public static void sendBugReport(@NonNull Activity activity, @NonNull String subject)
  {
    LoggerFactory.INSTANCE.zipLogs(new SupportInfoWithLogsCallback(activity, subject,
                                                                   Constants.Email.SUPPORT));
  }

  public static void sendFeedback(@NonNull Activity activity)
  {
    LoggerFactory.INSTANCE.zipLogs(new SupportInfoWithLogsCallback(activity, "Feedback",
                                                                   Constants.Email.FEEDBACK));
  }

  public static void navigateToParent(@Nullable Activity activity)
  {
    if (activity == null)
      return;

    if (activity instanceof CustomNavigateUpListener)
      ((CustomNavigateUpListener) activity).customOnNavigateUp();
    else
      NavUtils.navigateUpFromSameTask(activity);
  }

  public static SpannableStringBuilder formatUnitsText(Context context, @DimenRes int size, @DimenRes int units, String dimension, String unitText)
  {
    final SpannableStringBuilder res = new SpannableStringBuilder(dimension).append(" ").append(unitText);
    res.setSpan(new AbsoluteSizeSpan(UiUtils.dimen(context, size), false), 0, dimension.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    res.setSpan(new AbsoluteSizeSpan(UiUtils.dimen(context, units), false), dimension.length(), res.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    return res;
  }

  public static void checkConnection(final Context context, final @StringRes int message,
                                     final Proc<Boolean> onCheckPassedCallback)
  {
    if (ConnectionState.INSTANCE.isConnected())
    {
      onCheckPassedCallback.invoke(true);
      return;
    }

    class Holder
    {
      boolean accepted;
    }

    final Holder holder = new Holder();
    new AlertDialog.Builder(context)
                   .setMessage(message)
                   .setNegativeButton(android.R.string.cancel, null)
                   .setPositiveButton(R.string.downloader_retry, new DialogInterface.OnClickListener()
                   {
                     @Override
                     public void onClick(DialogInterface dialog, int which)
                     {
                       holder.accepted = true;
                       checkConnection(context, message, onCheckPassedCallback);
                     }
                   }).setOnDismissListener(new DialogInterface.OnDismissListener()
                   {
                     @Override
                     public void onDismiss(DialogInterface dialog)
                     {
                       if (!holder.accepted)
                         onCheckPassedCallback.invoke(false);
                     }
                   }).show();
  }

  @NonNull
  public static String getInstallationId(@NonNull Context context)
  {
    final SharedPreferences sharedPrefs = context.getSharedPreferences(
        org.alohalytics.Statistics.PREF_FILE, Context.MODE_PRIVATE);
    // "UNIQUE_ID" is the value of org.alohalytics.Statistics.PREF_UNIQUE_ID, but it private.
    String installationId = sharedPrefs.getString("UNIQUE_ID", null);

    if (TextUtils.isEmpty(installationId))
      return "";

    return installationId;
  }

  @Nullable
  public static String getAdvertisingId(@NonNull Context context)
  {
    MwmApplication application = MwmApplication.from(context);
    ExternalLibrariesMediator mediator = application.getMediator();
    return mediator.getAdvertisingId();
  }

  @NonNull
  public static String getMacAddress(@NonNull Context context,  boolean md5Decoded)
  {
    byte[] macBytes = null;
    String address = "";
    try
    {
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M)
      {
        WifiManager manager = (WifiManager) context.getApplicationContext().
            getSystemService(Context.WIFI_SERVICE);
        if (manager == null)
          return "";
        WifiInfo info = manager.getConnectionInfo();
        address = info.getMacAddress();
        macBytes = address.getBytes();
      }
      else
      {
        List<NetworkInterface> all = Collections.list(NetworkInterface.getNetworkInterfaces());
        for (NetworkInterface nif : all)
        {
          if (!nif.getName().equalsIgnoreCase("wlan0"))
            continue;

          macBytes = nif.getHardwareAddress();
          if (macBytes == null)
            return "";

          StringBuilder result = new StringBuilder();
          for (int i = 0; i < macBytes.length; i++)
          {
            result.append(String.format("%02X", (0xFF & macBytes[i])));
            if (i + 1 != macBytes.length)
              result.append(":");
          }
          address = result.toString();
        }
      }
    }
    catch (Exception exc)
    {
      return "";
    }
    return md5Decoded ? decodeMD5(macBytes) : address;
  }

  @NonNull
  private static String decodeMD5(@Nullable byte[] bytes)
  {
    if (bytes == null || bytes.length == 0)
      return "";

    try
    {
      MessageDigest digest = java.security.MessageDigest.getInstance("MD5");
      digest.update(bytes);
      byte[] messageDigest = digest.digest();

      StringBuilder hexString = new StringBuilder();
      for (int i = 0; i < messageDigest.length; i++)
        hexString.append(String.format("%02X", (0xFF & messageDigest[i])));
      return hexString.toString();
    }
    catch (Exception e)
    {
      return "";
    }
  }

  public static boolean isAppInstalled(@NonNull Context context, @NonNull String packageName)
  {
    try
    {
      PackageManager pm = context.getPackageManager();
      pm.getPackageInfo(packageName, PackageManager.GET_ACTIVITIES);
      return true;
    } catch (PackageManager.NameNotFoundException e)
    {
      return false;
    }
  }

  private static void launchAppDirectly(@NonNull Context context, @NonNull SponsoredLinks links)
  {
    Intent intent = new Intent(Intent.ACTION_VIEW);
    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    intent.setData(Uri.parse(links.getDeepLink()));
    context.startActivity(intent);
  }

  private static void launchAppIndirectly(@NonNull Context context, @NonNull SponsoredLinks links)
  {
    Intent intent = new Intent(Intent.ACTION_VIEW);
    intent.setData(Uri.parse(links.getDeepLink()));
    context.startActivity(intent);
  }

  public static void openPartner(@NonNull Context activity, @NonNull SponsoredLinks links,
                                 @NonNull String packageName, @NonNull PartnerAppOpenMode openMode)
  {
    switch (openMode)
    {
      case  Direct:
        if (!Utils.isAppInstalled(activity, packageName))
        {
          openUrl(activity, links.getUniversalLink());
          return;
        }
        launchAppDirectly(activity, links);
        break;
      case Indirect:
        launchAppIndirectly(activity, links);
        break;
      default:
        throw new AssertionError("Unsupported partner app open mode: " + openMode +
                                 "; Package name: " + packageName);
    }
  }

  public static void sendTo(@NonNull Context context, @NonNull String email)
  {
    Intent intent = new Intent(Intent.ACTION_SENDTO);
    intent.setData(Utils.buildMailUri(email, "", ""));
    context.startActivity(intent);
  }

  public static void callPhone(@NonNull Context context, @NonNull String phone)
  {
    Intent intent = new Intent(Intent.ACTION_DIAL);
    intent.setData(Uri.parse("tel:" + phone));
    try
    {
      context.startActivity(intent);
    }
    catch (ActivityNotFoundException e)
    {
      LOGGER.e(TAG, "Failed to call phone", e);
      AlohaHelper.logException(e);
    }
  }

  public static void showSystemConnectionSettings(@NonNull Context context)
  {
    try
    {
      context.startActivity(new Intent(Settings.ACTION_WIRELESS_SETTINGS));
    }
    catch (ActivityNotFoundException e)
    {
      LOGGER.e(TAG, "Failed to open system connection settings", e);
      try
      {
        context.startActivity(new Intent(Settings.ACTION_SETTINGS));
      }
      catch (ActivityNotFoundException ex)
      {
        LOGGER.e(TAG, "Failed to open system settings", ex);
      }
    }
  }

  @Nullable
  public static String getCurrencyCode()
  {
    Locale[] locales = { Locale.getDefault(), Locale.US };
    for (Locale locale : locales)
    {
      Currency currency = getCurrencyForLocale(locale);
      if (currency != null)
        return currency.getCurrencyCode();
    }
    return null;
  }

  @NonNull
  public static String getCountryCode()
  {
    return Locale.getDefault().getCountry();
  }

  @NonNull
  public static String getLanguageCode()
  {
    return Locale.getDefault().getLanguage();
  }

  @Nullable
  public static Currency getCurrencyForLocale(@NonNull Locale locale)
  {
    try
    {
      return Currency.getInstance(locale);
    }
    catch (Throwable e)
    {
      LOGGER.e(TAG, "Failed to obtain a currency for locale: " + locale, e);
      return null;
    }
  }

  @NonNull
  public static String formatCurrencyString(@NonNull String price, @NonNull String currencyCode)
  {

    float value = Float.valueOf(price);
    return formatCurrencyString(value, currencyCode);
  }

  @NonNull
  public static String formatCurrencyString(float price, @NonNull String currencyCode)
  {
    String text;
    try
    {
      Locale locale = Locale.getDefault();
      Currency currency = Utils.getCurrencyForLocale(locale);
      // If the currency cannot be obtained for the default locale we will use Locale.US.
      if (currency == null)
        locale = Locale.US;

      if (isNougatOrLater())
      {
        android.icu.text.NumberFormat formatter = android.icu.text.NumberFormat.getInstance(
            locale, android.icu.text.NumberFormat.CURRENCYSTYLE);
        if (!TextUtils.isEmpty(currencyCode))
          formatter.setCurrency(android.icu.util.Currency.getInstance(currencyCode));
        return formatter.format(price);
      }

      NumberFormat formatter = NumberFormat.getCurrencyInstance(locale);
      if (!TextUtils.isEmpty(currencyCode))
        formatter.setCurrency(Currency.getInstance(currencyCode));
      return formatter.format(price);
    }
    catch (Throwable e)
    {
      LOGGER.e(TAG, "Failed to format string for price = " + price
                    + " and currencyCode = " + currencyCode, e);
      text = (price + " " + currencyCode);
    }
    return text;
  }

  @NonNull
  public static String getCurrencySymbol(@NonNull String currencyCode)
  {
    try
    {
      return Currency.getInstance(currencyCode).getSymbol(Locale.getDefault());
    }
    catch (Throwable e)
    {
      LOGGER.e(TAG, "Failed to obtain currency symbol by currency code = " + currencyCode, e);
    }

    return currencyCode;
  }

  static String makeUrlSafe(@NonNull final String url)
  {
    return url.replaceAll("(token|password|key)=([^&]+)", "***");
  }

  @StringRes
  public static int getStringIdByKey(@NonNull Context context, @NonNull String key)
  {
    try
    {
      Resources res = context.getResources();
      @StringRes
      int nameId = res.getIdentifier(key, "string", context.getPackageName());
      if (nameId == INVALID_ID || nameId == View.NO_ID)
        throw new Resources.NotFoundException("String id '" + key + "' is not found");
      return nameId;
    }
    catch (RuntimeException e)
    {
      LOGGER.e(TAG, "Failed to get string with id '" + key + "'", e);
      if (isDebugOrBeta())
      {
        Toast.makeText(context, "Add string id for '" + key + "'!", Toast.LENGTH_LONG).show();
      }
    }
    return INVALID_ID;
  }

  public static boolean isDebugOrBeta()
  {
    return BuildConfig.BUILD_TYPE.equals("debug") || BuildConfig.BUILD_TYPE.equals("beta");
  }

  /**
   * Returns a string value for the specified key. If the value is not found then its key will be
   * returned.
   *
   * @return string value or its key if there is no string for the specified key.
   */
  @NonNull
  public static String getStringValueByKey(@NonNull Context context, @NonNull String key)
  {
    @StringRes
    int id = getStringIdByKey(context, key);
    try
    {
      return context.getString(id);
    }
    catch (Resources.NotFoundException e)
    {
      LOGGER.e(TAG, "Failed to get value for string '" + key + "'", e);
    }
    return key;
  }

  @NonNull
  public static String getDeviceName()
  {
    return Build.MANUFACTURER;
  }

  @NonNull
  public static String getDeviceModel()
  {
    return Build.MODEL;
  }

  @NonNull
  public static <T> T[] concatArrays(@Nullable T[] a, T... b)
  {
    if (a == null || a.length == 0)
      return b;
    if (b == null || b.length == 0)
      return a;

    T[] c = Arrays.copyOf(a, a.length + b.length);
    System.arraycopy(b, 0, c, a.length, b.length);
    return c;
  }

  public static void detachFragmentIfCoreNotInitialized(@NonNull Context context,
                                                        @NonNull Fragment fragment)
  {
    if (MwmApplication.from(context).arePlatformAndCoreInitialized())
      return;

    FragmentManager manager = fragment.getFragmentManager();
    if (manager == null)
      return;

    manager.beginTransaction().detach(fragment).commit();
  }

  public static String capitalize(@Nullable String src)
  {
    if (TextUtils.isEmpty(src))
      return src;

    if (src.length() == 1)
      return Character.toString(Character.toUpperCase(src.charAt(0)));

    return Character.toUpperCase(src.charAt(0)) + src.substring(1);
  }

  public static String unCapitalize(@Nullable String src)
  {
    if (TextUtils.isEmpty(src))
      return src;

    if (src.length() == 1)
      return Character.toString(Character.toLowerCase(src.charAt(0)));

    return Character.toLowerCase(src.charAt(0)) + src.substring(1);
  }

  public enum PartnerAppOpenMode
  {
    None, Direct, Indirect;
  }

  public interface Proc<T>
  {
    void invoke(@NonNull T param);
  }

  @NonNull
  private static String getLocalizedFeatureByKey(@NonNull Context context, @NonNull String key)
  {
    @StringRes
    int id = getStringIdByKey(context, key);

    try
    {
      return context.getString(id);
    }
    catch (Resources.NotFoundException e)
    {
      LOGGER.e(TAG, "Failed to get localized string for key '" + key + "'", e);
    }

    return key;
  }

  @NonNull
  public static String getLocalizedFeatureType(@NonNull Context context, @Nullable String type)
  {
    if (TextUtils.isEmpty(type))
      return "";

    String key = "type." + type.replace('-', '.')
                               .replace(':', '_');
    return getLocalizedFeatureByKey(context, key);
  }

  @NonNull
  public static String getLocalizedBrand(@NonNull Context context, @Nullable String brand)
  {
    if (TextUtils.isEmpty(brand))
      return "";

    String key = "brand." + brand;
    return getLocalizedFeatureByKey(context, key);
  }

  @NonNull
  public static Calendar getCalendarInstance()
  {
    return Calendar.getInstance(TimeZone.getTimeZone("UTC"));
  }

  @NonNull
  public static String calculateFinishTime(int seconds)
  {
    Calendar calendar = getCalendarInstance();
    calendar.add(Calendar.SECOND, seconds);
    DateFormat dateFormat = new SimpleDateFormat("HH:mm", Locale.getDefault());
    return dateFormat.format(calendar.getTime());
  }

  @NonNull
  public static String fixCaseInString(@NonNull String string)
  {
    char firstChar = string.charAt(0);
    return firstChar + string.substring(1).toLowerCase();
  }

  private static class SupportInfoWithLogsCallback implements LoggerFactory.OnZipCompletedListener
  {
    @NonNull
    private final WeakReference<Activity> mActivityRef;
    @NonNull
    private final String mSubject;
    @NonNull
    private final String mEmail;

    private SupportInfoWithLogsCallback(@NonNull Activity activity, @NonNull String subject,
                                        @NonNull String email)
    {
      mActivityRef = new WeakReference<>(activity);
      mSubject = subject;
      mEmail = email;
    }

    @Override
    public void onCompleted(final boolean success)
    {
      UiThread.run(() -> {
        Activity activity = mActivityRef.get();
        if (activity == null)
          return;

        final Intent intent = new Intent(Intent.ACTION_SEND);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.putExtra(Intent.EXTRA_EMAIL, new String[] { mEmail });
        intent.putExtra(Intent.EXTRA_SUBJECT, "[" + BuildConfig.VERSION_NAME + "] " + mSubject);
        if (success)
        {
          String logsZipFile = StorageUtils.getLogsZipPath(activity.getApplication());
          if (!TextUtils.isEmpty(logsZipFile))
          {
            Uri uri = StorageUtils.getUriForFilePath(activity, logsZipFile);
            intent.putExtra(Intent.EXTRA_STREAM, uri);
          }
        }
        // Do this so some email clients don't complain about empty body.
        intent.putExtra(Intent.EXTRA_TEXT, "");
        intent.setType("message/rfc822");
        try
        {
          activity.startActivity(intent);
        }
        catch (ActivityNotFoundException e)
        {
          CrashlyticsUtils.INSTANCE.logException(e);
        }
      });
    }
  }
}
