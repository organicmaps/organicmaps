package com.mapswithme.maps.ads;

import android.os.Handler;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentActivity;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.util.ConnectionState;

import java.lang.ref.WeakReference;
import java.util.Arrays;

public class LikesManager
{
  public static final Integer[] GPLAY_NEW_USERS = new Integer[]{3, 7, 10, 15, 21};
  public static final Integer[] GPLUS_NEW_USERS = new Integer[]{11, 20, 30, 40, 50};
  public static final Integer[] GPLAY_OLD_USERS = new Integer[]{1, 7, 10, 15, 21};
  public static final Integer[] GPLUS_OLD_USERS = new Integer[]{4, 14, 24, 34, 44};

  private static final long DIALOG_DELAY_MILLIS = 30000;

  private final boolean mIsNewUser;
  private final int mSessionNum;

  private Handler mHandler;
  private Runnable mLikeRunnable;
  private WeakReference<FragmentActivity> mActivityRef;

  public static final String LAST_RATED_SESSION = "LastRatedSession";
  public static final String RATED_DIALOG = "RatedDialog";

  public LikesManager(FragmentActivity activity)
  {
    mHandler = new Handler(activity.getMainLooper());
    mActivityRef = new WeakReference<>(activity);

    mIsNewUser = MWMApplication.get().getFirstInstallVersion() >= 422;
    mSessionNum = MWMApplication.get().getSessionsNumber();
  }

  public void showLikeDialogs()
  {
    if (!ConnectionState.isConnected())
      return;

    if (mIsNewUser)
    {
      if (Arrays.asList(GPLAY_NEW_USERS).contains(mSessionNum))
        displayLikeDialog(RateStoreDialogFragment.class);
      else if (Arrays.asList(GPLUS_NEW_USERS).contains(mSessionNum))
        displayLikeDialog(GooglePlusDialogFragment.class);
    }
    else
    {
      if (Arrays.asList(GPLAY_OLD_USERS).contains(mSessionNum))
        displayLikeDialog(RateStoreDialogFragment.class);
      else if (Arrays.asList(GPLUS_OLD_USERS).contains(mSessionNum))
        displayLikeDialog(GooglePlusDialogFragment.class);
    }
  }

  private void displayLikeDialog(final Class<? extends DialogFragment> dialogFragmentClass)
  {
    if (isSessionRated() || isRatingApplied(dialogFragmentClass))
      return;
    setSessionRated();

    mHandler.removeCallbacks(mLikeRunnable);
    mLikeRunnable = new Runnable()
    {
      @Override
      public void run()
      {
        final FragmentActivity activity = mActivityRef.get();
        if (activity == null)
          return;

        final DialogFragment fragment;
        try
        {
          fragment = dialogFragmentClass.newInstance();
          fragment.show(activity.getSupportFragmentManager(), null);
        } catch (InstantiationException e)
        {
          e.printStackTrace();
        } catch (IllegalAccessException e)
        {
          e.printStackTrace();
        }
      }
    };
    mHandler.postDelayed(mLikeRunnable, DIALOG_DELAY_MILLIS);
  }

  public void cancelLikeDialogs()
  {
    mHandler.removeCallbacks(mLikeRunnable);
  }

  public boolean isSessionRated()
  {
    return MWMApplication.get().nativeGetInt(LAST_RATED_SESSION, 0) >= mSessionNum;
  }

  public void setSessionRated()
  {
    MWMApplication.get().nativeSetInt(LAST_RATED_SESSION, mSessionNum);
  }

  public static boolean isRatingApplied(final Class<? extends DialogFragment> dialogFragmentClass)
  {
    return MWMApplication.get().nativeGetBoolean(RATED_DIALOG + dialogFragmentClass.getSimpleName(), false);
  }

  public static void setRatingApplied(final Class<? extends DialogFragment> dialogFragmentClass, boolean applied)
  {
    MWMApplication.get().nativeSetBoolean(RATED_DIALOG + dialogFragmentClass.getSimpleName(), applied);
  }
}
