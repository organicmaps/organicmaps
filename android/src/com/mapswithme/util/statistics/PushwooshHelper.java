package com.mapswithme.util.statistics;

import android.content.Context;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.mapswithme.maps.Framework;
import com.pushwoosh.PushManager;
import com.pushwoosh.SendPushTagsCallBack;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

public final class PushwooshHelper implements SendPushTagsCallBack
{
  public final static String ADD_MAP_OBJECT = "editor_add_discovered";
  public final static String EDIT_MAP_OBJECT = "editor_edit_discovered";

  private static final PushwooshHelper sInstance = new PushwooshHelper();

  private WeakReference<Context> mContext;

  private final Object mSyncObject = new Object();
  private AsyncTask<Void, Void, Void> mTask;
  private List<Map<String, Object>> mTagsQueue = new LinkedList<>();

  private PushwooshHelper() {}

  public static PushwooshHelper get() { return sInstance; }

  public void setContext(Context context)
  {
    synchronized (mSyncObject)
    {
      mContext = new WeakReference<>(context);
    }
  }

  public void synchronize()
  {
    sendTags(null);
  }

  public void sendTag(String tag)
  {
    sendTag(tag, "1");
  }

  public void sendTag(String tag, Object value)
  {
    Map<String, Object> tags = new HashMap<>();
    tags.put(tag, value);
    sendTags(tags);
  }

  public void sendTags(Map<String, Object> tags)
  {
    synchronized (mSyncObject)
    {
      if (!canSendTags())
      {
        mTagsQueue.add(tags);
        return;
      }

      final Map<String, Object> tagsToSend = new HashMap<>();
      for (Map<String, Object> t: mTagsQueue)
      {
        if (t != null)
          tagsToSend.putAll(t);
      }
      if (tags != null)
        tagsToSend.putAll(tags);

      mTagsQueue.clear();

      if (tagsToSend.isEmpty())
        return;

      mTask = new AsyncTask<Void, Void, Void>()
      {
        @Override
        protected Void doInBackground(Void... params)
        {
          final Context context = mContext.get();
          if (context == null)
            return null;

          PushManager.sendTags(context, tagsToSend, PushwooshHelper.this);
          return null;
        }
      };
      mTask.execute((Void) null);
    }
  }

  @Override
  public void taskStarted() {}

  @Override
  public void onSentTagsSuccess(Map<String, String> map)
  {
    new Handler(Looper.getMainLooper()).post(new Runnable()
    {
      @Override
      public void run()
      {
        synchronized (mSyncObject)
        {
          mTask = null;
        }
      }
    });
  }

  @Override
  public void onSentTagsError(final Exception e)
  {
    new Handler(Looper.getMainLooper()).post(new Runnable()
    {
      @Override
      public void run()
      {
        synchronized (mSyncObject)
        {
          if (e != null)
            Log.e("Pushwoosh", e.getLocalizedMessage());
          mTask = null;
        }
      }
    });
  }

  private boolean canSendTags()
  {
    return mContext != null && mTask == null;
  }

  public static String getRoutingTag(int routerType, boolean isP2P)
  {
    String result = "routing_";
    if (isP2P)
      result += "p2p_";

    if (routerType == Framework.ROUTER_TYPE_VEHICLE)
      result += "vehicle";
    else if (routerType == Framework.ROUTER_TYPE_PEDESTRIAN)
      result += "pedestrian";
    else if (routerType == Framework.ROUTER_TYPE_BICYCLE)
      result += "bicycle";
    else
      result += "unknown";

    result += "_discovered";
    return result;
  }
}
