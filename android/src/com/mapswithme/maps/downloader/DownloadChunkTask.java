package com.mapswithme.maps.downloader;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

import android.os.AsyncTask;
import android.util.Log;

class DownloadChunkTask extends AsyncTask<Void, byte [], Void>
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
  protected void onPostExecute(Void resCode)
  {
    if (!mIsCancelled)
      onFinish(m_httpCallbackID, 200, m_beg, m_end);
  }

  @Override
  protected void onCancelled()
  {
    // Report error in callback only if we're not forcibly canceled
    if (m_httpErrorCode != NOT_SET)
      onFinish(m_httpCallbackID, m_httpErrorCode, m_beg, m_end);
  }

  @Override
  protected void onProgressUpdate(byte []... data)
  {
    if (!mIsCancelled)
    {
      // Use progress event to save downloaded bytes
      onWrite(m_httpCallbackID, m_beg + m_downloadedBytes, data[0], data[0].length);
      m_downloadedBytes += data[0].length;
    }
  }

  void start()
  {
    execute();
  }

  @Override
  protected Void doInBackground(Void... p)
  {
    URL url;
    try
    {
      url = new URL(m_url);

      HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();

      if (mIsCancelled)
      {
        urlConnection.disconnect();
        return null;
      }

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

      if (mIsCancelled)
      {
        urlConnection.disconnect();
        return null;
      }

      final int err = urlConnection.getResponseCode();
      if (err != HttpURLConnection.HTTP_OK && err != HttpURLConnection.HTTP_PARTIAL)
      {
        // we've set error code so client should be notified about the error
        m_httpErrorCode = err;
        cancel(false);
        urlConnection.disconnect();
        return null;
      }

      final InputStream is = urlConnection.getInputStream();

      byte [] tempBuf = new byte[1024 * 64];
      long readBytes;
      while ((readBytes = is.read(tempBuf)) != -1)
      {
        if (mIsCancelled)
        {
          urlConnection.disconnect();
          return null;
        }

        byte [] chunk = new byte[(int)readBytes];
        System.arraycopy(tempBuf, 0, chunk, 0, (int)readBytes);
        publishProgress(chunk);
      }
    }
    catch (MalformedURLException ex)
    {
      Log.d(TAG, "invalid url: " + m_url);
      // Notify the client about error
      m_httpErrorCode = INVALID_URL;
      cancel(false);
    }
    catch (IOException ex)
    {
      Log.d(TAG, "ioexception: " + ex.toString());
      // Notify the client about error
      m_httpErrorCode = IO_ERROR;
      cancel(false);
    }

    return null;
  }

  private boolean mIsCancelled = false;

  void safeCancel()
  {
    mIsCancelled = true;
    cancel(false);
  }
}
