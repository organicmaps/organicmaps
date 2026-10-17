package app.organicmaps.widget.placepage.sections;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.placepage.ElevationProfileViewRenderer;
import app.organicmaps.widget.placepage.PlacePageStateListener;
import app.organicmaps.widget.placepage.PlacePageViewModel;

public class PlacePageTrackFragment extends Fragment
    implements PlacePageStateListener, Observer<MapObject>, BookmarkManager.OnElevationActivePointChangedListener,
               BookmarkManager.OnElevationCurrentPositionChangedListener
{
  private PlacePageViewModel mViewModel;
  @Nullable
  private Track mTrack;
  private ElevationProfileViewRenderer mElevationProfileViewRenderer;
  private boolean mGraphDirty;
  private boolean mResumed;
  private final ViewTreeObserver.OnGlobalLayoutListener mLayoutListener = this::updateGraph;

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
    view.getViewTreeObserver().addOnGlobalLayoutListener(mLayoutListener);
    mViewModel.getPlacePageDistanceToTop().observe(getViewLifecycleOwner(), top -> updateGraph());
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BookmarkManager.INSTANCE.setElevationActivePointChangedListener(this);
    BookmarkManager.INSTANCE.setElevationCurrentPositionChangedListener(this);
    mViewModel.getMapObject().observe(getViewLifecycleOwner(), this);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    mResumed = true;
    updateGraph();
  }

  @Override
  public void onPause()
  {
    mResumed = false;
    super.onPause();
  }

  @Override
  public void onDestroyView()
  {
    requireView().getViewTreeObserver().removeOnGlobalLayoutListener(mLayoutListener);
    mTrack = null;
    super.onDestroyView();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    BookmarkManager.INSTANCE.setElevationActivePointChangedListener(null);
    BookmarkManager.INSTANCE.setElevationCurrentPositionChangedListener(null);
    mViewModel.getMapObject().removeObserver(this);
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    // MapObject could be something else than a Track if the user already has the place page
    // opened and clicks on a non-Track POI.
    // This callback would be called before the fragment had time to be destroyed
    if (mapObject == null || !mapObject.isTrack())
    {
      mTrack = null;
      mGraphDirty = false;
      UiUtils.hide(requireView());
      return;
    }

    Track track = (Track) mapObject;
    mGraphDirty |= mTrack != track || track.isRelationTrack();
    mTrack = track;
    mElevationProfileViewRenderer.renderStatistics(track.getTrackStatistics());
    UiUtils.show(requireView());
    updateGraph();
  }

  private void updateGraph()
  {
    if (!mResumed || getView() == null || mTrack == null || !mGraphDirty
        || !mElevationProfileViewRenderer.isGraphVisible())
      return;
    mGraphDirty = false;
    var elevation = mTrack.getElevationInfo();
    if (elevation != null)
      mElevationProfileViewRenderer.renderGraph(mTrack, elevation, mTrack.getTrackStatistics());
    else
      UiUtils.hide(requireView());
  }

  @Override
  public void onElevationActivePointChanged(long trackId, double distance)
  {
    if (mTrack == null || mTrack.getTrackId() != trackId)
      return;
    mElevationProfileViewRenderer.onChartElevationActivePointChanged(distance);
    final double[] coords = mTrack.getElevationActivePointCoordinates();
    mTrack.setLat(coords[0]);
    mTrack.setLon(coords[1]);
  }

  @Override
  public void onCurrentPositionChanged(long trackId, double distance)
  {
    if (mTrack == null || mTrack.getTrackId() != trackId)
      return;
    mElevationProfileViewRenderer.onChartCurrentPositionChanged(distance);
  }
}
