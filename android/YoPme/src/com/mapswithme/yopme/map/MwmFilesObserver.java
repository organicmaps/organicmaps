package com.mapswithme.yopme.map;

import com.mapswithme.yopme.BackscreenActivity;

import android.os.FileObserver;

public class MwmFilesObserver extends FileObserver
{
  private final MwmFilesListener mListener;

  public enum EventType
  {
    MAP_FILE_EVENT,
    KML_FILE_EVENT,
  }

  public interface MwmFilesListener
  {
    void onFileEvent(String path, EventType event);
  }

  private final static int MASK =
      MODIFY | CREATE | DELETE | MOVED_FROM | MOVED_TO;


  public MwmFilesObserver(MwmFilesListener listener)
  {
    super(BackscreenActivity.getDataStoragePath(), MASK);
    mListener = listener;
  }

  @Override
  public void onEvent(int event, String path)
  {
    if (path.endsWith(".mwm"))
      mListener.onFileEvent(path, EventType.MAP_FILE_EVENT);
    else if (path.endsWith(".kml"))
      mListener.onFileEvent(path, EventType.KML_FILE_EVENT);
  }

}
