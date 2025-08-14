package app.organicmaps.widget.placepage.sections;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.TrackStatistics;
import app.organicmaps.sdk.location.TrackRecorder;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.placepage.ElevationProfileViewRenderer;
import app.organicmaps.widget.placepage.PlacePageStateListener;
import app.organicmaps.widget.placepage.PlacePageViewModel;

public class PlacePageTrackRecordingFragment
    extends Fragment implements PlacePageStateListener, TrackRecorder.TrackRecordingUpdateHandler
{
  private PlacePageViewModel mViewModel;
  private ElevationProfileViewRenderer mElevationProfileViewRenderer;
  private View mFrame;
  private View mElevationProfileView;

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
    mElevationProfileViewRenderer = new ElevationProfileViewRenderer();
    mFrame = view;
    mElevationProfileView = mFrame.findViewById(R.id.elevation_profile);
    mElevationProfileViewRenderer.initialize(mElevationProfileView);
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

  @Override
  public void onTrackRecordingUpdate(TrackStatistics trackStatistics)
  {
    mElevationProfileViewRenderer.render(TrackRecorder.nativeGetElevationInfo(), trackStatistics, Utils.INVALID_ID);
    mViewModel.setTrackRecordingPPDescription(
        Framework.nativeFormatAltitude(trackStatistics.getLength()) + " • "
        + Utils.formatRoutingTime(getContext(), (int) trackStatistics.getDuration(), R.dimen.text_size_body_3));
  }
}
