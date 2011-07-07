package com.mapswithme.maps.downloader;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.util.HashSet;

import android.os.AsyncTask;

// Checks if incomplete file exists and resumes it's download
// Downloads from scratch otherwise
class DownloadFilesTask extends AsyncTask<Void, Long, IDownloadObserver.DownloadResult>
{
  public final HttpParams m_params;
  // Stored here to remove finished tasks 
  private final HashSet<DownloadFilesTask> m_activeTasks;

  DownloadFilesTask(final HttpParams params, HashSet<DownloadFilesTask> activeTasks)
  {
    m_params = params;
    m_activeTasks = activeTasks;
  }

  @Override
  protected void onPreExecute()
  {
  }

  @Override
  protected void onPostExecute(IDownloadObserver.DownloadResult result)
  {
    m_activeTasks.remove(this);
    m_params.m_observer.OnFinish(m_params, result);
  }
  
  @Override
  protected void onCancelled()
  {

  }

  @Override
  protected void onProgressUpdate(Long... progress)
  {
    m_params.m_observer.OnProgress(m_params, progress[0], progress[1]);
  }
  
  @Override
  protected IDownloadObserver.DownloadResult doInBackground(Void... p)
  {
    IDownloadObserver.DownloadResult retCode = IDownloadObserver.DownloadResult.EFailed;
    try
    {
      retCode = downloadUrl(m_params);
    } catch (IOException e)
    {
    }
    return retCode;
  }

  private IDownloadObserver.DownloadResult downloadUrl(HttpParams param) throws IOException
  {
    final File file = new File(new StringBuilder(m_params.m_fileToSave).append(
        ".downloading").toString());
    final FileOutputStream os = new FileOutputStream(file, true);

    HttpURLConnection urlConnection = (HttpURLConnection) param.m_url
        .openConnection();
    final long resumeFileLen = file.length();
    if (resumeFileLen > 0)
      urlConnection.setRequestProperty("Range", new StringBuilder("bytes=%1-")
          .append(resumeFileLen).toString());

    try
    {
      final InputStream is = urlConnection.getInputStream();
      if (!param.m_url.getHost().equals(urlConnection.getURL().getHost()))
      {
        return IDownloadObserver.DownloadResult.ERedirected;
      }
      else
      {
        byte[] buffer = new byte[1024];
        int len;
        final long totalLen = urlConnection.getContentLength();
        long sum = 0;
        while ((len = is.read(buffer)) >= 0 && !isCancelled())
        {
          os.write(buffer, 0, len);
          sum += len;
          publishProgress(sum, totalLen);
        }
      }
    }
    finally
    {
      urlConnection.disconnect();
    }
    return IDownloadObserver.DownloadResult.EOk;
  }
}
