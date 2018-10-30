package com.mapswithme.util;

import android.os.Build;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import android.util.Base64;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLSocketFactory;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class HttpUploader
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.NETWORK);
  private static final String TAG = HttpUploader.class.getSimpleName();
  private static final String LINE_FEED = "\r\n";
  private static final String CHARSET = "UTF-8";
  private static final int BUFFER = 8192;
  private static final int STATUS_CODE_UNKNOWN = -1;
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
  @NonNull
  private final String mBoundary;
  @NonNull
  private final String mEndPart;
  private final boolean mNeedClientAuth;

  public HttpUploader(@NonNull String method, @NonNull String url, @NonNull KeyValue[] params,
                      @NonNull KeyValue[] headers, @NonNull String fileKey, @NonNull String filePath,
                      boolean needClientAuth)
  {
    mMethod = method;
    mUrl = url;
    mFileKey = fileKey;
    mFilePath = filePath;
    mBoundary = "----" + System.currentTimeMillis();
    mParams = new ArrayList<>(Arrays.asList(params));
    mHeaders = new ArrayList<>(Arrays.asList(headers));
    mEndPart = LINE_FEED + "--" + mBoundary + "--" + LINE_FEED;
    mNeedClientAuth = needClientAuth;
  }

  public Result upload()
  {
    int status;
    String message;
    PrintWriter writer = null;
    BufferedReader reader = null;
    HttpURLConnection connection = null;
    try
    {
      URL url = new URL(mUrl);
      connection = (HttpURLConnection) url.openConnection();
      connection.setConnectTimeout(Constants.CONNECTION_TIMEOUT_MS);
      connection.setReadTimeout(Constants.READ_TIMEOUT_MS);
      connection.setUseCaches(false);
      connection.setRequestMethod(mMethod);
      connection.setDoOutput(mMethod.equals("POST"));
      if ("https".equals(connection.getURL().getProtocol()) && mNeedClientAuth)
      {
        HttpsURLConnection httpsConnection = (HttpsURLConnection) connection;
        String cert = HttpUploader.nativeUserBindingCertificate();
        String pwd = HttpUploader.nativeUserBindingPassword();
        byte[] decodedCert = Base64.decode(cert, Base64.DEFAULT);
        SSLSocketFactory socketFactory = ClientCertTLSSocketFactory.create(decodedCert, pwd.toCharArray());
        httpsConnection.setSSLSocketFactory(socketFactory);
      }

      long fileSize = StorageUtils.getFileSize(mFilePath);
      StringBuilder paramsBuilder = new StringBuilder();
      fillBodyParams(paramsBuilder);
      File file = new File(mFilePath);
      fillFileParams(paramsBuilder, mFileKey, file);
      int endPartSize = mEndPart.getBytes().length;
      int paramsSize = paramsBuilder.toString().getBytes().length;
      long bodyLength = paramsSize + fileSize + endPartSize;
      setStreamingMode(connection, bodyLength);
      setHeaders(connection, bodyLength);
      long startTime = System.currentTimeMillis();
      LOGGER.d(TAG, "Start bookmarks upload on url: '" + Utils.makeUrlSafe(mUrl) + "'");
      OutputStream outputStream = connection.getOutputStream();
      writer = new PrintWriter(new OutputStreamWriter(outputStream, CHARSET));
      writeParams(writer, paramsBuilder);
      writeFileContent(outputStream, writer, file);
      writeEndPart(writer);

      status = connection.getResponseCode();
      LOGGER.d(TAG, "Upload bookmarks status code: " + status);
      reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
      message = readResponse(reader);
      long duration = (System.currentTimeMillis() - startTime) / 1000;
      LOGGER.d(TAG, "Upload bookmarks response: '" + message + "', " +
                    "duration = " + duration + " sec, body size = " + bodyLength + " bytes.");
    }
    catch (IOException e)
    {
      status = STATUS_CODE_UNKNOWN;
      message = "I/O exception '" + Utils.makeUrlSafe(mUrl) + "'";
      if (connection != null)
      {
        String errMsg = readErrorResponse(connection);
        if (!TextUtils.isEmpty(errMsg))
          message = errMsg;
      }
      LOGGER.e(TAG, message, e);
    }
    finally
    {
      Utils.closeSafely(writer);
      Utils.closeSafely(reader);
      if (connection != null)
        connection.disconnect();
    }
    return new Result(status, message);
  }

  private static void setStreamingMode(@NonNull HttpURLConnection connection, long bodyLength)
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
    {
      connection.setFixedLengthStreamingMode(bodyLength);
      return;
    }

    if (bodyLength <= Integer.MAX_VALUE)
      connection.setFixedLengthStreamingMode((int) bodyLength);
    else
      connection.setChunkedStreamingMode(BUFFER);
  }

  @NonNull
  private String readResponse(@NonNull BufferedReader reader)
      throws IOException
  {
    StringBuilder response = new StringBuilder();
    String line;
    while ((line = reader.readLine()) != null)
      response.append(line);
    return response.toString();
  }

  @Nullable
  private String readErrorResponse(@NonNull HttpURLConnection connection)
  {
    BufferedReader reader = null;
    try
    {
      InputStream errStream = connection.getErrorStream();
      if (errStream == null)
        return null;

      reader = new BufferedReader(
          new InputStreamReader(connection.getErrorStream()));
      return readResponse(reader);
    }
    catch (IOException e)
    {
      LOGGER.e(TAG, "Failed to read a error stream.");
    }
    finally
    {
      Utils.closeSafely(reader);
    }
    return null;
  }

  private void writeParams(@NonNull PrintWriter writer, @NonNull StringBuilder paramsBuilder)
  {
    writer.append(paramsBuilder);
    writer.flush();
  }

  private void setHeaders(@NonNull URLConnection connection, long bodyLength)
  {
    mHeaders.add(new KeyValue(HttpClient.HEADER_USER_AGENT, Framework.nativeGetUserAgent()));
    mHeaders.add(new KeyValue("App-Version", BuildConfig.VERSION_NAME));
    mHeaders.add(new KeyValue("Content-Type", "multipart/form-data; boundary=" + mBoundary));
    mHeaders.add(new KeyValue("Content-Length", String.valueOf(bodyLength)));
    for (KeyValue header : mHeaders)
      connection.setRequestProperty(header.mKey, header.mValue);
  }

  private void fillBodyParams(@NonNull StringBuilder builder)
  {
    for (KeyValue field : mParams)
      addParam(builder, field.mKey, field.mValue);
  }

  private void addParam(@NonNull StringBuilder builder, @NonNull String key, @NonNull String value)
  {
    builder.append("--").append(mBoundary).append(LINE_FEED);
    builder.append("Content-Disposition: form-data; name=\"")
           .append(key)
           .append("\"")
           .append(LINE_FEED);
    builder.append(LINE_FEED);
    builder.append(value).append(LINE_FEED);
  }

  private void fillFileParams(@NonNull StringBuilder builder, @NonNull String fieldName,
                              @NonNull File uploadFile)
  {
    String fileName = uploadFile.getName();
    builder.append("--").append(mBoundary).append(LINE_FEED);
    builder.append("Content-Disposition: form-data; name=\"")
           .append(fieldName)
           .append("\"; filename=\"")
           .append(fileName)
           .append("\"")
           .append(LINE_FEED);
    builder.append("Content-Type: ").append(URLConnection.guessContentTypeFromName(fileName))
           .append(LINE_FEED);
    builder.append(LINE_FEED);
  }

  private void writeFileContent(@NonNull OutputStream outputStream, @NonNull PrintWriter writer,
                                @NonNull File uploadFile) throws IOException
  {
    FileInputStream inputStream = new FileInputStream(uploadFile);
    int size = Math.min((int) uploadFile.length(), BUFFER);
    byte[] buffer = new byte[size];
    int bytesRead;
    while ((bytesRead = inputStream.read(buffer)) != -1)
      outputStream.write(buffer, 0, bytesRead);
    Utils.closeSafely(inputStream);
  }

  private void writeEndPart(@NonNull PrintWriter writer)
  {
    writer.append(mEndPart);
    writer.flush();
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

  @NonNull
  public static native String nativeUserBindingCertificate();

  @NonNull
  public static native String nativeUserBindingPassword();
}
