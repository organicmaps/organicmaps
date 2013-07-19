package com.mapswithme.test;

import java.util.HashMap;
import java.util.Map;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.text.TextUtils;

import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MWMActivity.MapTask;
import com.mapswithme.maps.SearchActivity;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

public class MonkeyEventsReceiver extends BroadcastReceiver
{


  static Logger l = SimpleLogger.get("MonkeyBusiness");

  @Override
  public void onReceive(Context context, Intent intent)
  {
    ExtractableMapTask mapTask = KEY_TO_TASK.get(intent.getStringExtra(EXTRA_TASK));
    if (mapTask != null)
    {
      mapTask = mapTask.extract(intent);
      l.d(mapTask.toString());
      intent = new Intent(context, MWMActivity.class);
      intent.putExtra(MWMActivity.EXTRA_TASK, mapTask);
      intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      context.startActivity(intent);
    }
    else
      l.d("No task found");
  }


  public interface ExtractableMapTask extends MapTask
  {
    public ExtractableMapTask extract(Intent intent);
  };

  private static String EXTRA_TASK = "task";
  private static String TYPE_SEARCH = "task_search";
  private static String TYPE_BOOKMARK = "task_bmk";

  private static final Map<String, ExtractableMapTask> KEY_TO_TASK = new HashMap<String, ExtractableMapTask>(4);

  static {
    KEY_TO_TASK.put(TYPE_BOOKMARK, new ShowBookmarksTask());
    KEY_TO_TASK.put(TYPE_SEARCH, new SearchTask());
  }


  @SuppressWarnings("serial")
  public static class SearchTask implements ExtractableMapTask
  {
    String query;
    int scope;

    @Override
    public boolean run(MWMActivity target)
    {
      l.d("Running me!", this, target);
      SearchActivity.startForSearch(target, query, scope);
      return true;
    }

    @Override
    public ExtractableMapTask extract(Intent data)
    {
      SearchTask task = new SearchTask();

      task.query = data.getStringExtra(SearchActivity.EXTRA_QUERY);
      l.d("q: ", task.query);

      final String scopeStr = data.getStringExtra(SearchActivity.EXTRA_SCOPE);
      l.d("s: ", scopeStr);
      if (scopeStr != null) task.scope = Integer.parseInt(scopeStr);

      return task;
    }

    @Override
    public String toString()
    {
      return "SearchTask [query=" + query + ", scope=" + scope + "]";
    }
  }

  @SuppressWarnings("serial")
  public static class ShowBookmarksTask implements ExtractableMapTask
  {
    String name;

    @Override
    public boolean run(final MWMActivity target)
    {
      final BookmarkManager bmkManager  = BookmarkManager.getBookmarkManager(target);
      final int categoriesCount = bmkManager.getCategoriesCount();

      // find category
      BookmarkCategory categoryToShow = null;
      for (int i = 0; i < categoriesCount; i++)
      {
        categoryToShow = bmkManager.getCategoryById(i);
        if (categoryToShow.getName().contains(name))
          break;
        else
          categoryToShow = null;
      }

      if (categoryToShow != null)
      {
        final int catToShowId = categoryToShow.getId();
        final Handler h = new Handler();
        l.d("Found category:", categoryToShow.getName());

        class ShowBmkRunnable implements Runnable
        {
          MWMActivity activity;
          int bmkId;
          int catId;
          int maxCount;

          public ShowBmkRunnable(MWMActivity activity, int bmk, int cat, int count)
          {
            this.activity = activity;
            this.bmkId = bmk;
            this.catId = cat;
            this.maxCount = count;
          }

          @Override
          public void run()
          {
            l.d("Step!");
            // bring foreground
            activity.startActivity(new Intent(activity, MWMActivity.class));

            final Bookmark bookmark = bmkManager.getBookmark(catId, bmkId);
            final BookmarkCategory category = bmkManager.getCategoryById(catId);
            final String desc = bookmark.getBookmarkDescription();

            // Center camera at bookmark
            bmkManager.showBookmarkOnMap(catId, bmkId);

            // N2DP (Nataha to Dmitry Protocol)
            // Bookmark has no description: show map
            // Bookmark has '!' description: show balloon
            // Bookmark has '$' description: show place page
            // TODO it is better to swap cases 1 and 3
            if (TextUtils.isEmpty(desc))
            {
              // if empty we hide group and ballon
              category.setVisibility(false);
              target.deactivatePopup();
            }
            else
            {
              category.setVisibility(true);
              final char code = desc.charAt(0);
              if (code == '!')
              {
                // just show pop up
              }
              else if (code == '$')
              {
                //show place page
                target.onBookmarkActivated(catId, bmkId);
              }

            }
            // do while have bookmarks to show
            if (bmkId + 1 < maxCount)
              h.postDelayed(new ShowBmkRunnable(activity, ++bmkId, catId, maxCount), 4000);
          }
        }
        h.postDelayed(new ShowBmkRunnable(target, 0, catToShowId, categoryToShow.getSize()), 0);
      }
      else
        l.d("Cant find group name:", name);

      return true;
    }

    @Override
    public ExtractableMapTask extract(Intent intent)
    {
      final ShowBookmarksTask task = new ShowBookmarksTask();
      task.name = intent.getStringExtra("name");
      l.d("Got", task);
      return task;
    }

    @Override
    public String toString()
    {
      return "ShowBookmarksTask [name=" + name + "]";
    }
  }

}
