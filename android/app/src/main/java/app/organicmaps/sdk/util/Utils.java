package app.organicmaps.sdk.util;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.os.Build;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.view.View;
import android.widget.Toast;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import app.organicmaps.BuildConfig;
import app.organicmaps.sdk.util.log.Logger;
import java.text.DecimalFormatSymbols;
import java.util.Currency;
import java.util.Locale;

@Keep
public class Utils
{
  private static final String TAG = Utils.class.getSimpleName();

  @StringRes
  private static final int INVALID_ID = 0;

  static String makeUrlSafe(@NonNull final String url)
  {
    return url.replaceAll("(token|password|key)=([^&]+)", "***");
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @Nullable
  public static String getCurrencyCode()
  {
    Locale[] locales = {Locale.getDefault(), Locale.US};
    for (Locale locale : locales)
    {
      Currency currency = getCurrencyForLocale(locale);
      if (currency != null)
        return currency.getCurrencyCode();
    }
    return null;
  }

  @Nullable
  private static Currency getCurrencyForLocale(@NonNull Locale locale)
  {
    try
    {
      return Currency.getInstance(locale);
    }
    catch (Throwable e)
    {
      Logger.e(TAG, "Failed to obtain a currency for locale: " + locale, e);
      return null;
    }
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @NonNull
  public static String getCountryCode()
  {
    return Locale.getDefault().getCountry();
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @NonNull
  public static String getLanguageCode()
  {
    return Locale.getDefault().getLanguage();
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @NonNull
  public static String getDecimalSeparator()
  {
    return String.valueOf(DecimalFormatSymbols.getInstance().getDecimalSeparator());
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @NonNull
  public static String getGroupingSeparator()
  {
    return String.valueOf(DecimalFormatSymbols.getInstance().getGroupingSeparator());
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @NonNull
  public static String getCurrencySymbol(@NonNull String currencyCode)
  {
    try
    {
      return Currency.getInstance(currencyCode).getSymbol(Locale.getDefault());
    }
    catch (Throwable e)
    {
      Logger.e(TAG, "Failed to obtain currency symbol by currency code = " + currencyCode, e);
    }

    return currencyCode;
  }

  /**
   * Returns a string value for the specified key. If the value is not found then its key will be
   * returned.
   *
   * @return string value or its key if there is no string for the specified key.
   */
  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @NonNull
  public static String getStringValueByKey(@NonNull Context context, @NonNull String key)
  {
    try
    {
      return context.getString(getStringIdByKey(context, key));
    }
    catch (Resources.NotFoundException e)
    {
      Logger.e(TAG, "Failed to get value for string '" + key + "'", e);
    }
    return key;
  }

  @StringRes
  @SuppressLint("DiscouragedApi")
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
      Logger.e(TAG, "Failed to get string with id '" + key + "'", e);
      if (BuildConfig.BUILD_TYPE.equals("debug") || BuildConfig.BUILD_TYPE.equals("beta"))
      {
        Toast.makeText(context, "Add string id for '" + key + "'!", Toast.LENGTH_LONG).show();
      }
    }
    return INVALID_ID;
  }

  /**
   * Returns a name for a new bookmark created off the current GPS location.
   * The name includes current time and date in locale-specific format.
   *
   * @return bookmark name with time and date.
   */
  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @NonNull
  public static String getMyPositionBookmarkName(@NonNull Context context)
  {
    return DateUtils.formatDateTime(
        context, System.currentTimeMillis(),
        DateUtils.FORMAT_SHOW_TIME | DateUtils.FORMAT_SHOW_DATE | DateUtils.FORMAT_SHOW_YEAR);
  }

  // Called from JNI.
  @NonNull
  @Keep
  @SuppressWarnings("unused")
  public static String getDeviceName()
  {
    return Build.MANUFACTURER;
  }

  // Called from JNI.
  @NonNull
  @Keep
  @SuppressWarnings("unused")
  public static String getDeviceModel()
  {
    return Build.MODEL;
  }

  // Called from JNI.
  @NonNull
  @Keep
  @SuppressWarnings("unused")
  public static String getVersion()
  {
    return BuildConfig.VERSION_NAME;
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public static int getIntVersion()
  {
    // Please sync with getVersion() in build.gradle
    // - % 100000000 removes prefix for special markets, e.g Huawei.
    // - / 100 removes the number of commits in the current day.
    return (BuildConfig.VERSION_CODE % 1_00_00_00_00) / 100;
  }

  @NonNull
  public static String getTagValueLocalized(@NonNull Context context, @Nullable String tagKey, @Nullable String value)
  {
    if (TextUtils.isEmpty(tagKey) || TextUtils.isEmpty(value))
      return "";

    return getLocalizedFeatureType(context, tagKey + "-" + value);
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @NonNull
  public static String getLocalizedFeatureType(@NonNull Context context, @Nullable String type)
  {
    if (TextUtils.isEmpty(type))
      return "";

    String key = "type." + type.replace('-', '.').replace(':', '_');
    return getLocalizedFeatureByKey(context, key);
  }

  @NonNull
  private static String getLocalizedFeatureByKey(@NonNull Context context, @NonNull String key)
  {
    return getStringValueByKey(context, key);
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @NonNull
  public static String getLocalizedBrand(@NonNull Context context, @Nullable String brand)
  {
    if (TextUtils.isEmpty(brand))
      return "";

    try
    {
      @StringRes
      int nameId = context.getResources().getIdentifier("brand." + brand, "string", context.getPackageName());
      if (nameId == INVALID_ID || nameId == View.NO_ID)
        return brand;
      return context.getString(nameId);
    }
    catch (Resources.NotFoundException e)
    {}
    return brand;
  }
}
