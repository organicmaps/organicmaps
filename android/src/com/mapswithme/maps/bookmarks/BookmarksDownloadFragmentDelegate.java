package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.app.Application;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.AuthorizationListener;
import com.mapswithme.maps.auth.Authorizer;
import com.mapswithme.maps.auth.TargetFragmentCallback;
import com.mapswithme.maps.base.Detachable;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.PaymentData;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.dialog.ConfirmationDialogFactory;
import com.mapswithme.maps.dialog.ProgressDialogFragment;
import com.mapswithme.maps.purchase.BookmarkPaymentActivity;
import com.mapswithme.maps.purchase.BookmarksAllSubscriptionActivity;
import com.mapswithme.maps.purchase.BookmarksSightsSubscriptionActivity;
import com.mapswithme.maps.purchase.PurchaseUtils;
import com.mapswithme.maps.purchase.SubscriptionType;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.Objects;

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

  @Nullable
  private final AuthorizationListener mAuthorizationListener;
  @NonNull
  private final ExpiredCategoriesListener mExpiredCategoriesListener;
  @NonNull
  private final Bundle mBundle;

  BookmarksDownloadFragmentDelegate(@NonNull Fragment fragment)
  {
    this(fragment, AuthBundleFactory.guideCatalogue(), null);
  }

  BookmarksDownloadFragmentDelegate(@NonNull Fragment fragment,
                                    @NonNull Bundle bundle,
                                    @Nullable AuthorizationListener authorizationListener)
  {
    mFragment = fragment;
    mExpiredCategoriesListener = new ExpiredCategoriesListener(fragment);
    mBundle = bundle;
    mAuthorizationListener = authorizationListener;
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
    mExpiredCategoriesListener.attach(mFragment);
  }

  void onResume()
  {
    LOGGER.i(TAG, "Check invalid bookmark categories...");
    BookmarkManager.INSTANCE.checkExpiredCategories();
  }

  void onPause()
  {
    // Do nothing.
  }

  void onStop()
  {
    mAuthorizer.detach();
    mDownloadController.detach();
    mExpiredCategoriesListener.detach();
  }

  void onCreateView(@Nullable Bundle savedInstanceState)
  {
    BookmarkManager.INSTANCE.addExpiredCategoriesListener(mExpiredCategoriesListener);
  }

  void onDestroyView()
  {
    BookmarkManager.INSTANCE.removeExpiredCategoriesListener(mExpiredCategoriesListener);
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
        BookmarkManager.INSTANCE.resetExpiredCategories();
        break;
      case PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION:
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
      Utils.showSnackbar(mFragment.requireContext(), Objects.requireNonNull(mFragment.getView()),
                         R.string.profile_authorization_error);
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
    authorize(() -> {
      if (mAuthorizationListener != null)
        mAuthorizationListener.onAuthorized(true);
      retryBookmarkDownload();
    });
  }

  @Override
  public void onPaymentRequired(@NonNull PaymentData data)
  {
    if (TextUtils.isEmpty(data.getProductId()))
    {
      SubscriptionType type = SubscriptionType.getTypeByBookmarksGroup(data.getGroup());

      if (type.equals(SubscriptionType.BOOKMARKS_SIGHTS))
      {
        BookmarksSightsSubscriptionActivity.startForResult
            (mFragment, PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION, Statistics.ParamValue.WEBVIEW);
        return;
      }

      BookmarksAllSubscriptionActivity.startForResult
          (mFragment, PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION, Statistics.ParamValue.WEBVIEW);
      return;
    }

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
    mAuthorizer.authorize(mBundle);
  }

  private static class ExpiredCategoriesListener implements BookmarkManager.BookmarksExpiredCategoriesListener, Detachable<Fragment>
  {
    @Nullable
    private Fragment mFrag;
    @Nullable
    private Boolean mPendingExpiredCategoriesResult;

    ExpiredCategoriesListener(@NonNull Fragment fragment)
    {
      mFrag = fragment;
    }

    @Override
    public void onCheckExpiredCategories(boolean hasExpiredCategories)
    {
      LOGGER.i(TAG, "Has invalid categories: " + hasExpiredCategories);
      if (mFrag == null)
      {
        mPendingExpiredCategoriesResult = hasExpiredCategories;
        return;
      }

      if (!hasExpiredCategories)
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
      if (Boolean.TRUE.equals(mPendingExpiredCategoriesResult))
      {
        showInvalidBookmarksDialog();
        mPendingExpiredCategoriesResult = null;
      }
    }

    @Override
    public void detach()
    {
      mFrag = null;
    }
  }
}
