package app.organicmaps.bookmarks;

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.widget.RemoteViews;

import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.bookmarks.data.BookmarkCategory;
import app.organicmaps.bookmarks.data.BookmarkInfo;
import app.organicmaps.bookmarks.data.BookmarkManager;

public class FavoriteBookmarkWidget extends AppWidgetProvider
{
  private static final String TAG = "FavoriteBookmarkWidget";
  private static final String PREFS_NAME = "FavoriteBookmarkWidget";
  private static final String PREF_PREFIX_KEY = "favorite_bookmark_";

  private static final String SUFFIX_LAT = "_latitude";
  private static final String SUFFIX_LON = "_longitude";
  private static final String SUFFIX_NAME = "_name";
  private static final String SUFFIX_CATEGORY_NAME = "_category_name";
  private static final String SUFFIX_COLOR = "_color";

  @Override
  public void onUpdate(Context context, AppWidgetManager appWidgetManager, int[] appWidgetIds)
  {
    for (int appWidgetId : appWidgetIds)
    {
      updateWidget(context, appWidgetManager, appWidgetId);
    }
  }

  public static void updateWidget(Context context, AppWidgetManager appWidgetManager, int appWidgetId)
  {
    SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, 0);
    RemoteViews views = new RemoteViews(context.getPackageName(), R.layout.widget_favorite_bookmark);

    double lat = prefs.getFloat(PREF_PREFIX_KEY + appWidgetId + SUFFIX_LAT, 0);
    double lon = prefs.getFloat(PREF_PREFIX_KEY + appWidgetId + SUFFIX_LON, 0);
    String categoryName = prefs.getString(PREF_PREFIX_KEY + appWidgetId + "_category_name", "");
    String bookmarkName = prefs.getString(PREF_PREFIX_KEY + appWidgetId + SUFFIX_NAME, "");
    int iconColor = prefs.getInt(PREF_PREFIX_KEY + appWidgetId + SUFFIX_COLOR, 0);

    boolean validCoordinates = (lat != 0 || lon != 0);
    BookmarkInfo bookmarkInfo = null;

    if (validCoordinates)
    {
      bookmarkInfo = BookmarkManager.INSTANCE.findBookmarkByCoordinates(lat, lon, bookmarkName, categoryName);
    }

    String displayName = bookmarkInfo != null ? bookmarkInfo.getName() :
        (bookmarkName != null && !bookmarkName.isEmpty() ? bookmarkName :
            context.getString(R.string.select_bookmark));

    views.setTextViewText(R.id.tv__bookmark_name, displayName);

    if (bookmarkInfo != null)
    {

      int iconResId = BookmarkManager.INSTANCE.getBookmarkIcon(bookmarkInfo.getBookmarkId());
      if (iconResId == 0)
      {
        iconResId = R.drawable.ic_bookmarks;
      }

      views.setImageViewResource(R.id.iv__bookmark_color, iconResId);

      Intent intent = new Intent(context, MwmActivity.class);
      intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK |
          Intent.FLAG_ACTIVITY_CLEAR_TOP |
          Intent.FLAG_ACTIVITY_SINGLE_TOP);
      intent.setAction("app.organicmaps.action.SHOW_BOOKMARK");

      intent.putExtra("FROM_WIDGET", true);
      intent.putExtra("BOOKMARK_ID", bookmarkInfo.getBookmarkId());
      intent.putExtra("BOOKMARK_NAME", bookmarkInfo.getName());
      intent.putExtra("BOOKMARK_LAT", bookmarkInfo.getLat());
      intent.putExtra("BOOKMARK_LON", bookmarkInfo.getLon());
      intent.putExtra("BOOKMARK_CATEGORY",
          BookmarkManager.INSTANCE.getCategoryById(bookmarkInfo.getCategoryId()).getName());


      PendingIntent pendingIntent = PendingIntent.getActivity(
          context,
          appWidgetId,
          intent,
          PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

      views.setOnClickPendingIntent(R.id.widget_container, pendingIntent);
    }
    else
    {
      views.setImageViewResource(R.id.iv__bookmark_color, R.drawable.ic_bookmarks);

      Intent intent = new Intent(context, FavoriteBookmarkWidgetConfigActivity.class);
      PendingIntent pendingIntent = PendingIntent.getActivity(
          context,
          0,
          intent,
          PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

      views.setOnClickPendingIntent(R.id.widget_container, pendingIntent);
    }

    appWidgetManager.updateAppWidget(appWidgetId, views);
  }

  public static void saveBookmarkPref(Context context, int appWidgetId,
                                      BookmarkInfo bookmarkInfo, BookmarkCategory category)
  {
    SharedPreferences.Editor prefs = context.getSharedPreferences(PREFS_NAME, 0).edit();

    prefs.putFloat(PREF_PREFIX_KEY + appWidgetId + SUFFIX_LAT, (float) bookmarkInfo.getLat());
    prefs.putFloat(PREF_PREFIX_KEY + appWidgetId + SUFFIX_LON, (float) bookmarkInfo.getLon());
    prefs.putString(PREF_PREFIX_KEY + appWidgetId + SUFFIX_NAME, bookmarkInfo.getName());
    prefs.putString(PREF_PREFIX_KEY + appWidgetId + SUFFIX_CATEGORY_NAME, category.getName());
    prefs.putLong(PREF_PREFIX_KEY + appWidgetId + "_bookmark_id", bookmarkInfo.getBookmarkId());
    prefs.putLong(PREF_PREFIX_KEY + appWidgetId + "_category_id", category.getId());

    prefs.putString(PREF_PREFIX_KEY + appWidgetId + "_name_hash",
        String.valueOf(bookmarkInfo.getName().hashCode()));
    prefs.putString(PREF_PREFIX_KEY + appWidgetId + "_lat_lon_hash",
        String.valueOf((bookmarkInfo.getLat() + "," + bookmarkInfo.getLon()).hashCode()));

    prefs.apply();
  }

  @Override
  public void onDeleted(Context context, int[] appWidgetIds)
  {
    SharedPreferences.Editor prefs = context.getSharedPreferences(PREFS_NAME, 0).edit();
    for (int appWidgetId : appWidgetIds)
    {
      prefs.remove(PREF_PREFIX_KEY + appWidgetId + SUFFIX_LAT);
      prefs.remove(PREF_PREFIX_KEY + appWidgetId + SUFFIX_LON);
      prefs.remove(PREF_PREFIX_KEY + appWidgetId + SUFFIX_NAME);
      prefs.remove(PREF_PREFIX_KEY + appWidgetId + SUFFIX_CATEGORY_NAME);
      prefs.remove(PREF_PREFIX_KEY + appWidgetId + SUFFIX_COLOR);
    }
    prefs.apply();
  }
}    
