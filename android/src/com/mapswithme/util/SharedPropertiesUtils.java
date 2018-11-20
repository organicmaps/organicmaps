package com.mapswithme.util;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarksPageFactory;

import static com.mapswithme.util.Config.KEY_PREF_STATISTICS;

public final class SharedPropertiesUtils
{
  private static final String PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING = "ShowEmulateBadStorageSetting";
  private static final String PREFS_BACKUP_WIDGET_EXPANDED = "BackupWidgetExpanded";
  private static final String PREFS_WHATS_NEW_TITLE_CONCATENATION = "WhatsNewTitleConcatenation";
  private static final String PREFS_CATALOG_CATEGORIES_HEADER_CLOSED = "CatalogCategoriesHeaderClosed";
  private static final String PREFS_BOOKMARK_CATEGORIES_LAST_VISIBLE_PAGE = "BookmarkCategoriesLastVisiblePage";
  private static final SharedPreferences PREFS
      = PreferenceManager.getDefaultSharedPreferences(MwmApplication.get());

  //Utils class
  private SharedPropertiesUtils()
  {
    throw new IllegalStateException("Try instantiate utility class SharedPropertiesUtils");
  }

  public static boolean isStatisticsEnabled()
  {
    return MwmApplication.prefs().getBoolean(KEY_PREF_STATISTICS, true);
  }

  public static void setStatisticsEnabled(boolean enabled)
  {
    MwmApplication.prefs().edit().putBoolean(KEY_PREF_STATISTICS, enabled).apply();
  }

  public static void setShouldShowEmulateBadStorageSetting(boolean show)
  {
    PREFS.edit().putBoolean(PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING, show).apply();
  }

  public static boolean shouldShowEmulateBadStorageSetting()
  {
    return PREFS.getBoolean(PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING, false);
  }

  public static boolean shouldEmulateBadExternalStorage()
  {
    String key = MwmApplication.get().getString(R.string.pref_emulate_bad_external_storage);
    return PREFS.getBoolean(key, false);
  }

  public static void setBackupWidgetExpanded(boolean expanded)
  {
    PREFS.edit().putBoolean(PREFS_BACKUP_WIDGET_EXPANDED, expanded).apply();
  }

  public static boolean getBackupWidgetExpanded()
  {
    return PREFS.getBoolean(PREFS_BACKUP_WIDGET_EXPANDED, true);
  }

  @Nullable
  public static String getWhatsNewTitleConcatenation()
  {
    return PREFS.getString(PREFS_WHATS_NEW_TITLE_CONCATENATION, null);
  }

  public static void setWhatsNewTitleConcatenation(@NonNull String concatenation)
  {
    PREFS.edit().putString(PREFS_WHATS_NEW_TITLE_CONCATENATION, concatenation).apply();
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
}
