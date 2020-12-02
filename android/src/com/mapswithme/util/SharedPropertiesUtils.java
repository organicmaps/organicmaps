package com.mapswithme.util;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarksPageFactory;
import com.mapswithme.maps.maplayer.Mode;

import java.util.Locale;

import static com.mapswithme.util.Config.KEY_PREF_STATISTICS;

public final class SharedPropertiesUtils
{
  private static final String USER_AGREEMENT_TERM_OF_USE = "user_agreement_term_of_use";
  private static final String USER_AGREEMENT_PRIVACY_POLICY = "user_agreement_privacy_policy";
  private static final String PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING = "ShowEmulateBadStorageSetting";
  private static final String PREFS_BACKUP_WIDGET_EXPANDED = "BackupWidgetExpanded";
  private static final String PREFS_WHATS_NEW_TITLE_CONCATENATION = "WhatsNewTitleConcatenation";
  private static final String PREFS_CATALOG_CATEGORIES_HEADER_CLOSED = "CatalogCategoriesHeaderClosed";
  private static final String PREFS_BOOKMARK_CATEGORIES_LAST_VISIBLE_PAGE = "BookmarkCategoriesLastVisiblePage";
  private static final String PREFS_SHOULD_SHOW_LAYER_MARKER_FOR = "ShouldShowGuidesLayerMarkerFor";
  private static final String PREFS_SHOULD_SHOW_LAYER_TUTORIAL_TOAST = "ShouldShowLayerTutorialToast";
  private static final String PREFS_SHOULD_SHOW_HOW_TO_USE_GUIDES_LAYER_TOAST
      = "ShouldShowHowToUseGuidesLayerToast";

  //Utils class
  private SharedPropertiesUtils()
  {
    throw new IllegalStateException("Try instantiate utility class SharedPropertiesUtils");
  }

  public static boolean isStatisticsEnabled(@NonNull Context context)
  {
    return MwmApplication.prefs(context).getBoolean(KEY_PREF_STATISTICS, true);
  }

  public static void setStatisticsEnabled(@NonNull Context context, boolean enabled)
  {
    MwmApplication.prefs(context).edit().putBoolean(KEY_PREF_STATISTICS, enabled).apply();
  }

  public static void setShouldShowEmulateBadStorageSetting(@NonNull Context context, boolean show)
  {
    SharedPreferences prefs = PreferenceManager
        .getDefaultSharedPreferences(MwmApplication.from(context));
    prefs.edit().putBoolean(PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING, show).apply();
  }

  public static boolean shouldShowEmulateBadStorageSetting(@NonNull Context context)
  {
    SharedPreferences prefs = PreferenceManager
        .getDefaultSharedPreferences(MwmApplication.from(context));
    return prefs.getBoolean(PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING, false);
  }

  public static boolean shouldEmulateBadExternalStorage(@NonNull Context context)
  {
    SharedPreferences prefs = PreferenceManager
        .getDefaultSharedPreferences(MwmApplication.from(context));
    String key = MwmApplication.from(context).getString(R.string.pref_emulate_bad_external_storage);
    return prefs.getBoolean(key, false);
  }

  public static void setBackupWidgetExpanded(@NonNull Context context, boolean expanded)
  {
    SharedPreferences prefs = PreferenceManager
        .getDefaultSharedPreferences(MwmApplication.from(context));
    prefs.edit().putBoolean(PREFS_BACKUP_WIDGET_EXPANDED, expanded).apply();
  }

  public static boolean getBackupWidgetExpanded(@NonNull Context context)
  {
    SharedPreferences prefs = PreferenceManager
        .getDefaultSharedPreferences(MwmApplication.from(context));
    return prefs.getBoolean(PREFS_BACKUP_WIDGET_EXPANDED, true);
  }

  @Nullable
  public static String getWhatsNewTitleConcatenation(@NonNull Context context)
  {
    SharedPreferences prefs = PreferenceManager
        .getDefaultSharedPreferences(MwmApplication.from(context));
    return prefs.getString(PREFS_WHATS_NEW_TITLE_CONCATENATION, null);
  }

  public static void setWhatsNewTitleConcatenation(@NonNull Context context,
                                                   @NonNull String concatenation)
  {
    SharedPreferences prefs = PreferenceManager
        .getDefaultSharedPreferences(MwmApplication.from(context));
    prefs.edit().putString(PREFS_WHATS_NEW_TITLE_CONCATENATION, concatenation).apply();
  }

  public static boolean isCatalogCategoriesHeaderClosed(@NonNull Context context)
  {
    return MwmApplication.prefs(context)
                         .getBoolean(PREFS_CATALOG_CATEGORIES_HEADER_CLOSED, false);
  }

  public static void setCatalogCategoriesHeaderClosed(@NonNull Context context, boolean value)
  {
    MwmApplication.prefs(context)
                  .edit()
                  .putBoolean(PREFS_CATALOG_CATEGORIES_HEADER_CLOSED, value)
                  .apply();
  }

  public static int getLastVisibleBookmarkCategoriesPage(@NonNull Context context)
  {
    return MwmApplication.prefs(context)
                         .getInt(PREFS_BOOKMARK_CATEGORIES_LAST_VISIBLE_PAGE,
                                 BookmarksPageFactory.PRIVATE.ordinal());
  }

  public static void setLastVisibleBookmarkCategoriesPage(@NonNull Context context, int pageIndex)
  {
    MwmApplication.prefs(context)
                  .edit()
                  .putInt(PREFS_BOOKMARK_CATEGORIES_LAST_VISIBLE_PAGE, pageIndex)
                  .apply();
  }

  public static boolean isTermOfUseAgreementConfirmed(@NonNull Context context)
  {
    return getBoolean(context, USER_AGREEMENT_TERM_OF_USE);
  }

  public static boolean isPrivacyPolicyAgreementConfirmed(@NonNull Context context)
  {
    return getBoolean(context, USER_AGREEMENT_PRIVACY_POLICY);
  }

  public static void putPrivacyPolicyAgreement(@NonNull Context context, boolean isChecked)
  {
    putBoolean(context, USER_AGREEMENT_PRIVACY_POLICY, isChecked);
  }

  public static void putTermOfUseAgreement(@NonNull Context context, boolean isChecked)
  {
    putBoolean(context, USER_AGREEMENT_TERM_OF_USE, isChecked);
  }

  public static boolean shouldShowNewMarkerForLayerMode(@NonNull Context context,
                                                        @NonNull Mode mode)
  {
    switch (mode)
    {
      case SUBWAY:
      case TRAFFIC:
      case ISOLINES:
        return false;
      default:
        return getBoolean(context, PREFS_SHOULD_SHOW_LAYER_MARKER_FOR + mode.name()
                                                                            .toLowerCase(Locale.ENGLISH),
                          true);
    }
  }

  public static boolean shouldShowNewMarkerForLayerMode(@NonNull Context context, @NonNull String mode)
  {
    return shouldShowNewMarkerForLayerMode(context, Mode.valueOf(mode));
  }

  public static boolean shouldShowLayerTutorialToast(@NonNull Context context)
  {
    boolean result = getBoolean(context, PREFS_SHOULD_SHOW_LAYER_TUTORIAL_TOAST, true);
    putBoolean(context, PREFS_SHOULD_SHOW_LAYER_TUTORIAL_TOAST, false);
    return result;
  }

  public static boolean shouldShowHowToUseGuidesLayerToast(@NonNull Context context)
  {
    boolean result = getBoolean(context, PREFS_SHOULD_SHOW_HOW_TO_USE_GUIDES_LAYER_TOAST, true);
    putBoolean(context, PREFS_SHOULD_SHOW_HOW_TO_USE_GUIDES_LAYER_TOAST, false);
    return result;
  }

  public static void setLayerMarkerShownForLayerMode(@NonNull Context context, @NonNull Mode mode)
  {
    putBoolean(context, PREFS_SHOULD_SHOW_LAYER_MARKER_FOR + mode.name()
                                                                 .toLowerCase(Locale.ENGLISH), false);
  }

  public static void setLayerMarkerShownForLayerMode(@NonNull Context context, @NonNull String mode)
  {
    setLayerMarkerShownForLayerMode(context, Mode.valueOf(mode));
  }

  private static boolean getBoolean(@NonNull Context context,  @NonNull String key)
  {
    return getBoolean(context, key, false);
  }

  private static boolean getBoolean(@NonNull Context context,  @NonNull String key, boolean defValue)
  {
    return MwmApplication.prefs(context).getBoolean(key, defValue);
  }

  private static void putBoolean(@NonNull Context context,  @NonNull String key, boolean value)
  {
    MwmApplication.prefs(context)
                  .edit()
                  .putBoolean(key, value)
                  .apply();
  }
}
