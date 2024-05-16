package app.organicmaps.location;

import app.organicmaps.util.Listeners;

public class TrackRecorder
{
  private final Listeners<TrackRecordingListener> mListeners = new Listeners<>();

  private static final TrackRecorder INSTANCE = new TrackRecorder();

  private TrackRecorder(){}
  public static TrackRecorder getInstance()
  {
    return INSTANCE;
  }
  public interface TrackRecordingListener
  {
    void onTrackRecordingStarted();
    void onTrackRecordingStopped();
  }
  public void startTrackRecording()
  {
    if(!nativeIsEnabled()) nativeSetEnabled(true);
    for (TrackRecordingListener listener : mListeners)
    {
      listener.onTrackRecordingStarted();
    }
    mListeners.finishIterate();
  }
  public void stopTrackRecording()
  {
    if(nativeIsEnabled()) nativeSetEnabled(false);
    for (TrackRecordingListener listener : mListeners)
    {
      listener.onTrackRecordingStopped();
    }
    mListeners.finishIterate();
  }
  public void addListener(TrackRecordingListener listener)
  {
    mListeners.register(listener);
  }
  public void removeListener(TrackRecordingListener listener)
  {
    mListeners.unregister(listener);
  }
  public static native void nativeSetEnabled(boolean enable);
  public static native boolean nativeIsEnabled();
  public static native void nativeSetDuration(int hours);
  public static native int nativeGetDuration();
}
