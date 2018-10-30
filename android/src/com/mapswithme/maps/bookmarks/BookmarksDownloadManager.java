package com.mapswithme.maps.bookmarks;

import android.app.DownloadManager;
import android.content.Context;
import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Pair;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.HttpClient;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.net.MalformedURLException;
import java.net.URLEncoder;

public class BookmarksDownloadManager
{
  private static final String QUERY_PARAM_ID_KEY = "id";
  private static final String QUERY_PARAM_NAME_KEY = "name";
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);

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

    LOGGER.d("Bookmarks catalog url", "Value = " + dstUri);
    DownloadManager.Request request = new DownloadManager
        .Request(dstUri)
        .setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE)
        .addRequestHeader(HttpClient.HEADER_USER_AGENT, Framework.nativeGetUserAgent())
        .setDestinationInExternalFilesDir(mContext, null, dstUri.getLastPathSegment());

    String accessToken = Framework.nativeGetAccessToken();
    if (!TextUtils.isEmpty(accessToken))
    {
      String headerValue = HttpClient.HEADER_BEARER_PREFFIX + accessToken;
      request.addRequestHeader(HttpClient.HEADER_AUTHORIZATION, headerValue);
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
      builder.appendQueryParameter(each, URLEncoder.encode(queryParameter));
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
