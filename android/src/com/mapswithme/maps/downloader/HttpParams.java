package com.mapswithme.maps.downloader;

import java.net.URL;

public class HttpParams
{
  public final URL m_url;
  public final String m_fileToSave;
  public final IDownloadObserver m_observer;
  
  public HttpParams(URL url, String fileToSave, IDownloadObserver observer)
  {
    m_url = url;
    m_fileToSave = fileToSave;
    m_observer = observer;
  }
}
