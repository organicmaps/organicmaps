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
import android.os.Build;
import android.support.annotation.DimenRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.Fragment;
import android.support.v4.app.NavUtils;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.AbsoluteSizeSpan;
import android.view.View;
import android.view.Window;
import android.widget.Toast;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.taxi.TaxiLinks;
import com.mapswithme.maps.taxi.TaxiManager;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.AlohaHelper;

import java.io.Closeable;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.text.NumberFormat;
import java.util.Currency;
import java.util.Locale;
import java.util.Map;

public class Utils
{
  @StringRes
  public static final int INVALID_ID = 0;
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = "Utils";

  public interface Proc<T>
  {
    void invoke(@NonNull T param);
  }

  private Utils() {}

  public static void closeStream(Closeable stream)
  {
    if (stream != null)
    {
      try
      {
        stream.close();
      } catch (final IOException e)
      {
        LOGGER.e(TAG, "Can't close stream", e);
      }
    }
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

  public static void toastShortcut(Context context, String message)
  {
    Toast.makeText(context, message, Toast.LENGTH_LONG).show();
  }

  public static void toastShortcut(Context context, int messageResId)
  {
    final String message = context.getString(messageResId);
    toastShortcut(context, message);
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

  public static boolean isPackageInstalled(String packageUri)
  {
    PackageManager pm = MwmApplication.get().getPackageManager();
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

  public static String getDeviceModel()
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

  public static void openUrl(@NonNull Context activity, @NonNull String url)
  {
    final Intent intent = new Intent(Intent.ACTION_VIEW);
    if (!url.startsWith("http://") && !url.startsWith("https://"))
      url = "http://" + url;
    intent.setData(Uri.parse(url));
    activity.startActivity(intent);
  }

  public static void sendSupportMail(@NonNull Activity activity, @NonNull String subject)
  {
    LoggerFactory.INSTANCE.zipLogs(new OnZipCompletedCallback(activity, subject));
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

  public static void checkConnection(final Context context, final @StringRes int message, final Proc<Boolean> onCheckPassedCallback)
  {
    if (ConnectionState.isConnected())
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

  public static String getInstallationId()
  {
    final Context context = MwmApplication.get();
    final SharedPreferences sharedPrefs = context.getSharedPreferences(
      org.alohalytics.Statistics.PREF_FILE, Context.MODE_PRIVATE);
    // "UNIQUE_ID" is the value of org.alohalytics.Statistics.PREF_UNIQUE_ID, but it private.
    String installationId = sharedPrefs.getString("UNIQUE_ID", null);

    if (TextUtils.isEmpty(installationId))
      return "";

    return installationId;
  }

  private static boolean isAppInstalled(@NonNull Activity context, @NonNull String packageName)
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

  public static boolean isTaxiAppInstalled(@NonNull Activity context, @TaxiManager.TaxiType int type)
  {
    switch (type)
    {
      case TaxiManager.PROVIDER_UBER:
        return Utils.isAppInstalled(context, "com.ubercab");
      case TaxiManager.PROVIDER_YANDEX:
        return Utils.isAppInstalled(context, "ru.yandex.taxi");
      default:
        throw new AssertionError("Unsupported taxi type: " + type);
    }
  }

  public static void launchTaxiApp(@NonNull Activity context, @NonNull TaxiLinks links,
                                   @TaxiManager.TaxiType int type)
  {
    switch (type)
    {
      case TaxiManager.PROVIDER_UBER:
        launchUber(context, links, isTaxiAppInstalled(context, type));
        break;
      case TaxiManager.PROVIDER_YANDEX:
        launchYandexTaxi(context, links);
        break;
      default:
        throw new AssertionError("Unsupported taxi type: " + type);
    }
  }

  private static void launchUber(@NonNull Activity context, @NonNull TaxiLinks links, boolean isAppInstalled)
  {
    final Intent intent = new Intent(Intent.ACTION_VIEW);
    if (isAppInstalled)
    {
      intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      intent.setData(Uri.parse(links.getDeepLink()));
    } else
    {
      // No Taxi app! Open mobile website.
      intent.setData(Uri.parse(links.getUniversalLink()));
    }
    context.startActivity(intent);
  }

  private static void launchYandexTaxi(@NonNull Activity context, @NonNull TaxiLinks links)
  {
    Intent intent = new Intent(Intent.ACTION_VIEW);
    intent.setData(Uri.parse(links.getDeepLink()));
    context.startActivity(intent);
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
    String text;
    try
    {
      float value = Float.valueOf(price);
      Locale locale = Locale.getDefault();
      Currency currency = Utils.getCurrencyForLocale(locale);
      // If the currency cannot be obtained for the default locale we will use Locale.US.
      if (currency == null)
        locale = Locale.US;
      NumberFormat formatter = NumberFormat.getCurrencyInstance(locale);
      if (!TextUtils.isEmpty(currencyCode))
        formatter.setCurrency(Currency.getInstance(currencyCode));
      return formatter.format(value);
    }
    catch (Throwable e)
    {
      LOGGER.e(TAG, "Failed to format string for price = " + price
                    + " and currencyCode = " + currencyCode, e);
      text = (price + " " + currencyCode);
    }
    return text;
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
      if (BuildConfig.BUILD_TYPE.equals("debug") || BuildConfig.BUILD_TYPE.equals("beta"))
      {
        Toast.makeText(context, "Add string id for '" + key + "'!",
                       Toast.LENGTH_LONG).show();
      }
    }
    return INVALID_ID;
  }

  private  static class OnZipCompletedCallback implements LoggerFactory.OnZipCompletedListener
  {
    @NonNull
    private final WeakReference<Activity> mActivityRef;
    @NonNull
    private final String mSubject;

    private OnZipCompletedCallback(@NonNull Activity activity, @NonNull String subject)
    {
      mActivityRef = new WeakReference<>(activity);
      mSubject = subject;
    }

    @Override
    public void onCompleted(final boolean success)
    {
      UiThread.run(new Runnable()
      {
        @Override
        public void run()
        {
          Activity activity = mActivityRef.get();
          if (activity == null)
            return;

          final Intent intent = new Intent(Intent.ACTION_SEND);
          intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
          intent.putExtra(Intent.EXTRA_EMAIL, new String[] { Constants.Email.SUPPORT });
          intent.putExtra(Intent.EXTRA_SUBJECT, "[" + BuildConfig.VERSION_NAME + "] " + mSubject);
          if (success)
          {
            String logsZipFile = StorageUtils.getLogsZipPath();
            if (!TextUtils.isEmpty(logsZipFile))
              intent.putExtra(Intent.EXTRA_STREAM, Uri.parse("file://" + logsZipFile));
          }
          intent.putExtra(Intent.EXTRA_TEXT, ""); // do this so some email clients don't complain about empty body.
          intent.setType("message/rfc822");
          try
          {
            activity.startActivity(intent);
          }
          catch (ActivityNotFoundException e)
          {
            AlohaHelper.logException(e);
          }
        }
      });
    }
  }

  public static void detachFragmentIfCoreNotInitialized(@NonNull Context context,
                                                        @NonNull Fragment fragment)
  {
    if (context instanceof AppCompatActivity && !MwmApplication.get().arePlatformAndCoreInitialized())
    {
      ((AppCompatActivity)context).getSupportFragmentManager()
                                  .beginTransaction()
                                  .detach(fragment)
                                  .commit();
    }
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
}
