package app.organicmaps.util;

import android.os.Build;
import android.text.TextUtils;
import android.util.Base64;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.util.log.Logger;

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
import java.util.List;

public final class HttpUploader extends AbstractHttpUploader
{
  private static final String TAG = HttpUploader.class.getSimpleName();

  private static final String LINE_FEED = "\r\n";
  private static final String CHARSET = "UTF-8";
  private static final int BUFFER = 8192;
  private static final int STATUS_CODE_UNKNOWN = -1;

  @NonNull
  private final String mBoundary;
  @NonNull
  private final String mEndPart;

  public HttpUploader(@NonNull HttpPayload payload)
  {
    super(payload);
    mBoundary = "----" + System.currentTimeMillis();
    mEndPart = LINE_FEED + "--" + mBoundary + "--" + LINE_FEED;
  }

  public Result upload()
  {
    int status = STATUS_CODE_UNKNOWN;
    String message;
    PrintWriter writer = null;
    BufferedReader reader = null;
    HttpURLConnection connection = null;
    try
    {
      URL url = new URL(getPayload().getUrl());
      connection = (HttpURLConnection) url.openConnection();
      connection.setConnectTimeout(Constants.CONNECTION_TIMEOUT_MS);
      connection.setReadTimeout(Constants.READ_TIMEOUT_MS);
      connection.setUseCaches(false);
      connection.setRequestMethod(getPayload().getMethod());
      connection.setDoOutput(getPayload().getMethod().equals("POST"));
      if ("https".equals(connection.getURL().getProtocol()) && getPayload().needClientAuth())
      {
        HttpsURLConnection httpsConnection = (HttpsURLConnection) connection;
        String cert = HttpUploader.nativeUserBindingCertificate();
        String pwd = HttpUploader.nativeUserBindingPassword();
        byte[] decodedCert = Base64.decode(cert, Base64.DEFAULT);
        SSLSocketFactory socketFactory = ClientCertTLSSocketFactory.create(decodedCert, pwd.toCharArray());
        httpsConnection.setSSLSocketFactory(socketFactory);
      }

      long fileSize = StorageUtils.getFileSize(getPayload().getFilePath());
      StringBuilder paramsBuilder = new StringBuilder();
      fillBodyParams(paramsBuilder);
      File file = new File(getPayload().getFilePath());
      fillFileParams(paramsBuilder, getPayload().getFileKey(), file);
      int endPartSize = mEndPart.getBytes().length;
      int paramsSize = paramsBuilder.toString().getBytes().length;
      long bodyLength = paramsSize + fileSize + endPartSize;
      setStreamingMode(connection, bodyLength);
      setHeaders(connection, bodyLength);
      long startTime = System.currentTimeMillis();
      Logger.d(TAG, "Start bookmarks upload on url: '" + Utils.makeUrlSafe(getPayload().getUrl()) + "'");
      OutputStream outputStream = connection.getOutputStream();
      writer = new PrintWriter(new OutputStreamWriter(outputStream, CHARSET));
      writeParams(writer, paramsBuilder);
      writeFileContent(outputStream, writer, file);
      writeEndPart(writer);

      status = connection.getResponseCode();
      Logger.d(TAG, "Upload bookmarks status code: " + status);
      reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
      message = readResponse(reader);
      long duration = (System.currentTimeMillis() - startTime) / 1000;
      Logger.d(TAG, "Upload bookmarks response: '" + message + "', " +
               "duration = " + duration + " sec, body size = " + bodyLength + " bytes.");
    }
    catch (IOException e)
    {
      message = "I/O exception '" + Utils.makeUrlSafe(getPayload().getUrl()) + "'";
      if (connection != null)
      {
        String errMsg = readErrorResponse(connection);
        if (!TextUtils.isEmpty(errMsg))
          message = errMsg;
      }
      Logger.e(TAG, message, e);
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
    connection.setFixedLengthStreamingMode(bodyLength);
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
      Logger.e(TAG, "Failed to read a error stream.");
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
    List<KeyValue> headers = getPayload().getHeaders();
    headers.add(new KeyValue("Content-Type", "multipart/form-data; boundary=" + mBoundary));
    headers.add(new KeyValue("Content-Length", String.valueOf(bodyLength)));
    for (KeyValue header : headers)
      connection.setRequestProperty(header.getKey(), header.getValue());
  }

  private void fillBodyParams(@NonNull StringBuilder builder)
  {
    for (KeyValue field : getPayload().getParams())
      addParam(builder, field.getKey(), field.getValue());
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

  static class Result
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

    @Override
    public String toString()
    {
      return "Result{" +
             "mHttpCode=" + mHttpCode +
             ", mDescription='" + mDescription + '\'' +
             '}';
    }
  }

  @NonNull
  public static native String nativeUserBindingCertificate();

  @NonNull
  public static native String nativeUserBindingPassword();
}
