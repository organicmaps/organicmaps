package com.mapswithme.util;

import android.support.annotation.NonNull;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

final class HttpUploader
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

  @SuppressWarnings("unused")
  private HttpUploader(@NonNull String method, @NonNull String url, @NonNull KeyValue[] params,
                       @NonNull KeyValue[] headers, @NonNull String fileKey, @NonNull String filePath)
  {
    mMethod = method;
    mUrl = url;
    mParams = new ArrayList<>(Arrays.asList(params));
    mHeaders = new ArrayList<>(Arrays.asList(headers));
    mFileKey = fileKey;
    mFilePath = filePath;
  }

  @SuppressWarnings("unused")
  private Result upload()
  {
    // Dummy. Error code 200 - Http OK.
    return new Result(200, "");
  }

  private static class Result
  {
    private final int mHttpCode;
    @NonNull
    private final String mDescription;

    Result(int httpCode, @NonNull String description)
    {
      mHttpCode = httpCode;
      mDescription = description;
    }

    public int getHttpCode()
    {
      return mHttpCode;
    }

    @NonNull
    public String getDescription()
    {
      return mDescription;
    }
  }
}
