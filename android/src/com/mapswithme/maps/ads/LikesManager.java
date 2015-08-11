package com.mapswithme.maps.ads;

import android.os.Handler;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentActivity;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.ConnectionState;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

public class LikesManager
{
  /*
   Maps type of like dialog to the dialog, performing like.
   */
  private enum LikeType
  {
    GPLAY_NEW_USERS(RateStoreDialogFragment.class),
    GPLAY_OLD_USERS(RateStoreDialogFragment.class),
    GPLUS_NEW_USERS(GooglePlusDialogFragment.class),
    GPLUS_OLD_USERS(GooglePlusDialogFragment.class),
    FACEBOOK_INVITE_NEW_USERS(FacebookInvitesDialogFragment.class),
    FACEBOOK_INVITES_OLD_USERS(FacebookInvitesDialogFragment.class);

    public final Class<? extends DialogFragment> clazz;

    LikeType(Class<? extends DialogFragment> clazz)
    {
      this.clazz = clazz;
    }
  }

  /*
   Maps number of session to LikeType
   */
  private static Map<Integer, LikeType> mOldUsersMapping = new HashMap<>();
  private static Map<Integer, LikeType> mNewUsersMapping = new HashMap<>();

  static
  {
    mOldUsersMapping.put(1, LikeType.GPLAY_OLD_USERS);
    mOldUsersMapping.put(4, LikeType.GPLUS_OLD_USERS);
    mOldUsersMapping.put(6, LikeType.FACEBOOK_INVITES_OLD_USERS);
    mOldUsersMapping.put(10, LikeType.GPLAY_OLD_USERS);
    mOldUsersMapping.put(21, LikeType.GPLAY_OLD_USERS);
    mOldUsersMapping.put(24, LikeType.GPLUS_OLD_USERS);
    mOldUsersMapping.put(30, LikeType.FACEBOOK_INVITES_OLD_USERS);
    mOldUsersMapping.put(44, LikeType.GPLUS_OLD_USERS);
    mOldUsersMapping.put(50, LikeType.FACEBOOK_INVITES_OLD_USERS);

    mNewUsersMapping.put(3, LikeType.GPLAY_NEW_USERS);
    mNewUsersMapping.put(9, LikeType.FACEBOOK_INVITE_NEW_USERS);
    mNewUsersMapping.put(10, LikeType.GPLAY_NEW_USERS);
    mNewUsersMapping.put(11, LikeType.GPLUS_NEW_USERS);
    mNewUsersMapping.put(21, LikeType.GPLAY_NEW_USERS);
    mNewUsersMapping.put(30, LikeType.GPLUS_NEW_USERS);
    mNewUsersMapping.put(35, LikeType.FACEBOOK_INVITE_NEW_USERS);
    mNewUsersMapping.put(50, LikeType.GPLUS_NEW_USERS);
    mNewUsersMapping.put(55, LikeType.FACEBOOK_INVITE_NEW_USERS);
  }

  private static final long DIALOG_DELAY_MILLIS = 30000;

  private final boolean mIsNewUser = MwmApplication.get().getFirstInstallVersion() == BuildConfig.VERSION_CODE;
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

    mSessionNum = MwmApplication.get().getSessionsNumber();
  }

  public void showLikeDialogForCurrentSession()
  {
    if (!ConnectionState.isConnected())
      return;

    final LikeType type = mIsNewUser ? mNewUsersMapping.get(mSessionNum) : mOldUsersMapping.get(mSessionNum);
    if (type != null)
      displayLikeDialog(type.clazz);
  }

  private void displayLikeDialog(final Class<? extends DialogFragment> dialogFragmentClass)
  {
    if (isSessionRated(mSessionNum) || isRatingApplied(dialogFragmentClass))
      return;
    setSessionRated(mSessionNum);

    mHandler.removeCallbacks(mLikeRunnable);
    mLikeRunnable = new Runnable()
    {
      @SuppressWarnings("TryWithIdenticalCatches")
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

  public void cancelLikeDialog()
  {
    mHandler.removeCallbacks(mLikeRunnable);
  }

  public static boolean isSessionRated(int sessionNum)
  {
    return MwmApplication.get().nativeGetInt(LAST_RATED_SESSION, 0) >= sessionNum;
  }

  public static void setSessionRated(int sessionNum)
  {
    MwmApplication.get().nativeSetInt(LAST_RATED_SESSION, sessionNum);
  }

  public static boolean isRatingApplied(final Class<? extends DialogFragment> dialogFragmentClass)
  {
    return MwmApplication.get().nativeGetBoolean(RATED_DIALOG + dialogFragmentClass.getSimpleName(), false);
  }

  public static void setRatingApplied(final Class<? extends DialogFragment> dialogFragmentClass, boolean applied)
  {
    MwmApplication.get().nativeSetBoolean(RATED_DIALOG + dialogFragmentClass.getSimpleName(), applied);
  }
}
