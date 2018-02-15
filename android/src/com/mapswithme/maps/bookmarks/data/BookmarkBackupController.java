package com.mapswithme.maps.bookmarks.data;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.BookmarkBackupView;
import com.mapswithme.util.DateUtils;

import java.util.Date;

public class BookmarkBackupController
{
  @NonNull
  private final BookmarkBackupView mBackupView;
  @Nullable
  private BackupListener mListener;
  @NonNull
  private final View.OnClickListener mSignInClickListener = v ->
  {
    if (mListener != null)
      mListener.onSignInClick();
  };
  @NonNull
  private final View.OnClickListener mEnableClickListener = v ->
  {
    BookmarkManager.INSTANCE.setCloudEnabled(true);
    update();
  };


  public BookmarkBackupController(@NonNull BookmarkBackupView backupView)
  {
    mBackupView = backupView;
    update();
  }

  public void setListener(@Nullable BackupListener listener)
  {
    mListener = listener;
  }

  public void update()
  {
    final Context context = mBackupView.getContext();
    boolean isAuthorized = Framework.nativeIsUserAuthenticated();
    if (!isAuthorized)
    {
      mBackupView.setMessage(context.getString(R.string.bookmarks_message_unauthorized_user));
      mBackupView.setButtonLabel(context.getString(R.string.authorization_button_sign_in));
      mBackupView.setClickListener(mSignInClickListener);
      mBackupView.showButton();
      return;
    }

    boolean isEnabled = BookmarkManager.INSTANCE.isCloudEnabled();

    if (isEnabled)
    {
      long backupTime = BookmarkManager.INSTANCE.getLastSynchronizationTimestampInMs();
      String msg;
      if (backupTime > 0)
      {
        msg = context.getString(R.string.bookmarks_message_backuped_user,
                                DateUtils.getShortDateFormatter().format(new Date(backupTime)));
      }
      else
      {
        msg = context.getString(R.string.bookmarks_message_unbackuped_user);
      }
      mBackupView.setMessage(msg);
      mBackupView.hideButton();
      return;
    }

    // If backup is disabled.
    mBackupView.setMessage(context.getString(R.string.bookmarks_message_authorized_user));
    mBackupView.setButtonLabel(context.getString(R.string.bookmarks_backup));
    mBackupView.setClickListener(mEnableClickListener);
    mBackupView.showButton();
  }

  public interface BackupListener
  {
    void onSignInClick();
  }
}
