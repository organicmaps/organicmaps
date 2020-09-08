package com.mapswithme.maps.bookmarks;

import android.app.Application;
import android.content.Context;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.text.TextUtils;

import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.PaymentData;
import com.mapswithme.maps.purchase.BookmarkPaymentDataParser;
import com.mapswithme.maps.purchase.PaymentDataParser;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.net.MalformedURLException;

class DefaultBookmarkDownloadController implements BookmarkDownloadController,
                                                   BookmarkDownloadHandler
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = DefaultBookmarkDownloadController.class.getSimpleName();
  private static final String EXTRA_DOWNLOAD_URL = "extra_download_url";
  @NonNull
  private final BookmarkDownloadReceiver mDownloadCompleteReceiver = new BookmarkDownloadReceiver();
  @Nullable
  private String mDownloadUrl;
  @NonNull
  private final BookmarkManager.BookmarksCatalogListener mCatalogListener;
  @Nullable
  private BookmarkDownloadCallback mCallback;
  @NonNull
  private final Application mApplication;

  DefaultBookmarkDownloadController(@NonNull Application application,
                                    @NonNull BookmarkManager.BookmarksCatalogListener catalogListener)
  {
    mApplication = application;
    mCatalogListener = catalogListener;
  }

  @Override
  public boolean downloadBookmark(@NonNull String url)
  {
    try
    {
      downloadBookmarkInternal(mApplication, url);
      mDownloadUrl = url;
      return true;
    }
    catch (MalformedURLException e)
    {
      LOGGER.e(TAG, "Failed to download bookmark, url: " + url);
      return false;
    }
  }

  @Override
  public void retryDownloadBookmark()
  {
    if (TextUtils.isEmpty(mDownloadUrl))
      return;

    try
    {
      downloadBookmarkInternal(mApplication, mDownloadUrl);
    }
    catch (MalformedURLException e)
    {
      LOGGER.e(TAG, "Failed to retry bookmark downloading, url: " + mDownloadUrl);
    }
  }

  @Override
  public void attach(@NonNull BookmarkDownloadCallback callback)
  {
    if (mCallback != null)
      throw new AssertionError("Already attached! Call detach.");

    mCallback = callback;
    mDownloadCompleteReceiver.attach(this);
    mDownloadCompleteReceiver.register(mApplication);
    BookmarkManager.INSTANCE.addCatalogListener(mCatalogListener);
  }

  @Override
  public void detach()
  {
    if (mCallback == null)
      throw new AssertionError("Already detached! Call attach.");

    mDownloadCompleteReceiver.detach();
    mDownloadCompleteReceiver.unregister(mApplication);
    BookmarkManager.INSTANCE.removeCatalogListener(mCatalogListener);
    mCallback = null;
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
    LOGGER.i(TAG, "Authorization required for bookmark purchase");
    if (mCallback != null)
      mCallback.onAuthorizationRequired();
  }

  @Override
  public void onPaymentRequired()
  {
    LOGGER.i(TAG, "Payment required for bookmark purchase");
    if (TextUtils.isEmpty(mDownloadUrl))
      return;

    if (mCallback != null)
    {
      PaymentData data = parsePaymentData(mDownloadUrl);
      mCallback.onPaymentRequired(data);
    }
  }

  @NonNull
  private static PaymentData parsePaymentData(@NonNull String url)
  {
    PaymentDataParser parser = createPaymentDataParser();
    return parser.parse(url);
  }

  @Override
  public void onSave(@NonNull Bundle outState)
  {
    outState.putString(EXTRA_DOWNLOAD_URL, mDownloadUrl);
  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {
    mDownloadUrl = inState.getString(EXTRA_DOWNLOAD_URL);
  }
}
