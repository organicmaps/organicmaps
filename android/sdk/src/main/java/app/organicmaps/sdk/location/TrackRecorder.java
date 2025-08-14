package app.organicmaps.sdk.location;

import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import app.organicmaps.sdk.bookmarks.data.TrackStatistics;
public class TrackRecorder
{
  public static native void nativeStartTrackRecording();

  public static native void nativeStopTrackRecording();

  public static native void nativeSaveTrackRecordingWithName(String name);

  public static native boolean nativeIsTrackRecordingEmpty();

  public static native boolean nativeIsTrackRecordingEnabled();

  public static native void nativeSetTrackRecordingStatsListener(TrackRecorder.TrackRecordingUpdateHandler listener);

  public static native ElevationInfo nativeGetElevationInfo();

  public interface TrackRecordingUpdateHandler
  {
    void onTrackRecordingUpdate(TrackStatistics trackStatistics);
  }
}
