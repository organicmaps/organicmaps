package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.app.Application;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.widget.Toast;

import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.Authorizer;
import com.mapswithme.maps.auth.TargetFragmentCallback;
import com.mapswithme.maps.bookmarks.data.PaymentData;
import com.mapswithme.maps.dialog.ProgressDialogFragment;
import com.mapswithme.maps.purchase.BookmarkPaymentActivity;

class BookmarksDownloadFragmentDelegate implements Authorizer.Callback, BookmarkDownloadCallback,
                                                   TargetFragmentCallback
{
  private final static int REQ_CODE_PAY_BOOKMARK = 1;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private Authorizer mAuthorizer;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarkDownloadController mDownloadController;
  @NonNull
  private final Fragment mFragment;
  @Nullable
  private Runnable mAuthCompletionRunnable;

   BookmarksDownloadFragmentDelegate(@NonNull Fragment fragment)
  {
    mFragment = fragment;
  }

  void onCreate(@Nullable Bundle savedInstanceState)
  {
    mAuthorizer = new Authorizer(mFragment);
    Application application = mFragment.getActivity().getApplication();
    mDownloadController = new DefaultBookmarkDownloadController(application,
                                                                new CatalogListenerDecorator(mFragment));
    if (savedInstanceState != null)
      mDownloadController.onRestore(savedInstanceState);
  }

  void onStart()
  {
    mAuthorizer.attach(this);
    mDownloadController.attach(this);
  }

  void onStop()
  {
    mAuthorizer.detach();
    mDownloadController.detach();
  }

  void onSaveInstanceState(@NonNull Bundle outState)
  {
    mDownloadController.onSave(outState);
  }

  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (resultCode == Activity.RESULT_OK && requestCode == REQ_CODE_PAY_BOOKMARK)
    {
      mDownloadController.retryDownloadBookmark();
    }
  }

  private void showAuthorizationProgress()
  {
    String message = mFragment.getString(R.string.please_wait);
    ProgressDialogFragment dialog = ProgressDialogFragment.newInstance(message, false, true);
    mFragment.getActivity().getSupportFragmentManager()
             .beginTransaction()
             .add(dialog, dialog.getClass().getCanonicalName())
             .commitAllowingStateLoss();
  }

  private void hideAuthorizationProgress()
  {
    FragmentManager fm = mFragment.getActivity().getSupportFragmentManager();
    String tag = ProgressDialogFragment.class.getCanonicalName();
    DialogFragment frag = (DialogFragment) fm.findFragmentByTag(tag);
    if (frag != null)
      frag.dismissAllowingStateLoss();
  }

  @Override
  public void onAuthorizationFinish(boolean success)
  {
    hideAuthorizationProgress();
    if (!success)
    {
      Toast.makeText(mFragment.getContext(), R.string.profile_authorization_error,
                     Toast.LENGTH_LONG).show();
      return;
    }

    if (mAuthCompletionRunnable != null)
      mAuthCompletionRunnable.run();
  }

  @Override
  public void onAuthorizationStart()
  {
    showAuthorizationProgress();
  }

  @Override
  public void onSocialAuthenticationCancel(int type)
  {
    // Do nothing by default.
  }

  @Override
  public void onSocialAuthenticationError(int type, @Nullable String error)
  {
    // Do nothing by default.
  }

  @Override
  public void onAuthorizationRequired()
  {
    authorize(this::retryBookmarkDownload);
  }

  @Override
  public void onPaymentRequired(@NonNull PaymentData data)
  {
    BookmarkPaymentActivity.startForResult(mFragment, data, REQ_CODE_PAY_BOOKMARK);
  }

  @Override
  public void onTargetFragmentResult(int resultCode, @Nullable Intent data)
  {
    mAuthorizer.onTargetFragmentResult(resultCode, data);
  }

  @Override
  public boolean isTargetAdded()
  {
    return mFragment.isAdded();
  }

  boolean downloadBookmark(@NonNull String url)
  {
    return mDownloadController.downloadBookmark(url);
  }

  private void retryBookmarkDownload()
  {
    mDownloadController.retryDownloadBookmark();
  }

  void authorize(@NonNull Runnable completionRunnable)
  {
    mAuthCompletionRunnable = completionRunnable;
    mAuthorizer.authorize();
  }
}
