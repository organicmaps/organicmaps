package app.organicmaps.sdk.location;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import app.organicmaps.sdk.bookmarks.data.TrackStatistics;

public class TrackRecorder
{
  public static native void nativeStartTrackRecording();

  public static native void nativeStopTrackRecording();

  public static native void nativeDiscardTrackRecording();

  public static native boolean nativeSaveTrackRecordingWithName(String name);

  public static native boolean nativeIsTrackRecordingEnabled();

  public static native void nativeSetTrackRecordingStatsListener(TrackRecorder.TrackRecordingUpdateHandler listener);

  @Nullable
  public static native ElevationInfo nativeGetElevationInfo();

  /** Stops the recording. Returns {@code true} when it creates a track. The file is written asynchronously. */
  public static boolean saveAndStop()
  {
    return nativeSaveTrackRecordingWithName("");
  }

  public interface TrackRecordingUpdateHandler
  {
    @Keep
    @SuppressWarnings("unused")
    void onTrackRecordingUpdate(TrackStatistics trackStatistics);
  }
}
