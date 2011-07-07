package com.mapswithme.maps.downloader;

import java.net.URL;
import java.util.HashSet;
import java.util.Iterator;

import android.util.Log;

import com.mapswithme.maps.downloader.DownloadFilesTask;
import com.mapswithme.maps.downloader.HttpParams;

public class DownloadManager
{
  private static final String TAG = "DownloadManager";
  
  private static HashSet<DownloadFilesTask> m_activeTasks = new HashSet<DownloadFilesTask>(); 
  
  public static void httpGet(URL url, String fileToSave, IDownloadObserver observer)
  {
    Iterator<DownloadFilesTask> iterator = m_activeTasks.iterator();
    while (iterator.hasNext())
    {
      if (iterator.next().m_params.m_url == url)
      {
        Log.w(TAG, "File is already downloading: " + url.toString());
        return;
      }
    }
    final DownloadFilesTask task = new DownloadFilesTask(new HttpParams(url, fileToSave, observer), m_activeTasks);
    m_activeTasks.add(task);
    task.execute();
  }

  public static void cancelDownload(URL url)
  {
    Iterator<DownloadFilesTask> iterator = m_activeTasks.iterator();
    while (iterator.hasNext())
    {
      final DownloadFilesTask task = iterator.next();
      if (task.m_params.m_url == url)
      {
        task.cancel(false);
        iterator.remove();
        break;
      }
    }
  }
  
  public static void cancelAllDownloads()
  {
    Iterator<DownloadFilesTask> iterator = m_activeTasks.iterator();
    while (iterator.hasNext())
      iterator.next().cancel(false);
    m_activeTasks.clear();
  }
}
