package app.organicmaps.util;

import androidx.annotation.NonNull;

import com.google.gson.annotations.SerializedName;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class HttpPayload
{
  @NonNull
  @SerializedName("method")
  private final String mMethod;
  @NonNull
  @SerializedName("url")
  private final String mUrl;
  @NonNull
  @SerializedName("params")
  private final List<KeyValue> mParams;
  @NonNull
  @SerializedName("header")
  private final List<KeyValue> mHeaders;
  @NonNull
  @SerializedName("fileKey")
  private final String mFileKey;
  @NonNull
  @SerializedName("filePath")
  private final String mFilePath;
  @SerializedName("needClientAuth")
  private final boolean mNeedClientAuth;

  public HttpPayload(@NonNull String method, @NonNull String url, @NonNull KeyValue[] params,
                     @NonNull KeyValue[] headers, @NonNull String fileKey, @NonNull String filePath,
                     boolean needClientAuth)
  {
    mMethod = method;
    mUrl = url;
    mParams = new ArrayList<>(Arrays.asList(params));
    mHeaders = new ArrayList<>(Arrays.asList(headers));
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

  @Override
  public String toString()
  {
    return "HttpPayload{" +
           "mMethod='" + mMethod + '\'' +
           ", mUrl='" + mUrl + '\'' +
           ", mParams=" + mParams +
           ", mHeaders=" + mHeaders +
           ", mFileKey='" + mFileKey + '\'' +
           ", mFilePath='" + mFilePath + '\'' +
           ", mNeedClientAuth=" + mNeedClientAuth +
           '}';
  }
}
