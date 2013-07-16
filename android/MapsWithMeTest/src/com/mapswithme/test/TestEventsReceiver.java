package com.mapswithme.test;

import java.util.HashMap;
import java.util.Map;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MWMActivity.MapTask;
import com.mapswithme.maps.SearchActivity;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

public class TestEventsReceiver extends BroadcastReceiver
{


  static Logger l = SimpleLogger.get("TestBro");

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
  private static String TYPE_POINT = "task_point";
  private static String TYPE_BOOKMARK = "task_bmk";
  private static String TYPE_PPAGE = "task_ppage";

  private static final Map<String, ExtractableMapTask> KEY_TO_TASK = new HashMap<String, ExtractableMapTask>(4);

  static {
    KEY_TO_TASK.put(TYPE_POINT, null);
    KEY_TO_TASK.put(TYPE_BOOKMARK, null);
    KEY_TO_TASK.put(TYPE_SEARCH, new SearchTask());
    KEY_TO_TASK.put(TYPE_PPAGE, null);
  }


  @SuppressWarnings("serial")
  public static class SearchTask implements ExtractableMapTask
  {
    String query;
    int scope;

    @Override
    public boolean run(MWMActivity target)
    {
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

}
