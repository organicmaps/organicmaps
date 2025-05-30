package app.organicmaps.widget.placepage.sections;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.bookmarks.data.Track;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.placepage.EditBookmarkFragment;
import app.organicmaps.widget.placepage.ElevationProfileViewRenderer;
import app.organicmaps.widget.placepage.PlacePageStateListener;
import app.organicmaps.widget.placepage.PlacePageViewModel;

public class PlacePageTrackFragment extends Fragment implements PlacePageStateListener,
                                                                Observer<MapObject>,
                                                                View.OnClickListener,
                                                                EditBookmarkFragment.EditBookmarkListener
{
  private PlacePageViewModel mViewModel;
  @Nullable
  private Track mTrack;
  private ElevationProfileViewRenderer mElevationProfileViewRenderer;
  private View mFrame;
  private View mElevationProfileView;
  private View editTrack;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
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
    editTrack = mFrame.findViewById(R.id.tv__track_edit);
    editTrack.setOnClickListener(this);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mViewModel.getMapObject().observe(requireActivity(), this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mViewModel.getMapObject().removeObserver(this);
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    // MapObject could be something else than a Track if the user already has the place page
    // opened and clicks on a non-Track POI.
    // This callback would be called before the fragment had time to be destroyed
    if (mapObject != null && mapObject.isTrack())
    {
      if (mTrack != null && ((Track) mapObject).getTrackId() == mTrack.getTrackId())
        return;
      mTrack = (Track) mapObject;
      if (mTrack.isElevationInfoHasValue())
      {
        mElevationProfileViewRenderer.render(mTrack);
        UiUtils.show(mElevationProfileView);
      }
      else UiUtils.hide(mElevationProfileView);
    }
  }

  @Override
  public void onClick(View view)
  {
    final FragmentActivity activity = requireActivity();
    EditBookmarkFragment.editTrack(mTrack.getCategoryId(),
                                   mTrack.getTrackId(),
                                   activity,
                                   getChildFragmentManager(),
                                   PlacePageTrackFragment.this);
  }

  @Override
  public void onBookmarkSaved(long bookmarkId, boolean movedFromCategory)
  {
    BookmarkManager.INSTANCE.updateTrackPlacePage(bookmarkId);
  }
}
