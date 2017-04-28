package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.util.SparseArray;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.downloader.DownloaderFragment;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.downloader.MigrationFragment;
import com.mapswithme.maps.editor.EditorHostFragment;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.search.SearchFragment;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Counters;
import com.mapswithme.util.concurrency.UiThread;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;

public enum LikesManager
{
  INSTANCE;


  private static final int DIALOG_DELAY_DEFAULT = 30000;
  private static final int DIALOG_DELAY_SHORT = 5000;
  private static final int SESSION_NUM = Counters.getSessionCount();

  /*
   Maps type of like dialog to the dialog, performing like.
  */
  public enum LikeType
  {
    GPLAY_NEW_USERS(RateStoreDialogFragment.class, DIALOG_DELAY_DEFAULT),
    GPLAY_OLD_USERS(RateStoreDialogFragment.class, DIALOG_DELAY_DEFAULT),
    GPLUS_NEW_USERS(GooglePlusDialogFragment.class, DIALOG_DELAY_DEFAULT),
    GPLUS_OLD_USERS(GooglePlusDialogFragment.class, DIALOG_DELAY_DEFAULT),
    FACEBOOK_INVITE_NEW_USERS(FacebookInvitesDialogFragment.class, DIALOG_DELAY_DEFAULT),
    FACEBOOK_INVITES_OLD_USERS(FacebookInvitesDialogFragment.class, DIALOG_DELAY_DEFAULT);

    public final Class<? extends DialogFragment> clazz;
    public final int delay;

    LikeType(Class<? extends DialogFragment> clazz, int delay)
    {
      this.clazz = clazz;
      this.delay = delay;
    }
  }

  /*
   Maps number of session to LikeType.
  */
  private static final SparseArray<LikeType> sOldUsersMapping = new SparseArray<>();
  private static final SparseArray<LikeType> sNewUsersMapping = new SparseArray<>();

  private static final List<Class<? extends Fragment>> sFragments = new ArrayList<>();

  static
  {
    sOldUsersMapping.put(6, LikeType.FACEBOOK_INVITES_OLD_USERS);
    sOldUsersMapping.put(24, LikeType.GPLUS_OLD_USERS);
    sOldUsersMapping.put(30, LikeType.FACEBOOK_INVITES_OLD_USERS);
    sOldUsersMapping.put(44, LikeType.GPLUS_OLD_USERS);
    sOldUsersMapping.put(50, LikeType.FACEBOOK_INVITES_OLD_USERS);
    sOldUsersMapping.put(90, LikeType.GPLAY_OLD_USERS);

    sNewUsersMapping.put(3, LikeType.GPLAY_NEW_USERS);
    sNewUsersMapping.put(9, LikeType.FACEBOOK_INVITE_NEW_USERS);
    sNewUsersMapping.put(10, LikeType.GPLAY_NEW_USERS);
    sNewUsersMapping.put(11, LikeType.GPLUS_NEW_USERS);
    sNewUsersMapping.put(21, LikeType.GPLAY_NEW_USERS);
    sNewUsersMapping.put(30, LikeType.GPLUS_NEW_USERS);
    sNewUsersMapping.put(35, LikeType.FACEBOOK_INVITE_NEW_USERS);
    sNewUsersMapping.put(50, LikeType.GPLUS_NEW_USERS);
    sNewUsersMapping.put(55, LikeType.FACEBOOK_INVITE_NEW_USERS);

    sFragments.add(SearchFragment.class);
    sFragments.add(EditorHostFragment.class);
    sFragments.add(DownloaderFragment.class);
    sFragments.add(MigrationFragment.class);
  }

  private final boolean mIsNewUser = (Counters.getFirstInstallVersion() == BuildConfig.VERSION_CODE);
  private Runnable mLikeRunnable;
  private WeakReference<FragmentActivity> mActivityRef;

  public boolean isNewUser()
  {
    return mIsNewUser;
  }

  public void showDialogs(FragmentActivity activity)
  {
    mActivityRef = new WeakReference<>(activity);

    if (!ConnectionState.isConnected())
      return;

    final LikeType type = mIsNewUser ? sNewUsersMapping.get(SESSION_NUM) : sOldUsersMapping.get(SESSION_NUM);
    if (type != null)
      displayLikeDialog(type.clazz, type.delay);
  }

  public void showRateDialogForOldUser(FragmentActivity activity)
  {
    if (mIsNewUser)
      return;

    mActivityRef = new WeakReference<>(activity);
    displayLikeDialog(LikeType.GPLAY_OLD_USERS.clazz, LikeType.GPLAY_OLD_USERS.delay);
  }

  public void cancelDialogs()
  {
    UiThread.cancelDelayedTasks(mLikeRunnable);
  }

  private boolean containsFragments(@NonNull MwmActivity activity)
  {
    for (Class<? extends Fragment> fragmentClass: sFragments)
    {
      if (activity.containsFragment(fragmentClass))
        return true;
    }

    return false;
  }

  private void displayLikeDialog(final Class<? extends DialogFragment> dialogFragmentClass, final int delayMillis)
  {
    if (Counters.isSessionRated(SESSION_NUM) || Counters.isRatingApplied(dialogFragmentClass))
      return;

    Counters.setRatedSession(SESSION_NUM);

    UiThread.cancelDelayedTasks(mLikeRunnable);
    mLikeRunnable = new Runnable()
    {
      @SuppressWarnings("TryWithIdenticalCatches")
      @Override
      public void run()
      {
        final FragmentActivity activity = mActivityRef.get();
        if (activity == null || activity.isFinishing() || RoutingController.get().isNavigating()
            || RoutingController.get().isPlanning() || MapManager.nativeIsDownloading())
        {
          return;
        }
        if (!(activity instanceof MwmActivity))
          return;
        MwmActivity mwmActivity = (MwmActivity) activity;

        if (!mwmActivity.isMapAttached() || containsFragments(mwmActivity))
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
    UiThread.runLater(mLikeRunnable, delayMillis);
  }
}
