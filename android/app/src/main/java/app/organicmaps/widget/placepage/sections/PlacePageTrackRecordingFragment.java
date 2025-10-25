package app.organicmaps.widget.placepage.sections;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.TrackRecording;
import app.organicmaps.sdk.bookmarks.data.TrackStatistics;
import app.organicmaps.sdk.location.TrackRecorder;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.placepage.ElevationProfileViewRenderer;
import app.organicmaps.widget.placepage.PlacePageStateListener;
import app.organicmaps.widget.placepage.PlacePageViewModel;

public class PlacePageTrackRecordingFragment
    extends Fragment implements PlacePageStateListener, TrackRecorder.TrackRecordingUpdateHandler
{
  private PlacePageViewModel mViewModel;
  private ElevationProfileViewRenderer mElevationProfileViewRenderer;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.placepage_track_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mElevationProfileViewRenderer = new ElevationProfileViewRenderer(view.findViewById(R.id.elevation_profile));
  }

  @Override
  public void onStart()
  {
    super.onStart();
    TrackRecorder.nativeSetTrackRecordingStatsListener(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    TrackRecorder.nativeSetTrackRecordingStatsListener(null);
  }

  @Keep
  @SuppressWarnings("unused")
  @Override
  public void onTrackRecordingUpdate(TrackStatistics trackStatistics)
  {
    if (getContext() == null)
      return;
    if (!TrackRecorder.nativeIsTrackRecordingEnabled())
      return;

    MapObject mo = mViewModel.getMapObject().getValue();
    if (mo == null || !mo.isTrackRecording())
      return;
    ((TrackRecording) mViewModel.getMapObject().getValue())
        .setTrackRecordingPPDescription(
            StringUtils.nativeFormatDistance(trackStatistics.getLength()).toString(getContext()) + " • "
            + Utils.formatRoutingTime(getContext(), (int) trackStatistics.getDuration(), R.dimen.text_size_body_3));
    ElevationInfo elevationInfo = TrackRecorder.nativeGetElevationInfo();
    // This check is needed because the elevationInfo can be null in case there are no elevation data.
    // When Track Recording has just started
    if (elevationInfo == null)
      return;
    mElevationProfileViewRenderer.render(/* track */ null, elevationInfo, trackStatistics);
  }
}
