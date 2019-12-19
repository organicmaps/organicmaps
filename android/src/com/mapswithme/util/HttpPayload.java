package com.mapswithme.util;

import androidx.annotation.NonNull;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class HttpPayload
{
  @NonNull
  private final String mMethod;
  @NonNull
  private final String mUrl;
  @NonNull
  private final List<KeyValue> mParams;
  @NonNull
  private final List<KeyValue> mHeaders;
  @NonNull
  private final String mFileKey;
  @NonNull
  private final String mFilePath;

  private final boolean mNeedClientAuth;

  public HttpPayload(@NonNull String method, @NonNull String url, @NonNull KeyValue[] params,
      @NonNull KeyValue[] headers, @NonNull String fileKey, @NonNull String filePath,
      boolean needClientAuth)
  {
    mMethod = method;
    mUrl = url;
    mParams = Collections.unmodifiableList(Arrays.asList(params));
    mHeaders = Collections.unmodifiableList(Arrays.asList(headers));
    mFileKey = fileKey;
    mFilePath = filePath;
    mNeedClientAuth = needClientAuth;
  }

  @NonNull
  public String getMethod()
  {
    return mMethod;
  }

  @NonNull
  public String getUrl()
  {
    return mUrl;
  }

  @NonNull
  public List<KeyValue> getParams()
  {
    return mParams;
  }

  @NonNull
  public List<KeyValue> getHeaders()
  {
    return mHeaders;
  }

  @NonNull
  public String getFileKey()
  {
    return mFileKey;
  }

  @NonNull
  public String getFilePath()
  {
    return mFilePath;
  }

  public boolean needClientAuth()
  {
    return mNeedClientAuth;
  }
}
