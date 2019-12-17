package com.mapswithme.util;

import androidx.annotation.NonNull;

public class HttpBackgroundUploader extends AbstractHttpUploader
{

  public HttpBackgroundUploader(@NonNull String method, @NonNull String url,
                                @NonNull KeyValue[] params,
                                @NonNull KeyValue[] headers, @NonNull String fileKey,
                                @NonNull String filePath,
                                boolean needClientAuth)
  {
    super(method, url, params, headers, fileKey, filePath, needClientAuth);
  }

  public void upload()
  {
    /* WorkManager */
  }
}
