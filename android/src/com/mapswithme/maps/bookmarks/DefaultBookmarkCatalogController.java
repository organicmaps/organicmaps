package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.widget.Toast;

import com.mapswithme.maps.bookmarks.data.BookmarkManager;

import java.net.MalformedURLException;

public class DefaultBookmarkCatalogController
    implements BookmarkCatalogController, BookmarkDownloadHandler
{
  @Nullable
  private Activity mActivity;
  @NonNull
  private final BookmarkDownloadReceiver mDownloadCompleteReceiver = new BookmarkDownloadReceiver();
  @NonNull
  private final BookmarkManager.BookmarksCatalogListener mCatalogListener;

  DefaultBookmarkCatalogController(@NonNull BookmarkManager.BookmarksCatalogListener
                                       catalogListener)
  {
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
    mDownloadCompleteReceiver.attach(this);
    mDownloadCompleteReceiver.register(mActivity.getApplication());
    BookmarkManager.INSTANCE.addCatalogListener(mCatalogListener);
  }

  @Override
  public void detach()
  {
    if (mActivity == null)
      throw new AssertionError("Already detached! Call attach.");

    mDownloadCompleteReceiver.detach();
    mDownloadCompleteReceiver.unregister(mActivity.getApplication());
    BookmarkManager.INSTANCE.removeCatalogListener(mCatalogListener);
    mActivity = null;
  }

  @Override
  public void onAuthorizationRequired()
  {
    if (mActivity == null)
      return;

    Toast.makeText(mActivity, "Authorization required. Ui coming soon!",
                   Toast.LENGTH_SHORT).show();

  }

  @Override
  public void onPaymentRequired()
  {
    if (mActivity == null)
      return;

    Toast.makeText(mActivity, "Payment required. Ui coming soon!",
                   Toast.LENGTH_SHORT).show();

  }
}
