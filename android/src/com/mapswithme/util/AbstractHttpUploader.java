package com.mapswithme.util;

import androidx.annotation.NonNull;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public abstract class AbstractHttpUploader
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

  AbstractHttpUploader(@NonNull String method, @NonNull String url,
                       @NonNull KeyValue[] params,
                       @NonNull KeyValue[] headers, @NonNull String fileKey,
                       @NonNull String filePath,
                       boolean needClientAuth)
  {
    mMethod = method;
    mUrl = url;
    mFileKey = fileKey;
    mFilePath = filePath;
    mParams = new ArrayList<>(Arrays.asList(params));
    mHeaders = new ArrayList<>(Arrays.asList(headers));
    mNeedClientAuth = needClientAuth;
  }

  @NonNull
  protected String getMethod() {
    return mMethod;
  }

  @NonNull
  protected String getUrl() {
    return mUrl;
  }

  @NonNull
  protected List<KeyValue> getParams() {
    return mParams;
  }

  @NonNull
  protected List<KeyValue> getHeaders() {
    return mHeaders;
  }

  @NonNull
  protected String getFileKey() {
    return mFileKey;
  }

  @NonNull
  protected String getFilePath() {
    return mFilePath;
  }

  protected boolean needClientAuth() {
    return mNeedClientAuth;
  }

}
