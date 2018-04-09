package com.mapswithme.util.statistics;

import android.content.Context;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Looper;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.pushwoosh.PushManager;
import com.pushwoosh.SendPushTagsCallBack;
import ru.mail.libnotify.api.NotificationFactory;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

public final class PushwooshHelper implements SendPushTagsCallBack
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
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

  public void sendTag(String tag, Object value)
  {
    Map<String, Object> tags = new HashMap<>();
    tags.put(tag, value);
    sendTags(tags);
  }

  private void sendTags(Map<String, Object> tags)
  {
    //TODO: move notifylib code to another place when Pushwoosh is deleted.
    NotificationFactory.get(MwmApplication.get()).collectEventBatch(tags);
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
          {
            String msg = e.getLocalizedMessage();
            LOGGER.e("Pushwoosh", msg != null ? msg : "onSentTagsError");
          }
          mTask = null;
        }
      }
    });
  }

  private boolean canSendTags()
  {
    return mContext != null && mTask == null;
  }

  public static native void nativeProcessFirstLaunch();
  public static native void nativeSendEditorAddObjectTag();
  public static native void nativeSendEditorEditObjectTag();
}
