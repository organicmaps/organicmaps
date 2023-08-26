package app.organicmaps.background;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.os.Bundle;
import android.util.SparseArray;

import androidx.annotation.NonNull;
import app.organicmaps.MwmApplication;
import app.organicmaps.util.Listeners;
import app.organicmaps.util.concurrency.UiThread;
import app.organicmaps.util.log.Logger;

import java.lang.ref.WeakReference;

/**
 * Helper class that detects when the application goes to background and back to foreground.
 * <br/>Must be created as early as possible, i.e. in Application.onCreate().
 */
public final class AppBackgroundTracker
{
  private static final String TAG = AppBackgroundTracker.class.getSimpleName();
  private static final int TRANSITION_DELAY_MS = 1000;

  private final Listeners<OnTransitionListener> mTransitionListeners = new Listeners<>();
  private final Listeners<OnVisibleAppLaunchListener> mVisibleAppLaunchListeners = new Listeners<>();
  private SparseArray<WeakReference<Activity>> mActivities = new SparseArray<>();
  private volatile boolean mForeground;

  private final Runnable mTransitionProc = new Runnable()
  {
    @Override
    public void run()
    {
      SparseArray<WeakReference<Activity>> newArray = new SparseArray<>();
      for (int i = 0; i < mActivities.size(); i++)
      {
        int key = mActivities.keyAt(i);
        WeakReference<Activity> ref = mActivities.get(key);
        Activity activity = ref.get();
        if (activity != null && !activity.isFinishing())
          newArray.put(key, ref);
      }

      mActivities = newArray;
      boolean old = mForeground;
      mForeground = (mActivities.size() > 0);

      if (mForeground != old)
        notifyTransitionListeners();
    }
  };

  /** @noinspection FieldCanBeLocal*/
  private final Application.ActivityLifecycleCallbacks mAppLifecycleCallbacks = new Application.ActivityLifecycleCallbacks()
  {
    private void onActivityChanged()
    {
      UiThread.cancelDelayedTasks(mTransitionProc);
      UiThread.runLater(mTransitionProc, TRANSITION_DELAY_MS);
    }

    @Override
    public void onActivityStarted(Activity activity)
    {
      Logger.d(TAG, "onActivityStarted activity = " + activity);
      if (mActivities.size() == 0)
        notifyVisibleAppLaunchListeners();
      mActivities.put(activity.hashCode(), new WeakReference<>(activity));
      onActivityChanged();
    }

    @Override
    public void onActivityStopped(Activity activity)
    {
      Logger.d(TAG, "onActivityStopped activity = " + activity);
      mActivities.remove(activity.hashCode());
      onActivityChanged();
    }

    @Override
    public void onActivityCreated(Activity activity, Bundle savedInstanceState)
    {
      Logger.d(TAG, "onActivityCreated activity = " + activity);
    }

    @Override
    public void onActivityDestroyed(Activity activity)
    {
      Logger.d(TAG, "onActivityDestroyed activity = " + activity);
    }

    @Override
    public void onActivityResumed(Activity activity)
    {
      Logger.d(TAG, "onActivityResumed activity = " + activity);
    }

    @Override
    public void onActivityPaused(Activity activity)
    {
      Logger.d(TAG, "onActivityPaused activity = " + activity);
    }

    @Override
    public void onActivitySaveInstanceState(Activity activity, Bundle outState)
    {
      Logger.d(TAG, "onActivitySaveInstanceState activity = " + activity);
    }
  };

  public interface OnTransitionListener
  {
    void onTransit(boolean foreground);
  }

  public interface OnVisibleAppLaunchListener
  {
    void onVisibleAppLaunch();
  }

  public AppBackgroundTracker(@NonNull Context context)
  {
    MwmApplication.from(context).registerActivityLifecycleCallbacks(mAppLifecycleCallbacks);
  }

  public boolean isForeground()
  {
    return mForeground;
  }

  private void notifyTransitionListeners()
  {
    for (OnTransitionListener listener : mTransitionListeners)
      listener.onTransit(mForeground);
    mTransitionListeners.finishIterate();
  }

  private void notifyVisibleAppLaunchListeners()
  {
    for (OnVisibleAppLaunchListener listener : mVisibleAppLaunchListeners)
      listener.onVisibleAppLaunch();
    mVisibleAppLaunchListeners.finishIterate();
  }

  public void addListener(OnTransitionListener listener)
  {
    mTransitionListeners.register(listener);
  }

  public void removeListener(OnTransitionListener listener)
  {
    mTransitionListeners.unregister(listener);
  }

  public void addListener(OnVisibleAppLaunchListener listener)
  {
    mVisibleAppLaunchListeners.register(listener);
  }

  public void removeListener(OnVisibleAppLaunchListener listener)
  {
    mVisibleAppLaunchListeners.unregister(listener);
  }

  @androidx.annotation.UiThread
  public Activity getTopActivity()
  {
    return (mActivities.size() == 0 ? null : mActivities.get(mActivities.keyAt(0)).get());
  }
}
