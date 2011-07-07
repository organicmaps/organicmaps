package com.mapswithme.maps.downloader;

public interface IDownloadObserver
{
  public enum DownloadResult
  {
    EOk,
    EFailed,
    ERedirected
  }
  
  public abstract void OnFinish(final HttpParams params, final DownloadResult result);
  public abstract void OnProgress(final HttpParams params, final Long current, final Long total);
}
