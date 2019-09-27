package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.app.Application;
import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import android.widget.Toast;

import androidx.fragment.app.FragmentManager;
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
import com.mapswithme.maps.purchase.PurchaseUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

class BookmarksDownloadFragmentDelegate implements Authorizer.Callback, BookmarkDownloadCallback,
                                                   TargetFragmentCallback
{
  private final static Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private final static String TAG = BookmarksDownloadFragmentDelegate.class.getSimpleName();
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
    Application application = mFragment.requireActivity().getApplication();
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

  void onResume()
  {
    LOGGER.i(TAG, "Check invalid bookmark categories...");
    BookmarkManager.INSTANCE.checkInvalidCategories();
  }

  void onPause()
  {
    // Do nothing.
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
  }

  void onDestroyView()
  {
    BookmarkManager.INSTANCE.removeInvalidCategoriesListener(mInvalidCategoriesListener);
  }

  void onSaveInstanceState(@NonNull Bundle outState)
  {
    mDownloadController.onSave(outState);
  }

  public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data)
  {
    if (resultCode != Activity.RESULT_OK)
      return;

    switch (requestCode)
    {
      case PurchaseUtils.REQ_CODE_PAY_CONTINUE_SUBSCRIPTION:
        BookmarkManager.INSTANCE.resetInvalidCategories();
        break;
      case PurchaseUtils.REQ_CODE_PAY_BOOKMARK:
        mDownloadController.retryDownloadBookmark();
        break;
    }
  }

  private void showAuthorizationProgress()
  {
    String message = mFragment.getString(R.string.please_wait);
    ProgressDialogFragment dialog = ProgressDialogFragment.newInstance(message, false, true);
    mFragment.requireActivity().getSupportFragmentManager()
             .beginTransaction()
             .add(dialog, dialog.getClass().getCanonicalName())
             .commitAllowingStateLoss();
  }

  private void hideAuthorizationProgress()
  {
    FragmentManager fm = mFragment.requireActivity().getSupportFragmentManager();
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
    BookmarkPaymentActivity.startForResult(mFragment, data, PurchaseUtils.REQ_CODE_PAY_BOOKMARK);
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

    InvalidCategoriesListener(@NonNull Fragment fragment)
    {
      mFrag = fragment;
    }

    @Override
    public void onCheckInvalidCategories(boolean hasInvalidCategories)
    {
      LOGGER.i(TAG, "Has invalid categories: " + hasInvalidCategories);
      if (mFrag == null)
      {
        mPendingInvalidCategoriesResult = hasInvalidCategories;
        return;
      }

      if (!hasInvalidCategories)
        return;

      showInvalidBookmarksDialog();
    }

    private void showInvalidBookmarksDialog()
    {
      if (mFrag == null)
        return;

      AlertDialog dialog = new AlertDialog.Builder()
          .setTitleId(R.string.renewal_screen_title)
          .setMessageId(R.string.renewal_screen_message)
          .setPositiveBtnId(R.string.renewal_screen_button_restore)
          .setNegativeBtnId(R.string.renewal_screen_button_cancel)
          .setReqCode(PurchaseUtils.REQ_CODE_CHECK_INVALID_SUBS_DIALOG)
          .setImageResId(R.drawable.ic_error_red)
          .setFragManagerStrategyType(AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
          .setDialogViewStrategyType(AlertDialog.DialogViewStrategyType.CONFIRMATION_DIALOG)
          .setDialogFactory(new ConfirmationDialogFactory())
          .setNegativeBtnTextColor(R.color.rating_horrible)
          .build();

      dialog.setCancelable(false);
      dialog.setTargetFragment(mFrag, PurchaseUtils.REQ_CODE_CHECK_INVALID_SUBS_DIALOG);
      dialog.show(mFrag, PurchaseUtils.DIALOG_TAG_CHECK_INVALID_SUBS);
    }

    @Override
    public void attach(@NonNull Fragment object)
    {
      mFrag = object;
      if (Boolean.TRUE.equals(mPendingInvalidCategoriesResult))
      {
        showInvalidBookmarksDialog();
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
