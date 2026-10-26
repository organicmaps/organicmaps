package app.organicmaps.sdk.location;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import app.organicmaps.sdk.bookmarks.data.TrackStatistics;
import java.util.concurrent.atomic.AtomicBoolean;

public class TrackRecorder
{
  public static native void nativeStartTrackRecording();

  public static native void nativeStopTrackRecording();

  public static native void nativeDiscardTrackRecording();

  public static native void nativeSaveTrackRecordingWithName(String name);

  public static native boolean nativeIsTrackRecordingEmpty();

  public static native boolean nativeIsTrackRecordingEnabled();

  public static native void nativeSetTrackRecordingStatsListener(TrackRecorder.TrackRecordingUpdateHandler listener);

  @Nullable
  public static native ElevationInfo nativeGetElevationInfo(@Nullable ElevationInfo previous, AtomicBoolean cancelled);

  /** Saves the recorded track, if there is anything to save, stops the recording and reports whether it saved. */
  public static boolean saveAndStop()
  {
    final boolean saved = !nativeIsTrackRecordingEmpty();
    if (saved)
    {
      nativeSaveTrackRecordingWithName("");
      nativeStopTrackRecording();
    }
    else
      nativeDiscardTrackRecording();
    return saved;
  }

  public interface TrackRecordingUpdateHandler
  {
    @Keep
    @SuppressWarnings("unused")
    void onTrackRecordingUpdate(TrackStatistics trackStatistics);
  }
}
