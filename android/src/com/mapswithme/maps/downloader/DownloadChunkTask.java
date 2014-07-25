package com.mapswithme.maps.downloader;

import android.annotation.SuppressLint;
import android.os.AsyncTask;
import android.util.Log;

import com.mapswithme.util.StringUtils;
import com.mapswithme.util.Utils;

import java.io.BufferedInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Map;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

class DownloadChunkTask extends AsyncTask<Void, byte[], Boolean>
{
  private static final String TAG = "DownloadChunkTask";

  private static final int TIMEOUT_IN_SECONDS = 60;

  private final long m_httpCallbackID;
  private final String m_url;
  private final long m_beg;
  private final long m_end;
  private final long m_expectedFileSize;
  private byte[] m_postBody;
  private final String m_userAgent;

  private final int NOT_SET = -1;
  private final int IO_ERROR = -2;
  private final int INVALID_URL = -3;
  private final int WRITE_ERROR = -4;
  private final int FILE_SIZE_CHECK_FAILED = -5;

  private int m_httpErrorCode = NOT_SET;
  private long m_downloadedBytes = 0;

  private static Executor s_exec = Executors.newFixedThreadPool(4);

  native boolean onWrite(long httpCallbackID, long beg, byte[] data, long size);

  native void onFinish(long httpCallbackID, long httpCode, long beg, long end);

  public DownloadChunkTask(long httpCallbackID, String url, long beg, long end,
                           long expectedFileSize, byte[] postBody, String userAgent)
  {
    m_httpCallbackID = httpCallbackID;
    m_url = url;
    m_beg = beg;
    m_end = end;
    m_expectedFileSize = expectedFileSize;
    m_postBody = postBody;
    m_userAgent = userAgent;
  }

  @Override
  protected void onPreExecute()
  {
  }

  private long getChunkID() { return m_beg; }

  @Override
  protected void onPostExecute(Boolean success)
  {
    //Log.i(TAG, "Writing chunk " + getChunkID());

    // It seems like onPostExecute can be called (from GUI thread queue)
    // after the task was cancelled in destructor of HttpThread.
    // Reproduced by Samsung testers: touch Try Again for many times from
    // start activity when no connection is present.

    if (!isCancelled())
      onFinish(m_httpCallbackID, success ? 200 : m_httpErrorCode, m_beg, m_end);
  }

  @Override
  protected void onProgressUpdate(byte[]... data)
  {
    if (!isCancelled())
    {
      // Use progress event to save downloaded bytes.
      if (onWrite(m_httpCallbackID, m_beg + m_downloadedBytes, data[0], data[0].length))
        m_downloadedBytes += data[0].length;
      else
      {
        // Cancel downloading and notify about error.
        cancel(false);
        onFinish(m_httpCallbackID, WRITE_ERROR, m_beg, m_end);
      }
    }
  }

  @SuppressLint("NewApi")
  void start()
  {
    if (Utils.apiEqualOrGreaterThan(11))
      executeOnExecutor(s_exec, (Void[]) null);
    else
      execute((Void[]) null);
  }

  static long parseContentRange(String contentRangeValue)
  {
    if (contentRangeValue != null)
    {
      final int slashIndex = contentRangeValue.lastIndexOf('/');
      if (slashIndex >= 0)
      {
        try
        {
          return Long.parseLong(contentRangeValue.substring(slashIndex + 1));
        } catch (final NumberFormatException ex)
        {
          // Return -1 at the end of function
        }
      }
    }
    return -1;
  }

  @Override
  protected Boolean doInBackground(Void... p)
  {
    //Log.i(TAG, "Start downloading chunk " + getChunkID());

    HttpURLConnection urlConnection = null;

    try
    {
      final URL url = new URL(m_url);
      urlConnection = (HttpURLConnection) url.openConnection();

      if (isCancelled())
        return false;

      urlConnection.setUseCaches(false);
      urlConnection.setConnectTimeout(TIMEOUT_IN_SECONDS * 1000);
      urlConnection.setReadTimeout(TIMEOUT_IN_SECONDS * 1000);

      // Set user agent with unique client id
      urlConnection.setRequestProperty("User-Agent", m_userAgent);

      // use Range header only if we don't download whole file from start
      if (!(m_beg == 0 && m_end < 0))
      {
        if (m_end > 0)
          urlConnection.setRequestProperty("Range", StringUtils.formatUsingUsLocale("bytes=%d-%d", m_beg, m_end));
        else
          urlConnection.setRequestProperty("Range", StringUtils.formatUsingUsLocale("bytes=%d-", m_beg));
      }

      final Map<?, ?> requestParams = urlConnection.getRequestProperties();

      if (m_postBody != null)
      {
        urlConnection.setDoOutput(true);
        urlConnection.setFixedLengthStreamingMode(m_postBody.length);

        final DataOutputStream os = new DataOutputStream(urlConnection.getOutputStream());
        os.write(m_postBody);
        os.flush();
        m_postBody = null;
        Utils.closeStream(os);
      }

      if (isCancelled())
        return false;

      final int err = urlConnection.getResponseCode();
      // @TODO We can handle redirect (301, 302 and 307) here and display redirected page to user,
      // to avoid situation when downloading is always failed by "unknown" reason
      // When we didn't ask for chunks, code should be 200
      // When we asked for a chunk, code should be 206
      final boolean isChunk = !(m_beg == 0 && m_end < 0);
      if ((isChunk && err != HttpURLConnection.HTTP_PARTIAL) || (!isChunk && err != HttpURLConnection.HTTP_OK))
      {
        // we've set error code so client should be notified about the error
        m_httpErrorCode = FILE_SIZE_CHECK_FAILED;
        Log.w(TAG, "Error for " + urlConnection.getURL() +
            ": Server replied with code " + err +
            ", aborting download. " + Utils.mapPrettyPrint(requestParams));
        return false;
      }

      // Check for content size - are we downloading requested file or some router's garbage?
      if (m_expectedFileSize > 0)
      {
        long contentLength = parseContentRange(urlConnection.getHeaderField("Content-Range"));
        if (contentLength < 0)
          contentLength = urlConnection.getContentLength();

        // Check even if contentLength is invalid (-1), in this case it's not our server!
        if (contentLength != m_expectedFileSize)
        {
          // we've set error code so client should be notified about the error
          m_httpErrorCode = FILE_SIZE_CHECK_FAILED;
          Log.w(TAG, "Error for " + urlConnection.getURL() +
              ": Invalid file size received (" + contentLength + ") while expecting " + m_expectedFileSize +
              ". Aborting download.");
          return false;
        }
        // @TODO Else display received web page to user - router is redirecting us to some page
      }

      return downloadFromStream(new BufferedInputStream(urlConnection.getInputStream(), 65536));
    } catch (final MalformedURLException ex)
    {
      Log.d(TAG, "Invalid url: " + m_url);

      // Notify the client about error
      m_httpErrorCode = INVALID_URL;
      return false;
    } catch (final IOException ex)
    {
      Log.d(TAG, "IOException in doInBackground for URL: " + m_url, ex);

      // Notify the client about error
      m_httpErrorCode = IO_ERROR;
      return false;
    } finally
    {
      //Log.i(TAG, "End downloading chunk " + getChunkID());

      if (urlConnection != null)
        urlConnection.disconnect();
      else
      {
        m_httpErrorCode = IO_ERROR;
        return false;
      }
    }
  }

  /// Because of timeouts in InpetStream.read (for bad connection),
  /// try to introduce dynamic buffer size to read in one query.
  private boolean downloadFromStream(InputStream stream)
  {
    final int arrSize[] = {64, 32, 1};
    int ret = -1;

    for (int i = 0; i < arrSize.length; ++i)
    {
      try
      {
        // download chunk from stream
        ret = downloadFromStreamImpl(stream, arrSize[i] * 1024);
        break;
      } catch (final IOException ex)
      {
        Log.d(TAG, "IOException in downloadFromStream for chunk size: " + arrSize[i], ex);
      }
    }

    if (ret < 0)
    {
      // notify the client about error
      m_httpErrorCode = IO_ERROR;
    }

    Utils.closeStream(stream);

    return (ret == 0);
  }

  /// @return
  /// 0 - download successful;
  /// 1 - download canceled;
  /// -1 - some error occurred;
  private int downloadFromStreamImpl(InputStream stream, int bufferSize) throws IOException
  {
    final byte[] tempBuf = new byte[bufferSize];

    int readBytes;
    while ((readBytes = stream.read(tempBuf)) > 0)
    {
      if (isCancelled())
        return 1;

      final byte[] chunk = new byte[readBytes];
      System.arraycopy(tempBuf, 0, chunk, 0, readBytes);

      publishProgress(chunk);
    }

    // -1 - means the end of the stream (success), else - some error occurred
    return (readBytes == -1 ? 0 : -1);
  }
}
