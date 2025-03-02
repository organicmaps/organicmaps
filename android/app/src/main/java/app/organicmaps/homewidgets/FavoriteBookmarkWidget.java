package app.organicmaps.homewidgets;

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.util.Log;
import android.widget.RemoteViews;

import app.organicmaps.MwmActivity;
import app.organicmaps.R;

public class FavoriteBookmarkWidget extends AppWidgetProvider {

    private static final String PREFS_NAME = "app.organicmaps.homewidgets.FavoriteBookmarkWidget";
    private static final String PREF_PREFIX_KEY = "widget_";
    private static final String TAG = "FavoriteBookmarkWidget";

    @Override
    public void onUpdate(Context context, AppWidgetManager appWidgetManager, int[] appWidgetIds) {
        for (int widgetId : appWidgetIds) {
            updateWidget(context, appWidgetManager, widgetId);
        }
    }

    @Override
    public void onDeleted(Context context, int[] appWidgetIds) {
        for (int widgetId : appWidgetIds) {
            deleteBookmarkPref(context, widgetId);
        }
    }

    static void updateWidget(Context context, AppWidgetManager appWidgetManager, int widgetId) {
        SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, 0);
        String name = prefs.getString(PREF_PREFIX_KEY + widgetId + "_name", context.getString(R.string.favorite_place));
        int categoryIndex = prefs.getInt(PREF_PREFIX_KEY + widgetId + "_category_index", -1);
        int bookmarkIndex = prefs.getInt(PREF_PREFIX_KEY + widgetId + "_bookmark_index", -1);
        
        RemoteViews views = new RemoteViews(context.getPackageName(), R.layout.widget_favorite_bookmark);
        
        views.setTextViewText(R.id.bookmark_name, name);
        
        Log.d(TAG, "Updating widget " + widgetId + " with bookmark: " + name + 
              " (categoryIndex=" + categoryIndex + ", bookmarkIndex=" + bookmarkIndex + ")");
        
        Intent intent = new Intent(context, MwmActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        intent.setAction("app.organicmaps.action.SHOW_BOOKMARK");
        
        intent.putExtra("FROM_WIDGET", true);
        if (categoryIndex >= 0 && bookmarkIndex >= 0) {
            intent.putExtra("CATEGORY_INDEX", categoryIndex);
            intent.putExtra("BOOKMARK_INDEX", bookmarkIndex);
            
            PendingIntent pendingIntent = PendingIntent.getActivity(
                    context, 
                    widgetId, 
                    intent, 
                    PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
            
            views.setOnClickPendingIntent(R.id.widget_container, pendingIntent);
        }
        
        appWidgetManager.updateAppWidget(widgetId, views);
    }

    static void saveBookmarkPref(Context context, int appWidgetId, int categoryIndex, int bookmarkIndex, String name) {
        Log.d(TAG, "Saving bookmark pref: categoryIndex=" + categoryIndex + ", bookmarkIndex=" + bookmarkIndex + ", name=" + name);

        SharedPreferences.Editor prefs = context.getSharedPreferences(PREFS_NAME, 0).edit();
        prefs.putInt(PREF_PREFIX_KEY + appWidgetId + "_category_index", categoryIndex);
        prefs.putInt(PREF_PREFIX_KEY + appWidgetId + "_bookmark_index", bookmarkIndex);
        prefs.putString(PREF_PREFIX_KEY + appWidgetId + "_name", name);
        prefs.apply();
    }

    private static void deleteBookmarkPref(Context context, int appWidgetId) {
        SharedPreferences.Editor prefs = context.getSharedPreferences(PREFS_NAME, 0).edit();
        prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_category_index");
        prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_bookmark_index");
        prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_name");
        prefs.apply();
    }
}