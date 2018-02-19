package com.mapswithme.maps.bookmarks.data;

import android.content.Context;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.Authorizer;
import com.mapswithme.maps.widget.BookmarkBackupView;
import com.mapswithme.util.DateUtils;
import com.mapswithme.util.statistics.Statistics;

import java.util.Date;

public class BookmarkBackupController implements Authorizer.Callback
{
  @NonNull
  private final BookmarkBackupView mBackupView;
  @NonNull
  private final Authorizer mAuthorizer;
  @NonNull
  private final View.OnClickListener mSignInClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      mAuthorizer.authorize();
      Statistics.INSTANCE.trackBkmSyncProposalApproved(false);
    }
  };
  @NonNull
  private final View.OnClickListener mEnableClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      BookmarkManager.INSTANCE.setCloudEnabled(true);
      update();
      Statistics.INSTANCE.trackBkmSyncProposalApproved(mAuthorizer.isAuthorized());
    }
  };

  public BookmarkBackupController(@NonNull BookmarkBackupView backupView, @NonNull Authorizer authorizer)
  {
    mBackupView = backupView;
    mAuthorizer = authorizer;
  }

  public void update()
  {
    final Context context = mBackupView.getContext();
    if (!mAuthorizer.isAuthorized())
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
        Statistics.INSTANCE.trackBkmSyncProposalShown(mAuthorizer.isAuthorized());
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
    Statistics.INSTANCE.trackBkmSyncProposalShown(mAuthorizer.isAuthorized());
  }

  public void onStart()
  {
    mAuthorizer.attach(this);
    update();
  }

  public void onStop()
  {
    mAuthorizer.detach();
  }

  public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data)
  {
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
    {
      BookmarkManager.INSTANCE.setCloudEnabled(true);
      Statistics.INSTANCE.trackEvent(Statistics.EventName.BMK_SYNC_PROPOSAL_ENABLED);
    }
    else
    {
      Statistics.INSTANCE.trackBkmSyncProposalError(Framework.TOKEN_MAPSME, "Unknown error");
    }
    update();
  }

  @Override
  public void onSocialAuthenticationError(@Framework.AuthTokenType int type, @Nullable String error)
  {
    Statistics.INSTANCE.trackBkmSyncProposalError(type, error);
  }

  @Override
  public void onSocialAuthenticationCancel(@Framework.AuthTokenType int type)
  {
    Statistics.INSTANCE.trackBkmSyncProposalError(type, "Cancel");
  }
}
