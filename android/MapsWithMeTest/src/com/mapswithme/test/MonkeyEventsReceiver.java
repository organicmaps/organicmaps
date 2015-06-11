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
import com.mapswithme.maps.search.SearchActivity;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

public class MonkeyEventsReceiver extends BroadcastReceiver
{


  static Logger sLogger = SimpleLogger.get("MonkeyBusiness");

  @Override
  public void onReceive(Context context, Intent intent)
  {
    ExtractableMapTask mapTask = KEY_TO_TASK.get(intent.getStringExtra(EXTRA_TASK));
    if (mapTask != null)
    {
      mapTask = mapTask.extract(intent);
      sLogger.d(mapTask.toString());
      intent = new Intent(context, MWMActivity.class);
      intent.putExtra(MWMActivity.EXTRA_TASK, mapTask);
      intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      context.startActivity(intent);
    }
    else
      sLogger.d("No task found");
  }


  public interface ExtractableMapTask extends MapTask
  {
    public ExtractableMapTask extract(Intent intent);
  };

  private static String EXTRA_TASK = "task";
  private static String TYPE_SEARCH = "task_search";
  private static String TYPE_BOOKMARK = "task_bmk";

  private static final Map<String, ExtractableMapTask> KEY_TO_TASK = new HashMap<String, ExtractableMapTask>(4);

  static
  {
    KEY_TO_TASK.put(TYPE_BOOKMARK, new ShowBookmarksTask());
    KEY_TO_TASK.put(TYPE_SEARCH, new SearchTask());
  }


  @SuppressWarnings("serial")
  public static class SearchTask implements ExtractableMapTask
  {
    String mQuery;
    int mScope;

    @Override
    public boolean run(final MWMActivity target)
    {
      sLogger.d("Running me!", this, target);
      target.onMyPositionClicked(null);
      final Handler delayHandler = new Handler();

      delayHandler.postDelayed(new Runnable()
      {

        @Override
        public void run()
        {
          // TODO finish search task
        }
      }, 1500);

      return true;
    }

    @Override
    public ExtractableMapTask extract(Intent data)
    {
      final SearchTask task = new SearchTask();

      task.mQuery = data.getStringExtra(SearchActivity.EXTRA_QUERY);
      sLogger.d("q: ", task.mQuery);

      final String scopeStr = data.getStringExtra(SearchActivity.EXTRA_SCOPE);
      sLogger.d("s: ", scopeStr);
      if (scopeStr != null)
        task.mScope = Integer.parseInt(scopeStr);

      return task;
    }

    @Override
    public String toString()
    {
      return "SearchTask [query=" + mQuery + ", scope=" + mScope + "]";
    }
  }

  @SuppressWarnings("serial")
  public static class ShowBookmarksTask implements ExtractableMapTask
  {
    String mName;

    @Override
    public boolean run(final MWMActivity target)
    {
      final int categoriesCount = BookmarkManager.INSTANCE.getCategoriesCount();

      // find category
      BookmarkCategory categoryToShow = null;
      for (int i = 0; i < categoriesCount; i++)
      {
        categoryToShow = BookmarkManager.INSTANCE.getCategoryById(i);
        if (categoryToShow.getName().contains(mName))
          break;
        else
          categoryToShow = null;
      }

      if (categoryToShow != null)
      {
        final int catToShowId = categoryToShow.getId();
        final Handler handler = new Handler();
        sLogger.d("Found category:", categoryToShow.getName());

        class ShowBmkRunnable implements Runnable
        {
          MWMActivity mActivity;
          int mBmkId;
          int mCatId;
          int mMaxCount;

          public ShowBmkRunnable(MWMActivity activity, int bmk, int cat, int count)
          {
            mActivity = activity;
            mBmkId = bmk;
            mCatId = cat;
            mMaxCount = count;
          }

          @Override
          public void run()
          {
            sLogger.d("Step!");
            // bring foreground
            mActivity.startActivity(new Intent(mActivity, MWMActivity.class));

            final Bookmark bookmark = BookmarkManager.INSTANCE.getBookmark(mCatId, mBmkId);
            final BookmarkCategory category = BookmarkManager.INSTANCE.getCategoryById(mCatId);
            final String desc = bookmark.getBookmarkDescription();

            // Center camera at bookmark
            BookmarkManager.INSTANCE.showBookmarkOnMap(mCatId, mBmkId);

            // N2DP (Nataha to Dmitry Protocol)
            // Bookmark has no description: show map
            // Bookmark has '!' description: show balloon
            // Bookmark has '$' description: show place page
            // TODO: it is better to swap cases 1 and 3
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
                target.onBookmarkActivated(mCatId, mBmkId);
              }

            }
            // do while have bookmarks to show
            if (mBmkId + 1 < mMaxCount)
              handler.postDelayed(new ShowBmkRunnable(mActivity, ++mBmkId, mCatId, mMaxCount), 4000);
          }
        }
        handler.postDelayed(new ShowBmkRunnable(target, 0, catToShowId, categoryToShow.getSize()), 0);
      }
      else
        sLogger.d("Cant find group name:", mName);

      return true;
    }

    @Override
    public ExtractableMapTask extract(Intent intent)
    {
      final ShowBookmarksTask task = new ShowBookmarksTask();
      task.mName = intent.getStringExtra("name");
      sLogger.d("Got", task);
      return task;
    }

    @Override
    public String toString()
    {
      return "ShowBookmarksTask [name=" + mName + "]";
    }
  }

}
