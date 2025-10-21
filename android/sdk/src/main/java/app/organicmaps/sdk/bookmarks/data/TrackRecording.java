package app.organicmaps.sdk.bookmarks.data;

import androidx.lifecycle.MutableLiveData;
import app.organicmaps.sdk.location.TrackRecorder;
public class TrackRecording extends MapObject
{
  private final MutableLiveData<String> mTrackRecordingPPDescription = new MutableLiveData<>();
  public TrackRecording()
  {
    super(FeatureId.EMPTY, TRACK_RECORDING, "0 m • 0 min", "", "Track Recording", "", 0, 0, "", null,
          OPENING_MODE_PREVIEW, null, "", "", RoadWarningMarkType.UNKNOWN.ordinal(), null);
    mTrackRecordingPPDescription.setValue("");
  }

  public ElevationInfo getElevationInfo()
  {
    return TrackRecorder.nativeGetElevationInfo();
  }

  public void setTrackRecordingStatsListener(TrackRecorder.TrackRecordingUpdateHandler listener)
  {
    TrackRecorder.nativeSetTrackRecordingStatsListener(listener);
  }

  public MutableLiveData<String> getTrackRecordingPPDescription()
  {
    return mTrackRecordingPPDescription;
  }

  public void setTrackRecordingPPDescription(String description)
  {
    mTrackRecordingPPDescription.setValue(description);
  }
}
