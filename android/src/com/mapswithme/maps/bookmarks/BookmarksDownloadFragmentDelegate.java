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
import com.mapswithme.maps.base.Detachable;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.PaymentData;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.dialog.ConfirmationDialogFactory;
import com.mapswithme.maps.dialog.ProgressDialogFragment;
import com.mapswithme.maps.purchase.BookmarkPaymentActivity;

class BookmarksDownloadFragmentDelegate implements Authorizer.Callback, BookmarkDownloadCallback,
                                                   TargetFragmentCallback
{
  public static final int REQ_CODE_SUBSCRIPTION_ACTIVITY = 301;
  private final static int REQ_CODE_PAY_BOOKMARK = 1;
  private static final int REQ_CODE_CHECK_INVALID_SUBS_DIALOG = 300;
  private static final String CHECK_INVALID_SUBS_DIALOG_TAG = "check_invalid_subs_dialog_tag";

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

  @NonNull
  private final InvalidCategoriesListener mInvalidCategoriesListener;

   BookmarksDownloadFragmentDelegate(@NonNull Fragment fragment)
  {
    mFragment = fragment;
    mInvalidCategoriesListener = new InvalidCategoriesListener(fragment);
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
    mInvalidCategoriesListener.attach(mFragment);
  }

  void onStop()
  {
    mAuthorizer.detach();
    mDownloadController.detach();
    mInvalidCategoriesListener.detach();
  }

  void onCreateView(@Nullable Bundle savedInstanceState)
  {
    BookmarkManager.INSTANCE.addInvalidCategoriesListener(mInvalidCategoriesListener);
    if (savedInstanceState != null)
      return;

    BookmarkManager.INSTANCE.checkInvalidCategories();
  }

  void onDestroyView()
  {
    BookmarkManager.INSTANCE.removeInvalidCategoriesListener(mInvalidCategoriesListener);
  }

  void onSaveInstanceState(@NonNull Bundle outState)
  {
    mDownloadController.onSave(outState);
  }

  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (resultCode == Activity.RESULT_OK && requestCode == REQ_CODE_SUBSCRIPTION_ACTIVITY)
      BookmarkManager.INSTANCE.resetInvalidCategories();

    if (resultCode == Activity.RESULT_OK && requestCode == REQ_CODE_PAY_BOOKMARK)
      mDownloadController.retryDownloadBookmark();
    else if (requestCode == REQ_CODE_PAY_BOOKMARK)
      mFragment.requireActivity().finish();
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

  private static class InvalidCategoriesListener implements BookmarkManager.BookmarksInvalidCategoriesListener, Detachable<Fragment>
  {
    @Nullable
    private Fragment mFrag;
    @Nullable
    private Boolean mPendingInvalidCategoriesResult;

    public InvalidCategoriesListener(@NonNull Fragment fragment)
    {
      mFrag = fragment;
    }

    @Override
    public void onCheckInvalidCategories(boolean hasInvalidCategories)
    {
      BookmarkManager.INSTANCE.removeInvalidCategoriesListener(this);

      if (mFrag == null)
      {
        mPendingInvalidCategoriesResult = hasInvalidCategories;
        return;
      }

      if (!hasInvalidCategories)
        return;

      showDialog();
    }

    private void showDialog()
    {
      if (mFrag == null)
        return;

      AlertDialog dialog = new AlertDialog.Builder()
          .setTitleId(R.string.renewal_screen_title)
          .setMessageId(R.string.renewal_screen_message)
          .setPositiveBtnId(R.string.renewal_screen_button_restore)
          .setNegativeBtnId(R.string.renewal_screen_button_cancel)
          .setReqCode(REQ_CODE_CHECK_INVALID_SUBS_DIALOG)
          .setImageResId(R.drawable.ic_error_red)
          .setFragManagerStrategyType(AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
          .setDialogViewStrategyType(AlertDialog.DialogViewStrategyType.CONFIRMATION_DIALOG)
          .setDialogFactory(new ConfirmationDialogFactory())
          .setNegativeBtnTextColor(R.color.rating_horrible)
          .build();

      dialog.setCancelable(false);
      dialog.setTargetFragment(mFrag, REQ_CODE_CHECK_INVALID_SUBS_DIALOG);
      dialog.show(mFrag, CHECK_INVALID_SUBS_DIALOG_TAG);
    }

    @Override
    public void attach(@NonNull Fragment object)
    {
      mFrag = object;
      if (Boolean.TRUE.equals(mPendingInvalidCategoriesResult))
      {
        showDialog();
        mPendingInvalidCategoriesResult = null;
      }
    }

    @Override
    public void detach()
    {
      mFrag = null;
    }
  }
}
