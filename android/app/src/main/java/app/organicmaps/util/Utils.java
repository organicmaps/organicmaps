package app.organicmaps.util;

import static app.organicmaps.sdk.util.Utils.dimen;
import static app.organicmaps.sdk.util.Utils.isIntentSupported;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.Html;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.AbsoluteSizeSpan;
import android.util.AndroidRuntimeException;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.DimenRes;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.browser.customtabs.CustomTabsClient;
import androidx.core.app.NavUtils;
import androidx.core.os.BundleCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import app.organicmaps.BuildConfig;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.Constants;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.sdk.util.log.LogsManager;
import com.google.android.material.snackbar.Snackbar;
import java.lang.ref.WeakReference;
import java.time.LocalTime;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.TimeUnit;

@Keep
public class Utils
{
  private static final String TAG = Utils.class.getSimpleName();

  @StringRes
  public static final int INVALID_ID = 0;
  public static final String UTF_8 = "utf-8";
  public static final String TEXT_HTML = "text/html; charset=utf-8";
  public static final String ZIP_MIME_TYPE = "application/x-zip";
  public static final String EMAIL_MIME_TYPE = "message/rfc822";

  private Utils() {}

  /**
   * Enable to keep screen on.
   * Disable to let system turn it off automatically.
   */
  public static void keepScreenOn(boolean enable, Window w)
  {
    Logger.i(TAG, "KeepScreenOn = " + enable + " window = " + w);
    if (enable)
      w.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    else
      w.clearFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
  }

  private static void showOnLockScreenOld(boolean enable, Activity activity)
  {
    if (enable)
      activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
    else
      activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
  }

  public static void showOnLockScreen(boolean enable, Activity activity)
  {
    Logger.i(TAG, "showOnLockScreen = " + enable + " window = " + activity.getWindow());
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O_MR1)
      showOnLockScreenOld(enable, activity);
    else
      activity.setShowWhenLocked(enable);
  }

  public static void showSnackbar(@NonNull View view, @NonNull String message)
  {
    Snackbar snackbar = Snackbar.make(view, message, Snackbar.LENGTH_LONG);
    setSnackbarMaxLines(snackbar, 3);
    snackbar.show();
  }

  public static void showSnackbarAbove(@NonNull View view, @NonNull View viewAbove, @NonNull String message)
  {
    Snackbar snackbar = Snackbar.make(view, message, Snackbar.LENGTH_LONG);
    setSnackbarMaxLines(snackbar, 3);
    snackbar.setAnchorView(viewAbove);
    snackbar.show();
  }

  private static void setSnackbarMaxLines(@NonNull final Snackbar snackbar, final int maxLinesCount)
  {
    TextView snackTextView = snackbar.getView().findViewById(com.google.android.material.R.id.snackbar_text);
    snackTextView.setMaxLines(maxLinesCount);
  }

  public static void showSnackbar(@NonNull Context context, @NonNull View view, int messageResId)
  {
    showSnackbar(context, view, null, messageResId);
  }

  public static void showSnackbar(@NonNull Context context, @NonNull View view, @Nullable View viewAbove,
                                  int messageResId)
  {
    final String message = context.getString(messageResId);
    if (viewAbove == null)
      showSnackbar(view, message);
    else
      showSnackbarAbove(view, viewAbove, message);
  }

  public static @Nullable Intent makeSystemLocationSettingIntent(@NonNull Context context)
  {
    Intent intent = new Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS);
    if (isIntentSupported(context, intent))
      return intent;
    intent = new Intent(android.provider.Settings.ACTION_SECURITY_SETTINGS);
    if (isIntentSupported(context, intent))
      return intent;
    intent = new Intent(android.provider.Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
    Uri uri = Uri.fromParts("package", context.getPackageName(), null);
    intent.setData(uri);
    if (isIntentSupported(context, intent))
      return intent;
    return null;
  }

  public static void checkNotNull(Object object)
  {
    if (null == object)
      throw new NullPointerException("Argument here must not be NULL");
  }

  public static void copyTextToClipboard(Context context, String text)
  {
    final android.content.ClipboardManager clipboard =
        (android.content.ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
    final ClipData clip = ClipData.newPlainText("Organic Maps: " + text, text);
    clipboard.setPrimaryClip(clip);
  }
  public static Uri buildMailUri(String to, String subject, String body)
  {
    String uriString = Constants.Url.MAILTO_SCHEME + Uri.encode(to) + Constants.Url.MAIL_SUBJECT + Uri.encode(subject)
                     + Constants.Url.MAIL_BODY + Uri.encode(body);

    return Uri.parse(uriString);
  }

  public static void openAppInMarket(Activity activity, String url)
  {
    final Intent marketIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
    marketIntent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
    marketIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_DOCUMENT);

    try
    {
      activity.startActivity(marketIntent);
    }
    catch (ActivityNotFoundException e)
    {
      Logger.e(TAG, "Failed to start activity", e);
    }
  }

  /*
   * Check if WebBrowser intent could be opened.
   */
  public static boolean isBrowserAvailable(Context context)
  {
    final Intent intent = new Intent(Intent.ACTION_VIEW);
    intent.setData(Uri.parse("https://osm.org"));
    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    intent.addCategory(Intent.CATEGORY_BROWSABLE);

    // Check that an app exists to open URL
    return intent.resolveActivity(context.getPackageManager()) != null;
  }

  public static void showFacebookPage(Activity activity)
  {
    try
    {
      // Exception is thrown if we don't have installed Facebook application.
      getPackageInfo(activity.getPackageManager(), Constants.Package.FB_PACKAGE, 0);
      activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.FB_OM_COMMUNITY_NATIVE)));
    }
    catch (final Exception e)
    {
      activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.FB_OM_COMMUNITY_HTTP)));
    }
  }

  public static void openUrl(@NonNull Context context, @Nullable String url)
  {
    if (TextUtils.isEmpty(url))
      return;

    Uri uri =
        isHttpOrHttpsScheme(url) ? Uri.parse(url) : new Uri.Builder().scheme("http").appendEncodedPath(url).build();

    Utils.openUri(context, uri, R.string.browser_not_available);
  }

  /**
   * Attempts to open a URI in another app via the system app chooser.
   * @param context the app context
   * @param uri the URI to open.
   * @param failMessage string id: message to show in a toast when the system can't find an app to open with.
   */
  public static void openUri(@NonNull Context context, @NonNull Uri uri, @Nullable Integer failMessage)
  {
    final Intent intent = new Intent(Intent.ACTION_VIEW);
    intent.setData(uri);
    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    intent.addCategory(Intent.CATEGORY_BROWSABLE);

    try
    {
      context.startActivity(intent);
    }
    catch (ActivityNotFoundException e)
    {
      if (failMessage != null)
        Toast.makeText(context, context.getString(failMessage), Toast.LENGTH_LONG).show();
      Logger.e(TAG, "ActivityNotFoundException", e);
    }
    catch (AndroidRuntimeException e)
    {
      Logger.e(TAG, "AndroidRuntimeException", e);
      intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      context.startActivity(intent);
    }
  }

  private static boolean isHttpOrHttpsScheme(@NonNull String url)
  {
    return url.startsWith("http://") || url.startsWith("https://");
  }

  // subject is optional (could be an empty string).

  /**
   * @param subject could be an empty string
   */
  public static void sendBugReport(@NonNull ActivityResultLauncher<SharingUtils.SharingIntent> launcher,
                                   @NonNull Activity activity, @NonNull String subject, @NonNull String body)
  {
    subject = "Organic Maps Bugreport" + (TextUtils.isEmpty(subject) ? "" : ": " + subject);
    LogsManager.INSTANCE.zipLogs(
        new SupportInfoWithLogsCallback(launcher, activity, subject, body, BuildConfig.SUPPORT_MAIL));
  }

  // TODO: Don't send logs with general feedback, send system information only (version, device name, connectivity,
  // etc.)
  public static void sendFeedback(@NonNull ActivityResultLauncher<SharingUtils.SharingIntent> launcher,
                                  @NonNull Activity activity)
  {
    LogsManager.INSTANCE.zipLogs(
        new SupportInfoWithLogsCallback(launcher, activity, "Organic Maps Feedback", "", BuildConfig.SUPPORT_MAIL));
  }

  public static void navigateToParent(@NonNull Activity activity)
  {
    if (activity instanceof MwmActivity)
      ((MwmActivity) activity).customOnNavigateUp();
    else
      NavUtils.navigateUpFromSameTask(activity);
  }

  public static SpannableStringBuilder formatTime(Context context, @DimenRes int size, @DimenRes int units,
                                                  String dimension, String unitText)
  {
    final SpannableStringBuilder res = new SpannableStringBuilder(dimension).append("\u00A0").append(unitText);
    res.setSpan(new AbsoluteSizeSpan(dimen(context, size), false), 0, dimension.length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    res.setSpan(new AbsoluteSizeSpan(dimen(context, units), false), dimension.length(), res.length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    return res;
  }

  @NonNull
  public static Spannable formatDistance(Context context, @NonNull Distance distance)
  {
    final SpannableStringBuilder res = new SpannableStringBuilder(distance.toString(context));
    res.setSpan(new AbsoluteSizeSpan(dimen(context, R.dimen.text_size_nav_number), false), 0,
                distance.mDistanceStr.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    res.setSpan(new AbsoluteSizeSpan(dimen(context, R.dimen.text_size_nav_dimension), false),
                distance.mDistanceStr.length(), res.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    return res;
  }

  public static void sendTo(@NonNull Context context, @NonNull String email)
  {
    sendTo(context, email, "", "");
  }

  public static void sendTo(@NonNull Context context, @NonNull String email, @NonNull String subject)
  {
    sendTo(context, email, subject, "");
  }

  public static void sendTo(@NonNull Context context, @NonNull String email, @NonNull String subject,
                            @NonNull String body)
  {
    Intent intent = new Intent(Intent.ACTION_SENDTO);
    intent.setData(Utils.buildMailUri(email, subject, body));
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
      Logger.e(TAG, "Failed to call phone", e);
    }
  }

  public static void detachFragmentIfCoreNotInitialized(@NonNull Context context, @NonNull Fragment fragment)
  {
    if (MwmApplication.from(context).getOrganicMaps().arePlatformAndCoreInitialized())
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
  public static String getLocalizedLevel(@NonNull Context context, @Nullable String level)
  {
    if (TextUtils.isEmpty(level))
      return "";
    return context.getString(R.string.level_value_generic, level);
  }

  private static class SupportInfoWithLogsCallback implements LogsManager.OnZipCompletedListener
  {
    @NonNull
    ActivityResultLauncher<SharingUtils.SharingIntent> mLauncher;
    @NonNull
    private final WeakReference<Activity> mActivityRef;
    @NonNull
    private final String mSubject;
    @NonNull
    private final String mBody;
    @NonNull
    private final String mEmail;

    private SupportInfoWithLogsCallback(@NonNull ActivityResultLauncher<SharingUtils.SharingIntent> launcher,
                                        @NonNull Activity activity, @NonNull String subject, @NonNull String body,
                                        @NonNull String email)
    {
      mActivityRef = new WeakReference<>(activity);
      mSubject = subject;
      mBody = body;
      mEmail = email;
      mLauncher = launcher;
    }

    @Override
    public void onCompleted(final boolean success, @Nullable final String zipPath)
    {
      // TODO: delete zip file after its sent.
      UiThread.run(() -> {
        final Activity activity = mActivityRef.get();
        if (activity == null)
          return;

        SharingUtils.ShareInfo info = new SharingUtils.ShareInfo();

        info.mMail = mEmail;
        info.mSubject = "[" + BuildConfig.VERSION_NAME + "] " + mSubject;
        info.mText = mBody;

        if (success)
        {
          info.mFileName = zipPath;
          info.mMimeType = ZIP_MIME_TYPE;
        }
        else
        {
          info.mMimeType = EMAIL_MIME_TYPE;
        }

        SharingUtils.shareFile(activity.getApplicationContext(), mLauncher, info);
      });
    }
  }

  public static <T> T getParcelable(@NonNull Bundle in, @Nullable String key, @NonNull Class<T> clazz)
  {
    in.setClassLoader(clazz.getClassLoader());
    return BundleCompat.getParcelable(in, key, clazz);
  }

  @SuppressWarnings("deprecation")
  private static Spanned fromHtmlOld(@NonNull String htmlDescription)
  {
    return Html.fromHtml(htmlDescription);
  }

  public static Spanned fromHtml(@NonNull String htmlDescription)
  {
    if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.N)
      return fromHtmlOld(htmlDescription);
    return Html.fromHtml(htmlDescription, Html.FROM_HTML_MODE_LEGACY);
  }

  @SuppressWarnings("deprecation")
  private static PackageInfo getPackageInfoOld(@NonNull PackageManager manager, @NonNull String packageName, int flags)
      throws PackageManager.NameNotFoundException
  {
    return manager.getPackageInfo(packageName, flags);
  }

  public static PackageInfo getPackageInfo(@NonNull PackageManager manager, @NonNull String packageName, int flags)
      throws PackageManager.NameNotFoundException
  {
    if (android.os.Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU)
      return getPackageInfoOld(manager, packageName, flags);
    return manager.getPackageInfo(packageName, PackageManager.PackageInfoFlags.of(flags));
  }

  @NonNull
  public static CharSequence formatRoutingTime(Context context, int seconds, @DimenRes int unitsSize)
  {
    return formatRoutingTime(context, seconds, unitsSize, R.dimen.text_size_routing_number);
  }

  @NonNull
  public static CharSequence formatRoutingTime(Context context, int seconds, @DimenRes int unitsSize,
                                               @DimenRes int textSize)
  {
    long minutes = TimeUnit.SECONDS.toMinutes(seconds) % 60;
    long hours = TimeUnit.SECONDS.toHours(seconds);
    String min = context.getString(R.string.minute);
    String hour = context.getString(R.string.hour);
    SpannableStringBuilder displayedH = Utils.formatTime(context, textSize, unitsSize, String.valueOf(hours), hour);
    SpannableStringBuilder displayedM = Utils.formatTime(context, textSize, unitsSize, String.valueOf(minutes), min);
    return hours == 0 ? displayedM : TextUtils.concat(displayedH + "\u00A0", displayedM);
  }

  @NonNull
  public static String formatArrivalTime(int seconds)
  {
    final LocalTime time = LocalTime.now().plusSeconds(seconds);
    return StringUtils.formatUsingUsLocale("%d:%02d", time.getHour(), time.getMinute());
  }

  @NonNull
  public static String getDonateUrl(@NonNull Context context)
  {
    final String url = Config.getDonateUrl();
    // Enable donations by default if not Google or Huawei. Replace organicmaps.app/donate/ with localized page.
    if ((url.isEmpty() && !BuildConfig.FLAVOR.equals("google") && !BuildConfig.FLAVOR.equals("huawei"))
        || url.endsWith("organicmaps.app/donate/"))
      return context.getString(R.string.translated_om_site_url) + "donate/";
    return url;
  }

  /**
   * Taken from <a href="https://developer.chrome.com/docs/android/custom-tabs/howto-custom-tab-check">Chrome docs</a>.
   * @return The package name of a browser that supports Custom Tabs, prioritizing the default browser.
   *   Returns null if Custom Tabs isn't supported by any installed browser.
   */
  @Nullable
  public static String getCustomTabsPackage(Context context)
  {
    String packageName = CustomTabsClient.getPackageName(context, Collections.emptyList());
    if (packageName != null) // The default browser supports Custom Tabs
      return packageName;

    // Get all apps that can handle VIEW intents and Custom Tab service connections.
    Intent activityIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://example.com"));
    PackageManager packageManager = context.getPackageManager();
    List<ResolveInfo> resolveInfos = packageManager.queryIntentActivities(
        activityIntent, Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                            ? PackageManager.MATCH_ALL
                            : PackageManager.GET_ACTIVITIES | PackageManager.GET_INTENT_FILTERS);

    // Extract package names from ResolveInfo objects
    List<String> packageNames = new ArrayList<>();
    for (ResolveInfo info : resolveInfos)
      packageNames.add(info.activityInfo.packageName);

    // Get a package that supports Custom Tabs
    packageName = CustomTabsClient.getPackageName(context, packageNames, true /* ignore default */);
    return packageName;
  }
}
