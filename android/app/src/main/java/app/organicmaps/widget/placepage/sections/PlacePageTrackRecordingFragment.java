package app.organicmaps.widget.placepage.sections;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.location.TrackRecordingService;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.TrackRecording;
import app.organicmaps.sdk.bookmarks.data.TrackStatistics;
import app.organicmaps.sdk.location.TrackRecorder;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.placepage.ElevationProfileViewRenderer;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import app.organicmaps.widget.placepage.RecordingElevationProfileLoader;

public class PlacePageTrackRecordingFragment extends Fragment
{
  private PlacePageViewModel mViewModel;
  private ElevationProfileViewRenderer mElevationProfileViewRenderer;
  @Nullable
  private TrackStatistics mStatistics;
  private RecordingElevationProfileLoader mProfileLoader;
  private boolean mResumed;
  private final ViewTreeObserver.OnGlobalLayoutListener mLayoutListener = this::updateGraphVisibility;

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
    mProfileLoader = new RecordingElevationProfileLoader(
        ThreadPool.getWorker(), UiThread::run,
        () -> mStatistics != null && isGraphVisible(), TrackRecorder::nativeGetElevationInfo, info -> {
          if (info != null && mStatistics != null)
            mElevationProfileViewRenderer.renderGraph(null, info, mStatistics);
          else
            mElevationProfileViewRenderer.clearGraph();
        });
    view.getViewTreeObserver().addOnGlobalLayoutListener(mLayoutListener);
    mViewModel.getPlacePageDistanceToTop().observe(getViewLifecycleOwner(), top -> updateGraphVisibility());
    mViewModel.getMapObject().observe(getViewLifecycleOwner(), object -> updateGraphVisibility());
    TrackRecordingService.getStatistics().observe(getViewLifecycleOwner(), trackStatistics -> {
      if (trackStatistics != null)
        render(trackStatistics);
    });
  }

  @Override
  public void onResume()
  {
    super.onResume();
    mResumed = true;
    updateGraphVisibility();
  }

  @Override
  public void onPause()
  {
    mResumed = false;
    mProfileLoader.setVisible(false);
    super.onPause();
  }

  @Override
  public void onDestroyView()
  {
    mProfileLoader.setVisible(false);
    requireView().getViewTreeObserver().removeOnGlobalLayoutListener(mLayoutListener);
    mStatistics = null;
    super.onDestroyView();
  }

  private void render(@NonNull TrackStatistics trackStatistics)
  {
    if (getView() == null || getContext() == null || !TrackRecorder.nativeIsTrackRecordingEnabled())
      return;
    MapObject object = mViewModel.getMapObject().getValue();
    if (object == null || !object.isTrackRecording())
      return;
    mViewModel.setTrackRecordingSaveVisible(!TrackRecorder.nativeIsTrackRecordingEmpty());
    ((TrackRecording) object)
        .setTrackRecordingPPDescription(
            StringUtils.nativeFormatDistance(trackStatistics.getLength()).toString(getContext()) + " • "
            + Utils.formatDuration(getContext(), (int) trackStatistics.getDuration(), R.dimen.text_size_body_3));
    mStatistics = trackStatistics;
    mElevationProfileViewRenderer.renderStatistics(trackStatistics);
    mProfileLoader.invalidate();
    updateGraphVisibility();
  }

  private void updateGraphVisibility()
  {
    mProfileLoader.setVisible(mStatistics != null && isGraphVisible());
  }

  private boolean isGraphVisible()
  {
    MapObject object = mViewModel.getMapObject().getValue();
    return mResumed && getView() != null && object != null && object.isTrackRecording()
 && TrackRecorder.nativeIsTrackRecordingEnabled() && mElevationProfileViewRenderer.isGraphVisible();
  }
}
