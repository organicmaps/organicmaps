package com.mapswithme.maps.bookmarks;

import android.app.DownloadManager;
import android.content.Context;
import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Pair;

import com.mapswithme.maps.bookmarks.data.BookmarkManager;

import java.io.IOException;
import java.util.Set;

public class BookmarksDownloadManager
{
  private static final String QUERY_PARAM_ID_KEY = "id";
  private static final String QUERY_PARAM_NAME_KEY = "name";

  @NonNull
  private final Context mContext;
  @Nullable
  private final DownloadManager mWrapped;

  private BookmarksDownloadManager(@NonNull Context context)
  {
    mContext = context.getApplicationContext();
    mWrapped = (DownloadManager) context.getSystemService(Context.DOWNLOAD_SERVICE);
  }

  public long enqueueRequest(@NonNull String url) throws IOException
  {
    if (mWrapped == null){
      throw new IOException("system DownloadManager is null");
    }

    Pair<Uri, Uri> uriPair = onPrepareUriPair(url);
    Uri srcUri = uriPair.first;
    Uri dstUri = uriPair.second;

    String title = getTitle(srcUri);
    DownloadManager.Request request = new DownloadManager
        .Request(dstUri)
        .setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE)
        .setTitle(title)
        .setDestinationInExternalFilesDir(
            mContext,
            null,
            dstUri.getLastPathSegment());
    return mWrapped.enqueue(request);
  }

  @NonNull
  private String getTitle(Uri srcUri)
  {
    String title;
    return TextUtils.isEmpty((title = srcUri.getQueryParameter(QUERY_PARAM_NAME_KEY)))
           ? srcUri.getQueryParameter(QUERY_PARAM_ID_KEY)
           : title;
  }

  private Pair<Uri, Uri> onPrepareUriPair(String url)
  {
    Uri srcUri = Uri.parse(url);
    String fileId = srcUri.getQueryParameter(QUERY_PARAM_ID_KEY);
    if (TextUtils.isEmpty(fileId)){
      throw new IllegalArgumentException("query param id not found");
    }
    String downloadUrl = BookmarkManager.INSTANCE.getCatalogDownloadUrl(fileId);
    Uri.Builder builder = Uri.parse(downloadUrl).buildUpon();

    Set<String> paramNames = srcUri.getQueryParameterNames();
    for (String each : paramNames){
      builder.appendQueryParameter(each, srcUri.getQueryParameter(each));
    }
    Uri dstUri = builder.build();
    return new Pair<>(srcUri, dstUri);
  }

  public static BookmarksDownloadManager from(Context context)
  {
    return new BookmarksDownloadManager(context);
  }
}
