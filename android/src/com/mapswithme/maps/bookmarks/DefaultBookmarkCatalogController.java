package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.widget.Toast;

import com.mapswithme.maps.auth.Authorizer;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

import java.net.MalformedURLException;

public class DefaultBookmarkCatalogController
    implements BookmarkCatalogController, BookmarkDownloadHandler, Authorizer.Callback
{
  @Nullable
  private Activity mActivity;
  @NonNull
  private final BookmarkDownloadReceiver mDownloadCompleteReceiver = new BookmarkDownloadReceiver();
  @NonNull
  private final BookmarkManager.BookmarksCatalogListener mCatalogListener;
  @NonNull
  private final Authorizer mAuthorizer;

  DefaultBookmarkCatalogController(@NonNull Authorizer authorizer,
                                   @NonNull BookmarkManager.BookmarksCatalogListener catalogListener)
  {
    mAuthorizer = authorizer;
    mCatalogListener = catalogListener;
  }

  @Override
  public void downloadBookmark(@NonNull String url) throws MalformedURLException
  {
    if (mActivity == null)
      return;

    BookmarksDownloadManager dm = BookmarksDownloadManager.from(mActivity);
    dm.enqueueRequest(url);
  }

  @Override
  public void attach(@NonNull Activity activity)
  {
    if (mActivity != null)
      throw new AssertionError("Already attached! Call detach.");

    mActivity = activity;
    mAuthorizer.attach(this);
    mDownloadCompleteReceiver.attach(this);
    mDownloadCompleteReceiver.register(mActivity.getApplication());
    BookmarkManager.INSTANCE.addCatalogListener(mCatalogListener);
  }

  @Override
  public void detach()
  {
    if (mActivity == null)
      throw new AssertionError("Already detached! Call attach.");

    mAuthorizer.detach();
    mDownloadCompleteReceiver.detach();
    mDownloadCompleteReceiver.unregister(mActivity.getApplication());
    BookmarkManager.INSTANCE.removeCatalogListener(mCatalogListener);
    mActivity = null;
  }

  @Override
  public void onAuthorizationRequired()
  {
    mAuthorizer.authorize();
  }

  @Override
  public void onPaymentRequired()
  {
    if (mActivity == null)
      return;

    Toast.makeText(mActivity, "Payment required. Ui coming soon!",
                   Toast.LENGTH_SHORT).show();

  }

  @Override
  public void onAuthorizationFinish(boolean success)
  {
    Toast.makeText(mActivity, "Authorization completed, success = " + success,
                   Toast.LENGTH_SHORT).show();
    // TODO: repeat previous download.
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
