package app.organicmaps.sdk.location;

import androidx.annotation.Keep;
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

  /** Saves the recorded track, if there is anything to save, and stops the recording. */
  public static void saveAndStop()
  {
    if (!nativeIsTrackRecordingEmpty())
      nativeSaveTrackRecordingWithName("");
    nativeStopTrackRecording();
  }

  public interface TrackRecordingUpdateHandler
  {
    @Keep
    @SuppressWarnings("unused")
    void onTrackRecordingUpdate(TrackStatistics trackStatistics);
  }
}
