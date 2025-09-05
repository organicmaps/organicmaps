package app.organicmaps.sdk.bookmarks.data;

import app.organicmaps.sdk.location.TrackRecorder;
public class TrackRecording extends MapObject
{
  public TrackRecording()
  {
    super(FeatureId.EMPTY, TRACK_RECORDING, "Track Recording", "", "", "", 0, 0, "", null, OPENING_MODE_PREVIEW, null,
          "", RoadWarningMarkType.UNKNOWN.ordinal(), null);
  }

  public ElevationInfo getElevationInfo()
  {
    return TrackRecorder.nativeGetElevationInfo();
  }

  public void setTrackRecordingStatsListener(TrackRecorder.TrackRecordingUpdateHandler listener)
  {
    TrackRecorder.nativeSetTrackRecordingStatsListener(listener);
  }
}
