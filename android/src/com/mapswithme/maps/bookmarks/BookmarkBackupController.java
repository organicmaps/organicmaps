package com.mapswithme.maps.bookmarks;

import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.FragmentActivity;
import android.view.View;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.Authorizer;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.widget.BookmarkBackupView;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.DateUtils;
import com.mapswithme.util.DialogUtils;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

import java.util.Date;

import static com.mapswithme.maps.bookmarks.data.BookmarkManager.CLOUD_AUTH_ERROR;
import static com.mapswithme.maps.bookmarks.data.BookmarkManager.CLOUD_BACKUP;
import static com.mapswithme.maps.bookmarks.data.BookmarkManager.CLOUD_BACKUP_EXISTS;
import static com.mapswithme.maps.bookmarks.data.BookmarkManager.CLOUD_DISK_ERROR;
import static com.mapswithme.maps.bookmarks.data.BookmarkManager.CLOUD_INVALID_CALL;
import static com.mapswithme.maps.bookmarks.data.BookmarkManager.CLOUD_NETWORK_ERROR;
import static com.mapswithme.maps.bookmarks.data.BookmarkManager.CLOUD_NOT_ENOUGH_DISK_SPACE;
import static com.mapswithme.maps.bookmarks.data.BookmarkManager.CLOUD_NO_BACKUP;
import static com.mapswithme.maps.bookmarks.data.BookmarkManager.CLOUD_RESTORE;
import static com.mapswithme.maps.bookmarks.data.BookmarkManager.CLOUD_SUCCESS;
import static com.mapswithme.maps.bookmarks.data.BookmarkManager.CLOUD_USER_INTERRUPTED;

public class BookmarkBackupController implements Authorizer.Callback,
                                                 BookmarkManager.BookmarksCloudListener
{
  @NonNull
  private final FragmentActivity mContext;
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
      Statistics.INSTANCE.trackBmSyncProposalApproved(false);
    }
  };
  @NonNull
  private final View.OnClickListener mEnableClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      BookmarkManager.INSTANCE.setCloudEnabled(true);
      updateWidget();
      Statistics.INSTANCE.trackBmSyncProposalApproved(mAuthorizer.isAuthorized());
    }
  };
  @NonNull
  private final View.OnClickListener mRestoreClickListener = v -> requestRestoring();
  @Nullable
  private ProgressDialog mRestoringProgressDialog;

  BookmarkBackupController(@NonNull FragmentActivity context, @NonNull BookmarkBackupView backupView,
                           @NonNull Authorizer authorizer)
  {
    mContext = context;
    mBackupView = backupView;
    mAuthorizer = authorizer;
  }

  private void requestRestoring()
  {
    if (!ConnectionState.isConnected())
    {
      DialogInterface.OnClickListener clickListener
          = (dialog, which) -> Utils.showSystemSettings(mContext);
      DialogUtils.showAlertDialog(mContext, R.string.common_check_internet_connection_dialog_title,
                                  R.string.common_check_internet_connection_dialog,
                                  R.string.settings, clickListener, R.string.ok);
      return;
    }

    NetworkPolicy.NetworkPolicyListener policyListener = policy -> {
      BookmarkManager.INSTANCE.requestRestoring();
    };

    NetworkPolicy.checkNetworkPolicy(mContext.getSupportFragmentManager(), policyListener);
  }

  private void showRestoringProgressDialog()
  {
    if (mRestoringProgressDialog != null && mRestoringProgressDialog.isShowing())
      throw new AssertionError("Previous progress must be dismissed before " +
                               "showing another one!");
    DialogInterface.OnClickListener cancelListener = (dialog, which) -> {
      BookmarkManager.INSTANCE.cancelRestoring();
    };
    mRestoringProgressDialog = DialogUtils.createModalProgressDialog(mContext,
                                                                     R.string.bookmarks_restore_process,
                                                                     DialogInterface.BUTTON_NEGATIVE,
                                                                     R.string.cancel, cancelListener);
    mRestoringProgressDialog.show();
  }

  private void hideRestoringProgressDialog()
  {
    if (mRestoringProgressDialog == null || !mRestoringProgressDialog.isShowing())
      return;

    mRestoringProgressDialog.dismiss();
    mRestoringProgressDialog = null;
  }

  private void updateWidget()
  {
    if (!mAuthorizer.isAuthorized())
    {
      mBackupView.setMessage(mContext.getString(R.string.bookmarks_message_unauthorized_user));
      mBackupView.setBackupButtonLabel(mContext.getString(R.string.authorization_button_sign_in));
      if (mAuthorizer.isAuthorizationInProgress())
      {
        mBackupView.showProgressBar();
        mBackupView.hideBackupButton();
        mBackupView.hideRestoreButton();
      }
      else
      {
        mBackupView.hideProgressBar();
        mBackupView.setBackupClickListener(mSignInClickListener);
        mBackupView.showBackupButton();
        mBackupView.hideRestoreButton();
        Statistics.INSTANCE.trackBmSyncProposalShown(mAuthorizer.isAuthorized());
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
        msg = mContext.getString(R.string.bookmarks_message_backuped_user,
                                 DateUtils.getShortDateFormatter().format(new Date(backupTime)));
      }
      else
      {
        msg = mContext.getString(R.string.bookmarks_message_unbackuped_user);
      }
      mBackupView.setMessage(msg);
      mBackupView.hideBackupButton();
      mBackupView.setRestoreClickListener(mRestoreClickListener);
      mBackupView.showRestoreButton();
      return;
    }

    // If backup is disabled.
    mBackupView.setMessage(mContext.getString(R.string.bookmarks_message_authorized_user));
    mBackupView.setBackupButtonLabel(mContext.getString(R.string.bookmarks_backup));
    mBackupView.setBackupClickListener(mEnableClickListener);
    mBackupView.showBackupButton();
    mBackupView.hideRestoreButton();
    Statistics.INSTANCE.trackBmSyncProposalShown(mAuthorizer.isAuthorized());
  }

  public void onStart()
  {
    mAuthorizer.attach(this);
    BookmarkManager.INSTANCE.addCloudListener(this);
    updateWidget();
  }

  public void onStop()
  {
    mAuthorizer.detach();
    BookmarkManager.INSTANCE.removeCloudListener(this);
  }

  public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data)
  {
    mAuthorizer.onActivityResult(requestCode, resultCode, data);
  }

  @Override
  public void onAuthorizationStart()
  {
    updateWidget();
  }

  @Override
  public void onAuthorizationFinish(boolean success)
  {
    if (success)
    {
      Notifier.cancelNotification(Notifier.ID_IS_NOT_AUTHENTICATED);
      BookmarkManager.INSTANCE.setCloudEnabled(true);
      Statistics.INSTANCE.trackEvent(Statistics.EventName.BM_SYNC_PROPOSAL_ENABLED);
    }
    else
    {
      Statistics.INSTANCE.trackBmSyncProposalError(Framework.TOKEN_MAPSME, "Unknown error");
    }
    updateWidget();
  }

  @Override
  public void onSocialAuthenticationError(@Framework.AuthTokenType int type, @Nullable String error)
  {
    Statistics.INSTANCE.trackBmSyncProposalError(type, error);
  }

  @Override
  public void onSocialAuthenticationCancel(@Framework.AuthTokenType int type)
  {
    Statistics.INSTANCE.trackBmSyncProposalError(type, "Cancel");
  }

  @Override
  public void onSynchronizationStarted(@BookmarkManager.SynchronizationType int type)
  {
    switch (type)
    {
      case CLOUD_BACKUP:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.BM_SYNC_STARTED);
        break;
      case CLOUD_RESTORE:
        showRestoringProgressDialog();
        break;
      default:
        throw new AssertionError("Unsupported synchronization type: " + type);
    }
    updateWidget();
  }

  @Override
  public void onSynchronizationFinished(@BookmarkManager.SynchronizationType int type,
                                        @BookmarkManager.SynchronizationResult int result,
                                        @NonNull String errorString)
  {
    hideRestoringProgressDialog();

    updateWidget();

    if (type == CLOUD_BACKUP)
    {
      Statistics.INSTANCE.trackBmSynchronizationFinish(type, result, errorString);
      return;
    }

    // Restoring is finished.
    switch (result)
    {
      case CLOUD_AUTH_ERROR:
      case CLOUD_NETWORK_ERROR:
        DialogInterface.OnClickListener clickListener
            = (dialog, which) -> requestRestoring();
        DialogUtils.showAlertDialog(mContext, R.string.error_server_title,
                                    mContext.getString(R.string.error_server_message),
                                    R.string.try_again, clickListener, R.string.cancel);
        break;
      case CLOUD_DISK_ERROR:
        DialogUtils.showAlertDialog(mContext, R.string.dialog_routing_system_error,
                                    R.string.error_system_message);
        break;
      case CLOUD_INVALID_CALL:
      case CLOUD_USER_INTERRUPTED:
      case CLOUD_SUCCESS:
        // Do nothing.
        break;
      default:
        throw new AssertionError("Unsupported synchronization result: " + result + "," +
                                 " error message: " + errorString);
    }
  }

  @Override
  public void onRestoreRequested(@BookmarkManager.RestoringRequestResult int result,
                                 long backupTimestampInMs)
  {
    hideRestoringProgressDialog();

    final DialogInterface.OnClickListener cancelListener
        = (dialog, which) -> BookmarkManager.INSTANCE.cancelRestoring();

    switch (result)
    {
      case CLOUD_BACKUP_EXISTS:
        String backupDate = DateUtils.getShortDateFormatter().format(new Date(backupTimestampInMs));
        DialogInterface.OnClickListener clickListener = (dialog, which) -> {
          showRestoringProgressDialog();
          BookmarkManager.INSTANCE.applyRestoring();
        };
        String msg = mContext.getString(R.string.bookmarks_restore_message, backupDate);
        DialogUtils.showAlertDialog(mContext, R.string.bookmarks_restore_title, msg,
                                    R.string.restore, clickListener, R.string.cancel, cancelListener);
        break;
      case CLOUD_NO_BACKUP:
        DialogUtils.showAlertDialog(mContext, R.string.bookmarks_restore_empty_title,
                                    mContext.getString(R.string.bookmarks_restore_empty_message),
                                    R.string.ok, cancelListener);
        break;
      case CLOUD_NOT_ENOUGH_DISK_SPACE:
        DialogInterface.OnClickListener tryAgainListener
            = (dialog, which) -> BookmarkManager.INSTANCE.requestRestoring();
        DialogUtils.showAlertDialog(mContext, R.string.routing_not_enough_space,
                                    R.string.not_enough_free_space_on_sdcard,
                                    R.string.try_again, tryAgainListener, R.string.cancel,
                                    cancelListener);
        break;
      default:
        throw new AssertionError("Unsupported restoring request result: " + result);
    }
  }

  @Override
  public void onRestoredFilesPrepared()
  {
    // No op.
  }
}
