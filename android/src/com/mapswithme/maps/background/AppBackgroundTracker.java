package com.mapswithme.maps.background;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;
import android.util.SparseArray;

import java.lang.ref.WeakReference;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.Listeners;
import com.mapswithme.util.concurrency.UiThread;

/**
 * Helper class that detects when the application goes to background and back to foreground.
 * <br/>Must be created as early as possible, i.e. in Application.Create().
 */
public final class AppBackgroundTracker
{
  private static final int TRANSITION_DELAY = 3000;

  private final Listeners<OnTransitionListener> mListeners = new Listeners<>();
  private SparseArray<WeakReference<Activity>> mActivities = new SparseArray<>();
  private boolean mForeground;

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
        if (ref.get() != null)
          newArray.put(key, ref);
      }

      mActivities = newArray;
      boolean old = mForeground;
      mForeground = (mActivities.size() > 0);

      if (mForeground != old)
        notifyListeners();
    }
  };

  /** @noinspection FieldCanBeLocal*/
  private final Application.ActivityLifecycleCallbacks mAppLifecycleCallbacks = new Application.ActivityLifecycleCallbacks()
  {
    private void onActivityChanged()
    {
      UiThread.cancelDelayedTasks(mTransitionProc);
      UiThread.runLater(mTransitionProc, TRANSITION_DELAY);
    }

    @Override
    public void onActivityStarted(Activity activity)
    {
      mActivities.put(activity.hashCode(), new WeakReference<>(activity));
      onActivityChanged();
    }

    @Override
    public void onActivityStopped(Activity activity)
    {
      mActivities.remove(activity.hashCode());
      onActivityChanged();
    }

    @Override
    public void onActivityCreated(Activity activity, Bundle savedInstanceState) {}

    @Override
    public void onActivityDestroyed(Activity activity) {}

    @Override
    public void onActivityResumed(Activity activity) {}

    @Override
    public void onActivityPaused(Activity activity) {}

    @Override
    public void onActivitySaveInstanceState(Activity activity, Bundle outState) {}
  };

  public interface OnTransitionListener
  {
    void onTransit(boolean foreground);
  }

  public AppBackgroundTracker()
  {
    MwmApplication.get().registerActivityLifecycleCallbacks(mAppLifecycleCallbacks);
  }

  public boolean isForeground()
  {
    return mForeground;
  }

  private void notifyListeners()
  {
    for (OnTransitionListener listener: mListeners)
      listener.onTransit(mForeground);
    mListeners.finishIterate();
  }

  public void addListener(OnTransitionListener listener)
  {
    mListeners.register(listener);
  }

  public void removeListener(OnTransitionListener listener)
  {
    mListeners.unregister(listener);
  }
}
