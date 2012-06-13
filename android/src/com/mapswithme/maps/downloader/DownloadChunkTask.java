package com.mapswithme.maps.downloader;

import java.io.BufferedInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

import android.os.AsyncTask;
import android.util.Log;

class DownloadChunkTask extends AsyncTask<Void, byte [], Boolean>
{
  private static final String TAG = "DownloadChunkTask";

  private long m_httpCallbackID;
  private String m_url;
  private long m_beg;
  private long m_end;
  private long m_expectedFileSize;
  private String m_postBody;
  private String m_userAgent;

  private final int NOT_SET = -1;
  private final int IO_ERROR = -2;
  private final int INVALID_URL = -3;
  private int m_httpErrorCode = NOT_SET;
  private long m_downloadedBytes = 0;

  native void onWrite(long httpCallbackID, long beg, byte [] data, long size);
  native void onFinish(long httpCallbackID, long httpCode, long beg, long end);

  public DownloadChunkTask(long httpCallbackID, String url, long beg, long end, long expectedFileSize,
                           String postBody, String userAgent)
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

  @Override
  protected void onPostExecute(Boolean success)
  {
    assert(!isCancelled());

    onFinish(m_httpCallbackID, success ? 200 : m_httpErrorCode, m_beg, m_end);
  }

  @Override
  protected void onProgressUpdate(byte []... data)
  {
    if (!isCancelled())
    {
      // Use progress event to save downloaded bytes.
      onWrite(m_httpCallbackID, m_beg + m_downloadedBytes, data[0], data[0].length);
      m_downloadedBytes += data[0].length;
    }
  }

  void start()
  {
    execute();
  }

  @Override
  protected Boolean doInBackground(Void... p)
  {
    HttpURLConnection urlConnection = null;

    try
    {
      URL url = new URL(m_url);
      urlConnection = (HttpURLConnection) url.openConnection();

      if (isCancelled())
        return false;

      urlConnection.setUseCaches(false);
      urlConnection.setConnectTimeout(15 * 1000);
      urlConnection.setReadTimeout(15 * 1000);

      // Set user agent with unique client id
      urlConnection.setRequestProperty("User-Agent", m_userAgent);

      // use Range header only if we don't download whole file from start
      if (!(m_beg == 0 && m_end < 0))
      {
        if (m_end > 0)
          urlConnection.setRequestProperty("Range", String.format("bytes=%d-%d", m_beg, m_end));
        else
          urlConnection.setRequestProperty("Range", String.format("bytes=%d-", m_beg));
      }

      if (m_postBody.length() > 0)
      {
        urlConnection.setDoOutput(true);
        byte[] utf8 = m_postBody.getBytes("UTF-8");
        urlConnection.setFixedLengthStreamingMode(utf8.length);
        final DataOutputStream os = new DataOutputStream(urlConnection.getOutputStream());
        os.write(utf8);
        os.flush();
      }

      if (isCancelled())
        return false;

      final int err = urlConnection.getResponseCode();
      if (err != HttpURLConnection.HTTP_OK && err != HttpURLConnection.HTTP_PARTIAL)
      {
        // we've set error code so client should be notified about the error
        m_httpErrorCode = err;
        return false;
      }

      return downloadFromStream(new BufferedInputStream(urlConnection.getInputStream()));
    }
    catch (MalformedURLException ex)
    {
      Log.d(TAG, "Invalid url: " + m_url);

      // Notify the client about error
      m_httpErrorCode = INVALID_URL;
      return false;
    }
    catch (IOException ex)
    {
      Log.d(TAG, "IOException: " + ex.toString());

      // Notify the client about error
      m_httpErrorCode = IO_ERROR;
      return false;
    }
    finally
    {
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
    int bufferSize = 64;
    while (true)
    {
      try
      {
        // download chunk from stream
        return downloadFromStreamImpl(stream, bufferSize * 1024);
      }
      catch (IOException ex)
      {
        // If exception is thrown, try to reduce buffer size or throw it up, if it's a last try.
        bufferSize /= 32;

        if (bufferSize == 0)
        {
          Log.d(TAG, "IOException in downloadFromStream: " + ex.toString());

          // Notify the client about error
          m_httpErrorCode = IO_ERROR;
          return false;
        }
      }
    }
  }

  private boolean downloadFromStreamImpl(InputStream stream, int bufferSize) throws IOException
  {
    byte[] tempBuf = new byte[bufferSize];

    int readBytes;
    while ((readBytes = stream.read(tempBuf)) != -1)
    {
      if (isCancelled())
        return false;

      byte[] chunk = new byte[readBytes];
      System.arraycopy(tempBuf, 0, chunk, 0, readBytes);

      publishProgress(chunk);
    }

    return true;
  }
}
