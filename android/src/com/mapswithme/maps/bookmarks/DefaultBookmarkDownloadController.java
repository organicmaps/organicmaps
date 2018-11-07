package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.text.TextUtils;
import android.widget.Toast;

import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.Authorizer;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.PaymentData;
import com.mapswithme.maps.dialog.ProgressDialogFragment;
import com.mapswithme.maps.purchase.BookmarkPaymentActivity;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.net.MalformedURLException;

class DefaultBookmarkDownloadController implements BookmarkDownloadController,
                                                   BookmarkDownloadHandler, Authorizer.Callback
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = DefaultBookmarkDownloadController.class.getSimpleName();
  @NonNull
  private final BookmarkDownloadReceiver mDownloadCompleteReceiver = new BookmarkDownloadReceiver();
  @NonNull
  private final Authorizer mAuthorizer;
  @Nullable
  private String mDownloadUrl;
  @NonNull
  private final BookmarkManager.BookmarksCatalogListener mCatalogListener;
  @Nullable
  private FragmentActivity mActivity;

  DefaultBookmarkDownloadController(@NonNull Authorizer authorizer,
                                    @NonNull BookmarkManager.BookmarksCatalogListener catalogListener)
  {
    mCatalogListener = catalogListener;
    mAuthorizer = authorizer;
  }

  @Override
  public void downloadBookmark(@NonNull String url) throws MalformedURLException
  {
    downloadBookmarkInternal(getActivityOrThrow(), url);
    mDownloadUrl = url;
  }

  @Override
  public void attach(@NonNull FragmentActivity activity)
  {
    if (mActivity != null)
      throw new AssertionError("Already attached! Call detach.");

    mActivity = activity;
    mAuthorizer.attach(this);
    mDownloadCompleteReceiver.attach(this);
    mDownloadCompleteReceiver.register(getActivityOrThrow().getApplication());
    BookmarkManager.INSTANCE.addCatalogListener(mCatalogListener);
  }

  @Override
  public void detach()
  {
    if (mActivity == null)
      throw new AssertionError("Already detached! Call attach.");

    mAuthorizer.detach();
    mDownloadCompleteReceiver.detach();
    mDownloadCompleteReceiver.unregister(getActivityOrThrow().getApplication());
    BookmarkManager.INSTANCE.removeCatalogListener(mCatalogListener);
    mActivity = null;
  }

  @NonNull
  FragmentActivity getActivityOrThrow()
  {
    if (mActivity == null)
      throw new IllegalStateException("Call this method only when controller is attached!");

    return mActivity;
  }

  private static void downloadBookmarkInternal(@NonNull Context context, @NonNull String url)
      throws MalformedURLException
  {
    BookmarksDownloadManager dm = BookmarksDownloadManager.from(context);
    dm.enqueueRequest(url);
  }

  @NonNull
  private static PaymentDataParser createPaymentDataParser()
  {
    return new BookmarkPaymentDataParser();
  }

  @Override
  public void onAuthorizationRequired()
  {
    mAuthorizer.authorize();
  }

  @Override
  public void onPaymentRequired()
  {
    if (TextUtils.isEmpty(mDownloadUrl))
      throw new IllegalStateException("Download url must be non-null if payment required!");

    PaymentData data = parsePaymentData(mDownloadUrl);
    BookmarkPaymentActivity.start(getActivityOrThrow(), data,
                                  BookmarksCatalogFragment.REQ_CODE_PAY_BOOKMARK);
  }

  @NonNull
  private static PaymentData parsePaymentData(@NonNull String url)
  {
    PaymentDataParser parser = createPaymentDataParser();
    return parser.parse(url);
  }

  @Override
  public void onAuthorizationFinish(boolean success)
  {
    hideProgress();
    if (!success)
    {
      Toast.makeText(getActivityOrThrow(), R.string.profile_authorization_error,
                     Toast.LENGTH_LONG).show();
      return;
    }

    if (TextUtils.isEmpty(mDownloadUrl))
      return;

    try
    {
      downloadBookmarkInternal(getActivityOrThrow(), mDownloadUrl);
    }
    catch (MalformedURLException e)
    {
      LOGGER.e(TAG, "Failed to download bookmark after authorization, url: " + mDownloadUrl, e);
    }
  }

  private void showProgress()
  {
    String message = getActivityOrThrow().getString(R.string.please_wait);
    ProgressDialogFragment dialog = ProgressDialogFragment.newInstance(message, false, true);
    getActivityOrThrow().getSupportFragmentManager()
                        .beginTransaction()
                        .add(dialog, dialog.getClass().getCanonicalName())
                        .commitAllowingStateLoss();
  }


  private void hideProgress()
  {
    FragmentManager fm = getActivityOrThrow().getSupportFragmentManager();
    String tag = ProgressDialogFragment.class.getCanonicalName();
    DialogFragment frag = (DialogFragment) fm.findFragmentByTag(tag);
    if (frag != null)
      frag.dismissAllowingStateLoss();
  }

  @Override
  public void onAuthorizationStart()
  {
    showProgress();
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

  private static class BookmarkPaymentDataParser implements PaymentDataParser
  {
    private final static String SERVER_ID = "id";
    private final static String PRODUCT_ID = "tier";
    private final static String NAME = "name";
    private final static String IMG_URL = "img";
    private final static String AUTHOR_NAME = "author_name";

    @NonNull
    @Override
    public PaymentData parse(@NonNull String url)
    {
      Uri uri = Uri.parse(url);
      String serverId = getQueryRequiredParameter(uri, SERVER_ID);
      String productId = getQueryRequiredParameter(uri, PRODUCT_ID);
      String name = getQueryRequiredParameter(uri, NAME);
      String authorName = getQueryRequiredParameter(uri, AUTHOR_NAME);
      String imgUrl = uri.getQueryParameter(IMG_URL);
      return new PaymentData(serverId, productId, name, imgUrl, authorName);
    }

    @NonNull
    private static String getQueryRequiredParameter(@NonNull Uri uri, @NonNull String name)
    {
      String parameter = uri.getQueryParameter(name);
      if (TextUtils.isEmpty(parameter))
        throw new AssertionError("'" + parameter + "' parameter is required!");

      return parameter;
    }
  }
}
