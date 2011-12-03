package com.mapswithme.maps.downloader;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import android.util.Log;

public class DownloadChunkTask extends Thread
{
  private final String TAG = "DownloadChunkTask";
  
  private long m_httpCallbackID;
  private String m_url;
  private long m_beg;
  private long m_end;
  private long m_size;
  private String m_postBody;
    
  public DownloadChunkTask(long httpCallbackID, String url, long beg, long end, long size, String postBody)
  {
    Log.d(TAG, "creating new task: " + httpCallbackID + url + beg + end + size + postBody);
    
    m_httpCallbackID = httpCallbackID;
    m_url = url;
    m_beg = beg;
    m_end = end;
    m_size = size;
    m_postBody = postBody;
  }
  
  public void debugPause()
  {
    try
    {
      sleep(1000);
    }
    catch (Exception ex)
    {}
  }
  
  @Override
  public void run()
  {
    Log.d(TAG, ("running DownloadChunkTask in separate thread"));
    
    URL url;
    try
    {
      url = new URL(m_url);
      
      Log.d(TAG, ("opening connection to " + m_url));
      
      HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
      
      Log.d(TAG, ("configuring connection parameter"));
      
      urlConnection.setDoOutput(true);
      urlConnection.setChunkedStreamingMode(0);
      urlConnection.setRequestProperty("Content-Type", "application/json");
      urlConnection.setUseCaches(false);
      
      if (m_beg != -1)
      {
        Log.d(TAG, ("setting requested range"));

        if (m_end != -1)
          urlConnection.setRequestProperty("Range", new StringBuilder("bytes=%1-%2").append(m_beg).append(m_end).toString());
        else
          urlConnection.setRequestProperty("Range", new StringBuilder("bytes=%1-").append(m_beg).toString());
      }

      if (!m_postBody.isEmpty())
      {
        Log.d(TAG, ("writing POST body"));
        
        DataOutputStream os = new DataOutputStream(urlConnection.getOutputStream());
        os.writeChars(m_postBody);
      }
      
      Log.d(TAG, ("getting response"));
      
      int response = urlConnection.getResponseCode();
      if (response == HttpURLConnection.HTTP_OK)
      {
        Log.d(TAG, "saving downloaded data");
        
        BufferedReader reader = new BufferedReader(new InputStreamReader(urlConnection.getInputStream()));

        int chunkSize = 1024 * 64;
        int offs = 0;
                
        char [] data = new char[chunkSize];
         
        while (true)
        {
          long readBytes = reader.read(data);

          Log.d(TAG, "got " + readBytes + " bytes of data");
          
          if (readBytes == -1)
          {
            onFinish(m_httpCallbackID, 200, m_beg, m_end);
            break;
          }
          else
          {
            onWrite(m_httpCallbackID, m_beg + offs, data, readBytes);
            offs += readBytes;
          }
        }
      }
      else
      {
        onFinish(m_httpCallbackID, response, m_beg, m_end);
      }
    }
    catch (MalformedURLException ex)
    {
      Log.d(TAG, "invalid url : " + m_url);
    }
    catch (IOException ex)
    {
      Log.d(TAG, "ioexception : " + ex.toString());
      /// report error here
    }
  }
  
  private native void onWrite(long httpCallbackID, long beg, char [] data, long size);
  private native void onFinish(long httpCallbackID, long httpCode, long beg, long end);
}
