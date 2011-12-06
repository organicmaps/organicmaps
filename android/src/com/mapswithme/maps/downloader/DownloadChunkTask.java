package com.mapswithme.maps.downloader;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;

import org.apache.http.util.ByteArrayBuffer;

import android.os.AsyncTask;
import android.util.Log;

// Checks if incomplete file exists and resumes it's download
// Downloads from scratch otherwise
class DownloadChunkTask extends AsyncTask<Void, Long, Void>
{
  private final String TAG = "DownloadFilesTask";
  
  private long m_httpCallbackID;
  private String m_url;
  private long m_beg;
  private long m_end;
  private String m_postBody;
  private long m_requestedSize;
  
  /// results of execution
  
  private List<byte[]> m_chunks;
  private boolean m_hasError;
  private boolean m_isCancelled;
  private long m_response;
  private long m_downloadedSize;
    
  native void onWrite(long httpCallbackID, long beg, byte [] data, long size);
  native void onFinish(long httpCallbackID, long httpCode, long beg, long end);
  
  public DownloadChunkTask(long httpCallbackID, String url, long beg, long end, long size, String postBody)
  {
    Log.d(TAG, String.format("creating new task: %1$d, %2$s, %3$d, %4$d, %5$d, %6$s", httpCallbackID, url, beg, end, size, postBody));
    
    m_httpCallbackID = httpCallbackID;
    m_url = url;
    m_beg = beg;
    m_end = end;
    m_requestedSize = end - beg + 1;
    m_postBody = postBody;
    m_chunks = new ArrayList<byte[]>();
    m_isCancelled = false;
  }
  
  protected void markAsCancelled()
  {
    m_isCancelled = true;    
  }

  @Override
  protected void onPreExecute()
  {
  }

  @Override
  protected void onPostExecute(Void resCode)
  {
    if (m_isCancelled)
    {
      Log.d(TAG, String.format("downloading was cancelled, chunk %1$d, %2$d", m_beg, m_end));
      return;
    }
    
    if (!m_hasError)
    {
      byte [] buf = new byte[(int) m_downloadedSize];
      int offs = 0;

      Iterator<byte[]> it = m_chunks.iterator();
      
      while (it.hasNext())
      {
        byte [] chunk = it.next();
        System.arraycopy(chunk, 0, buf, offs, chunk.length);
        offs += chunk.length;
      }
      
      Log.d(TAG, "merged chunk " + m_downloadedSize + " byte length");
      
      if (m_downloadedSize != m_requestedSize)
        Log.d(TAG, "chunk size mismatch, requested " + m_requestedSize + " bytes, downloaded " + m_downloadedSize + " bytes");
      
      onWrite(m_httpCallbackID, m_beg, buf, m_downloadedSize);
    }

    Log.d(TAG, "finishing download with error code " + m_response);
    onFinish(m_httpCallbackID, m_response, m_beg, m_end);
  }
  
  @Override
  protected void onCancelled()
  {
  }

  @Override
  protected void onProgressUpdate(Long... progress)
  {
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
      
      Log.d(TAG, ("opening connection to " + m_url));
      
      HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
      
      if (isCancelled())
      {
        urlConnection.disconnect();
        return null;
      }
      
      Log.d(TAG, String.format("configuring connection parameter for range %1$d : %2$d", m_beg, m_end));
      
      urlConnection.setDoOutput(true);
      urlConnection.setChunkedStreamingMode(0);
      urlConnection.setRequestProperty("Content-Type", "application/json");
      urlConnection.setUseCaches(false);
      urlConnection.setConnectTimeout(15 * 1000);
      
      if (m_beg != -1)
      {
        Log.d(TAG, ("setting requested range"));

        if (m_end != -1)
          urlConnection.setRequestProperty("Range", String.format("bytes=%1$d-%2$d", m_beg, m_end));
        else
          urlConnection.setRequestProperty("Range", String.format("bytes=%1$d-", m_beg));
      }

      if (!m_postBody.isEmpty())
      {
        Log.d(TAG, ("writing POST body"));
        
        DataOutputStream os = new DataOutputStream(urlConnection.getOutputStream());
        os.writeChars(m_postBody);
      }
      
      if (isCancelled())
      {
        urlConnection.disconnect();
        return null;
      }
      
      Log.d(TAG, ("getting response"));
      
      int response = urlConnection.getResponseCode();
      if (response == HttpURLConnection.HTTP_OK 
       || response == HttpURLConnection.HTTP_PARTIAL)
      {
        InputStream is = urlConnection.getInputStream();
        
        byte [] tempBuf = new byte[1024 * 64];
        
        try
        {
          while (true)
          {
            if (isCancelled())
            {
              urlConnection.disconnect();
              return null;
            }
            
            long readBytes = is.read(tempBuf);
            
            if (readBytes == -1)
            {
              Log.d(TAG, String.format("finished downloading interval %1$d : %2$d with response code %3$d", m_beg, m_end, response));
              m_hasError = false;
              m_response = 200;
              break;
            }
            else
            {
              byte [] chunk = new byte[(int)readBytes];
              System.arraycopy(tempBuf, 0, chunk, 0, (int)readBytes);
              m_chunks.add(chunk);
              m_downloadedSize += chunk.length;
            }
          }
        }
        catch (IOException ex)
        {
          Log.d(TAG, String.format("error occured while downloading range %1$d : %2$d, terminating chunk download", m_beg, m_end));          
          
          m_hasError = true;
          m_response = -1;
        }
      }
      else
      {
        m_hasError = true;
        m_response = response;
        Log.d(TAG, String.format("error downloading interval %1$d : %1$d , response code is %3$d", m_beg, m_end, response));
      }
    }
    catch (MalformedURLException ex)
    {
      Log.d(TAG, "invalid url : " + m_url);
      m_hasError = true;
      m_response = -1;
    }
    catch (IOException ex)
    {
      Log.d(TAG, "ioexception : " + ex.toString());
      m_hasError = true;
      m_response = -1;
    }

    return null;
  }
}
