package com.mapswithme.maps.bookmarks;

import android.app.DownloadManager;
import android.content.Context;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Pair;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.HttpClient;
import com.mapswithme.util.KeyValue;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URLEncoder;

public class BookmarksDownloadManager
{
  private static final String QUERY_PARAM_ID_KEY = "id";
  private static final String QUERY_PARAM_NAME_KEY = "name";
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = BookmarksDownloadManager.class.getSimpleName();

  @NonNull
  private final Context mContext;

  private BookmarksDownloadManager(@NonNull Context context)
  {
    mContext = context.getApplicationContext();
  }

  @SuppressWarnings("UnusedReturnValue")
  public long enqueueRequest(@NonNull String url) throws MalformedURLException
  {
    Pair<Uri, Uri> uriPair = prepareUriPair(url);

    DownloadManager downloadManager =
        (DownloadManager) mContext.getSystemService(Context.DOWNLOAD_SERVICE);

    if (downloadManager == null)
    {
      throw new NullPointerException(
          "Download manager is null, failed to download url = " + url);
    }

    Uri srcUri = uriPair.first;
    Uri dstUri = uriPair.second;

    LOGGER.d(TAG, "Bookmarks catalog url = " + dstUri);
    DownloadManager.Request request = new DownloadManager
        .Request(dstUri)
        .setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE)
        .setDestinationInExternalFilesDir(mContext, null, dstUri.getLastPathSegment());

    for (KeyValue header: BookmarkManager.INSTANCE.getCatalogHeaders())
      request.addRequestHeader(header.getKey(), header.getValue());

    String accessToken = Framework.nativeGetAccessToken();
    if (!TextUtils.isEmpty(accessToken))
    {
      LOGGER.d(TAG, "User authorized");
      String headerValue = HttpClient.HEADER_BEARER_PREFFIX + accessToken;
      request.addRequestHeader(HttpClient.HEADER_AUTHORIZATION, headerValue);
    }
    else
    {
      LOGGER.d(TAG, "User not authorized");
    }

    String title = makeTitle(srcUri);
    if (!TextUtils.isEmpty(title))
      request.setTitle(title);

    return downloadManager.enqueue(request);
  }

  @Nullable
  private static String makeTitle(@NonNull Uri srcUri)
  {
    String title = srcUri.getQueryParameter(QUERY_PARAM_NAME_KEY);
    return TextUtils.isEmpty(title) ? srcUri.getQueryParameter(QUERY_PARAM_ID_KEY) : title;
  }

  @NonNull
  private static Pair<Uri, Uri> prepareUriPair(@NonNull String url) throws MalformedURLException
  {
    Uri srcUri = Uri.parse(url);
    String fileId = srcUri.getQueryParameter(QUERY_PARAM_ID_KEY);
    if (TextUtils.isEmpty(fileId))
      throw new MalformedURLException("File id not found");

    String downloadUrl = BookmarkManager.INSTANCE.getCatalogDownloadUrl(fileId);
    Uri.Builder builder = Uri.parse(downloadUrl).buildUpon();

    for (String each : srcUri.getQueryParameterNames())
    {
      String queryParameter = srcUri.getQueryParameter(each);
      try
      {
        queryParameter = URLEncoder.encode(queryParameter, "UTF-8");
      }
      catch (UnsupportedEncodingException e)
      {
        queryParameter = srcUri.getQueryParameter(each);
      }

      builder.appendQueryParameter(each, queryParameter);
    }
    Uri dstUri = builder.build();
    return new Pair<>(srcUri, dstUri);
  }

  @NonNull
  public static BookmarksDownloadManager from(@NonNull Context context)
  {
    return new BookmarksDownloadManager(context);
  }
}
