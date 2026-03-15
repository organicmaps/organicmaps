/*******************************************************************************
 * The MIT License (MIT)
 * <p/>
 * Copyright (c) 2014 Alexander Borsuk <me@alex.bio> from Minsk, Belarus
 * <p/>
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * <p/>
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * <p/>
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *******************************************************************************/

package app.organicmaps.sdk.util;

import android.text.TextUtils;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.downloader.Android7RootCertificateWorkaround;
import app.organicmaps.sdk.util.log.Logger;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.TimeUnit;
import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.ResponseBody;
import okio.BufferedSink;
import okio.Okio;

// Used by JNI.
@Keep
@SuppressWarnings("unused")
public final class HttpClient
{
  private static final String TAG = HttpClient.class.getSimpleName();

  // These constants must match the C++ HttpClient error codes in http_client.hpp.
  private static final int kNoError = -1;
  private static final int kWriteException = -2;
  private static final int kCancelled = -6;

  private static volatile OkHttpClient sBaseClient;

  private static OkHttpClient getBaseClient()
  {
    if (sBaseClient == null)
    {
      synchronized (HttpClient.class)
      {
        if (sBaseClient == null)
        {
          OkHttpClient.Builder builder = new OkHttpClient.Builder()
                                             .connectTimeout(Constants.READ_TIMEOUT_MS, TimeUnit.MILLISECONDS)
                                             .readTimeout(Constants.READ_TIMEOUT_MS, TimeUnit.MILLISECONDS)
                                             .writeTimeout(Constants.READ_TIMEOUT_MS, TimeUnit.MILLISECONDS)
                                             .followRedirects(true)
                                             .followSslRedirects(true);

          // Apply custom root certificates for Android 7 and below.
          if (android.os.Build.VERSION.SDK_INT <= android.os.Build.VERSION_CODES.N)
          {
            javax.net.ssl.SSLSocketFactory factory = Android7RootCertificateWorkaround.getSslSocketFactory();
            javax.net.ssl.X509TrustManager trustManager = Android7RootCertificateWorkaround.getTrustManager();
            if (factory != null && trustManager != null)
              builder.sslSocketFactory(factory, trustManager);
          }

          sBaseClient = builder.build();
        }
      }
    }
    return sBaseClient;
  }

  // Asynchronous HTTP request using OkHttp's enqueue(). Called from C++ RunHttpRequestAsync.
  // Returns the Call object so C++ can cancel it via JNI.
  public static Call runAsync(@NonNull final Params p, final long nativeCtxPtr)
  {
    Request request;
    try
    {
      request = buildRequest(p);
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to build request for " + Utils.makeUrlSafe(p.url), e);
      nativeOnComplete(nativeCtxPtr, false);
      return null;
    }

    OkHttpClient client = buildClient(p);
    Call call = client.newCall(request);

    call.enqueue(new Callback() {
      @Override
      public void onResponse(@NonNull Call call, @NonNull Response response)
      {
        try
        {
          processResponse(p, response, nativeCtxPtr);
          nativeOnComplete(nativeCtxPtr, true);
        }
        catch (Exception e)
        {
          Logger.e(TAG, "Error processing response for " + Utils.makeUrlSafe(p.url), e);
          nativeOnComplete(nativeCtxPtr, false);
        }
        finally
        {
          response.close();
        }
      }

      @Override
      public void onFailure(@NonNull Call call, @NonNull IOException e)
      {
        if (call.isCanceled())
        {
          p.httpResponseCode = kCancelled;
        }
        else
        {
          Logger.d(TAG, "Request failed for " + Utils.makeUrlSafe(p.url) + ": " + e.getMessage());
          p.httpResponseCode = kNoError;
        }
        nativeOnComplete(nativeCtxPtr, false);
      }
    });

    return call;
  }

  private static OkHttpClient buildClient(@NonNull final Params p)
  {
    return getBaseClient()
        .newBuilder()
        .connectTimeout(p.timeoutMillisec, TimeUnit.MILLISECONDS)
        .readTimeout(p.timeoutMillisec, TimeUnit.MILLISECONDS)
        .writeTimeout(p.timeoutMillisec, TimeUnit.MILLISECONDS)
        .followRedirects(p.followRedirects)
        .followSslRedirects(p.followRedirects)
        .build();
  }

  private static Request buildRequest(@NonNull final Params p)
  {
    if (TextUtils.isEmpty(p.httpMethod))
      throw new IllegalArgumentException("Please set valid HTTP method for request at Params.httpMethod field.");

    Logger.d(TAG, "Connecting to " + Utils.makeUrlSafe(p.url));

    Request.Builder requestBuilder = new Request.Builder().url(p.url);

    if (!TextUtils.isEmpty(p.cookies))
      requestBuilder.header("Cookie", p.cookies);

    for (KeyValue header : p.headers)
      requestBuilder.header(header.getKey(), header.getValue());

    RequestBody body = null;
    String contentType = getHeaderValue(p.headers, "Content-Type");
    MediaType mediaType = contentType != null ? MediaType.parse(contentType) : null;
    if (p.requestData != null)
    {
      body = RequestBody.create(p.requestData, mediaType);
      Logger.d(TAG, "Sending " + p.httpMethod + " with content of size " + p.requestData.length);
    }
    else if (!TextUtils.isEmpty(p.inputFilePath))
    {
      File file = new File(p.inputFilePath);
      body = RequestBody.create(file, mediaType);
      Logger.d(TAG, "Sending " + p.httpMethod + " with file of size " + file.length());
    }

    // OkHttp requires a non-null RequestBody for PUT/POST/PATCH (even if empty).
    if (body == null && requiresRequestBody(p.httpMethod))
      body = RequestBody.create(new byte[0], null);

    requestBuilder.method(p.httpMethod, body);
    return requestBuilder.build();
  }

  private static void processResponse(@NonNull final Params p, @NonNull Response response, final long nativeCtxPtr)
      throws IOException
  {
    p.httpResponseCode = response.code();
    Logger.d(TAG, "Received HTTP " + p.httpResponseCode + " from server, for request = " + Utils.makeUrlSafe(p.url));

    if (p.httpResponseCode >= 300 && p.httpResponseCode < 400)
      p.receivedUrl = response.header("Location", p.url);
    else
      p.receivedUrl = response.request().url().toString();

    p.headers.clear();
    if (p.loadHeaders)
    {
      for (String name : response.headers().names())
      {
        // Use headers(name) to collect all values for multi-valued headers
        // (e.g., Set-Cookie), then join them with ", ".
        List<String> values = response.headers(name);
        p.headers.add(new KeyValue(StringUtils.toLowerCase(name), TextUtils.join(", ", values)));
      }
    }
    else
    {
      List<String> cookies = response.headers("Set-Cookie");
      if (!cookies.isEmpty())
        p.headers.add(new KeyValue("set-cookie", TextUtils.join(", ", cookies)));
    }

    ResponseBody responseBody = response.body();
    if (responseBody == null)
    {
      p.responseData = null;
      return;
    }

    final boolean hasDataHandler = hasNativeDataHandler(nativeCtxPtr);
    final boolean hasProgressHandler = hasNativeProgressHandler(nativeCtxPtr);
    final boolean useStreaming = hasDataHandler || hasProgressHandler;

    if (!TextUtils.isEmpty(p.outputFilePath))
    {
      try (BufferedSink sink = Okio.buffer(Okio.sink(new File(p.outputFilePath))))
      {
        if (useStreaming)
          readBodyStreaming(responseBody, nativeCtxPtr, sink, hasDataHandler, hasProgressHandler);
        else
          sink.writeAll(responseBody.source());
      }
      catch (IOException e)
      {
        Logger.e(TAG, "File write error for " + p.outputFilePath, e);
        p.httpResponseCode = kWriteException;
        throw e;
      }
    }
    else if (useStreaming)
    {
      byte[] accumulated = readBodyStreaming(responseBody, nativeCtxPtr, null, hasDataHandler, hasProgressHandler);
      if (accumulated != null)
        p.responseData = accumulated;
    }
    else
    {
      p.responseData = responseBody.bytes();
    }
  }

  @Nullable
  private static byte[] readBodyStreaming(@NonNull ResponseBody body, long nativeCtxPtr,
                                          @Nullable BufferedSink outputSink, boolean hasDataHandler,
                                          boolean hasProgressHandler) throws IOException
  {
    final long contentLength = body.contentLength();
    // Accumulate body in memory when no DataHandler consumes it and no file sink exists.
    final boolean accumulate = !hasDataHandler && outputSink == null;
    ByteArrayOutputStream baos = accumulate ? new ByteArrayOutputStream() : null;
    byte[] buffer = new byte[8192];
    long totalRead = 0;

    try (okio.BufferedSource source = body.source())
    {
      int bytesRead;
      while ((bytesRead = source.read(buffer)) != -1)
      {
        totalRead += bytesRead;

        if (hasDataHandler)
        {
          if (!nativeOnData(nativeCtxPtr, buffer, bytesRead))
            break;
        }

        if (outputSink != null)
          outputSink.write(buffer, 0, bytesRead);

        if (accumulate)
          baos.write(buffer, 0, bytesRead);

        if (hasProgressHandler)
          nativeOnProgress(nativeCtxPtr, totalRead, contentLength);
      }
    }
    return accumulate ? baos.toByteArray() : null;
  }

  private static boolean requiresRequestBody(@NonNull String method)
  {
    return "POST".equals(method) || "PUT".equals(method) || "PATCH".equals(method);
  }

  private static String getHeaderValue(@NonNull List<KeyValue> headers, @NonNull String name)
  {
    for (KeyValue kv : headers)
    {
      if (name.equalsIgnoreCase(kv.getKey()))
        return kv.getValue();
    }
    return null;
  }

  // Called from OkHttp's callback thread to deliver results to C++.
  private static native void nativeOnComplete(long nativeCtxPtr, boolean success);
  // Streaming callbacks — called during response body reading.
  private static native boolean nativeOnData(long nativeCtxPtr, byte[] buffer, int size);
  private static native void nativeOnProgress(long nativeCtxPtr, long bytesTransferred, long totalBytes);
  private static native boolean hasNativeDataHandler(long nativeCtxPtr);
  private static native boolean hasNativeProgressHandler(long nativeCtxPtr);

  // Used by JNI.
  @Keep
  @SuppressWarnings("unused")
  private static class Params
  {
    public void setHeaders(@NonNull KeyValue[] array)
    {
      headers = new ArrayList<>(Arrays.asList(array));
    }

    public Object[] getHeaders()
    {
      return headers.toArray();
    }

    public String url;
    String receivedUrl;
    String httpMethod;
    public byte[] requestData;
    public byte[] responseData;
    String inputFilePath;
    String outputFilePath;
    String cookies;
    ArrayList<KeyValue> headers = new ArrayList<>();
    int httpResponseCode = -1;
    boolean followRedirects = true;
    boolean loadHeaders;
    int timeoutMillisec = Constants.READ_TIMEOUT_MS;

    public Params(String url)
    {
      this.url = url;
      httpMethod = "GET";
    }
  }
}
