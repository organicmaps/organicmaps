package com.mapswithme.maps.bookmarks.data;

import android.content.Context;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.Authorizer;
import com.mapswithme.maps.widget.BookmarkBackupView;
import com.mapswithme.util.DateUtils;

import java.util.Date;

public class BookmarkBackupController implements Authorizer.Callback
{
  @NonNull
  private final BookmarkBackupView mBackupView;
  @Nullable
  private Authorizer mAuthorizer;
  @NonNull
  private final View.OnClickListener mSignInClickListener = v ->
  {
    if (mAuthorizer != null)
      mAuthorizer.authorize();
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
  }

  public void setAuthorizer(@Nullable Authorizer authorizer)
  {
    mAuthorizer = authorizer;
  }

  public void update()
  {
    final Context context = mBackupView.getContext();
    if (mAuthorizer != null && !mAuthorizer.isAuthorized())
    {
      mBackupView.setMessage(context.getString(R.string.bookmarks_message_unauthorized_user));
      mBackupView.setButtonLabel(context.getString(R.string.authorization_button_sign_in));
      if (mAuthorizer.isAuthorizationInProgress())
      {
        mBackupView.showProgressBar();
        mBackupView.hideButton();
      }
      else
      {
        mBackupView.hideProgressBar();
        mBackupView.setClickListener(mSignInClickListener);
        mBackupView.showButton();
      }
      return;
    }

    mBackupView.hideProgressBar();

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

  public void onStart()
  {
    if (mAuthorizer != null)
      mAuthorizer.attach(this);
    update();
  }

  public void onStop()
  {
    if (mAuthorizer != null)
      mAuthorizer.detach();
  }

  public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data)
  {
    if (mAuthorizer != null)
      mAuthorizer.onActivityResult(requestCode, resultCode, data);
  }

  @Override
  public void onAuthorizationStart()
  {
    update();
  }

  @Override
  public void onAuthorizationFinish(boolean success /* it's ignored for a while.*/)
  {
    if (success)
      BookmarkManager.INSTANCE.setCloudEnabled(true);
    update();
  }
}
