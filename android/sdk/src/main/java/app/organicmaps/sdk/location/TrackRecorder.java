package app.organicmaps.sdk.location;

import androidx.annotation.IntDef;
import androidx.annotation.Keep;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import app.organicmaps.sdk.bookmarks.data.TrackStatistics;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class TrackRecorder
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({SAVE_SUCCESS, SAVE_EMPTY, SAVE_FILE_ERROR})
  public @interface SaveResult
  {}

  // Mirrors Framework::TrackRecordingSaveResult.
  public static final int SAVE_SUCCESS = 0;
  public static final int SAVE_EMPTY = 1;
  public static final int SAVE_FILE_ERROR = 2;

  public static native void nativeStartTrackRecording();

  public static native void nativeStopTrackRecording();

  public static native void nativeDiscardTrackRecording();

  public static native @SaveResult int nativeSaveTrackRecordingWithName(String name, boolean showTrack);

  public static native boolean nativeIsTrackRecordingEnabled();

  public static native void nativeSetTrackRecordingStatsListener(TrackRecorder.TrackRecordingUpdateHandler listener);

  @Nullable
  public static native ElevationInfo nativeGetElevationInfo();

  /** Saves the recording as a track. A file error leaves the recording available to retry. */
  public static @SaveResult int saveAndStop()
  {
    return saveAndStop(true /* showTrack */);
  }

  public static @SaveResult int saveAndStop(boolean showTrack)
  {
    return nativeSaveTrackRecordingWithName("", showTrack);
  }

  public interface TrackRecordingUpdateHandler
  {
    @Keep
    @SuppressWarnings("unused")
    void onTrackRecordingUpdate(TrackStatistics trackStatistics);
  }
}
