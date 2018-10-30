package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.widget.Toast;

import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.Authorizer;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.net.MalformedURLException;

class DefaultBookmarkCatalogController implements BookmarkCatalogController,
                                                  BookmarkDownloadHandler, Authorizer.Callback
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = DefaultBookmarkCatalogController.class.getSimpleName();
  @NonNull
  private final BookmarkDownloadReceiver mDownloadCompleteReceiver = new BookmarkDownloadReceiver();
  @NonNull
  private final Authorizer mAuthorizer;
  @Nullable
  private String mDownloadUrl;
  @NonNull
  private final BookmarkManager.BookmarksCatalogListener mCatalogListener;
  @Nullable
  private Activity mActivity;

  DefaultBookmarkCatalogController(@NonNull Authorizer authorizer,
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
  public void attach(@NonNull Activity activity)
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

    mActivity = null;
    mAuthorizer.detach();
    mDownloadCompleteReceiver.detach();
    mDownloadCompleteReceiver.unregister(getActivityOrThrow().getApplication());
    BookmarkManager.INSTANCE.removeCatalogListener(mCatalogListener);
  }

  @NonNull
  Activity getActivityOrThrow()
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

  @Override
  public void onAuthorizationRequired()
  {
    mAuthorizer.authorize();
  }

  @Override
  public void onPaymentRequired()
  {
    Toast.makeText(getActivityOrThrow(), "Payment required. Ui coming soon!",
                   Toast.LENGTH_SHORT).show();

  }

  @Override
  public void onAuthorizationFinish(boolean success)
  {
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

  @Override
  public void onAuthorizationStart()
  {
    // Do nothing by default.
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
}
