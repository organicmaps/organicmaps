package app.organicmaps.widget.placepage.sections;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.widget.NestedScrollView;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.ChartController;
import app.organicmaps.sdk.Framework;
import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.UiUtils;
import app.organicmaps.sdk.widget.placepage.PlacePageData;
import app.organicmaps.sdk.widget.placepage.PlacePageStateListener;
import app.organicmaps.sdk.widget.placepage.PlacePageViewModel;

import java.util.Objects;

@SuppressWarnings("unused") // https://github.com/organicmaps/organicmaps/issues/2829
public class PlacePageElevationProfileFragment extends Fragment implements PlacePageStateListener,
                                                                           Observer<MapObject>
{
  // Must be correspond to map/elevation_info.hpp constants.
  private static final int MAX_DIFFICULTY_LEVEL = 3;
  private static final int UNKNOWN_DIFFICULTY = 0;

  private View mFrame;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private NestedScrollView mScrollView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mTitle;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mAscent;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mDescent;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mMaxAltitude;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mMinAltitude;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mTime;
  @NonNull
  private final View[] mDifficultyLevels = new View[MAX_DIFFICULTY_LEVEL];
  @SuppressWarnings("NullableProblems")
  @NonNull
  private ChartController mChartController;
  @Nullable
  private ElevationInfo mElevationInfo;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mDifficultyContainer;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mTimeContainer;
  private PlacePageViewModel mViewModel;
  private Track track;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.elevation_profile_bottom_sheet, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mFrame = view;
    initialize(mFrame);
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

  public void render(@NonNull PlacePageData data)
  {
    final Context context = mAscent.getContext();

    mElevationInfo = (ElevationInfo) data;
    mChartController.setData(mElevationInfo);
    mTitle.setText(mElevationInfo.getName());
    setDifficulty(mElevationInfo.getDifficulty());
    mAscent.setText(formatDistance(context, mElevationInfo.getAscent()));
    mDescent.setText(formatDistance(context, mElevationInfo.getDescent()));
    mMaxAltitude.setText(formatDistance(context, mElevationInfo.getMaxAltitude()));
    mMinAltitude.setText(formatDistance(context, mElevationInfo.getMinAltitude()));
    UiUtils.hideIf(mElevationInfo.getDuration() == 0, mTimeContainer);
    mTime.setText(RoutingController.formatRoutingTime(mTitle.getContext(), (int) mElevationInfo.getDuration(),
                                                      R.dimen.text_size_body_2));
  }

  @NonNull
  private static String formatDistance(final Context context, int distance)
  {
    return Framework.nativeFormatAltitude(distance);
  }

  public void initialize(@Nullable View view)
  {
    Objects.requireNonNull(view);
    mChartController = new ChartController(view.getContext());
    mChartController.initialize(view);
    mScrollView = (NestedScrollView) view;
    mTitle = view.findViewById(R.id.title);
    mAscent = view.findViewById(R.id.ascent);
    mDescent = view.findViewById(R.id.descent);
    mMaxAltitude = view.findViewById(R.id.max_altitude);
    mMinAltitude = view.findViewById(R.id.min_altitude);
    mTimeContainer = view.findViewById(R.id.time_container);
    mTime = mTimeContainer.findViewById(R.id.time);
    mDifficultyContainer = view.findViewById(R.id.difficulty_container);
    mDifficultyLevels[0] = mDifficultyContainer.findViewById(R.id.difficulty_level_1);
    mDifficultyLevels[1] = mDifficultyContainer.findViewById(R.id.difficulty_level_2);
    mDifficultyLevels[2] = mDifficultyContainer.findViewById(R.id.difficulty_level_3);
  }

  private void setDifficulty(int level)
  {
    for (View levelView : mDifficultyLevels)
      levelView.setEnabled(false);

    boolean invalidDifficulty = level > MAX_DIFFICULTY_LEVEL || level == UNKNOWN_DIFFICULTY;
    UiUtils.hideIf(invalidDifficulty, mDifficultyContainer);
    RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) mTimeContainer.getLayoutParams();
    params.removeRule(RelativeLayout.ALIGN_PARENT_END);
    params.removeRule(RelativeLayout.ALIGN_PARENT_RIGHT);
    params.removeRule(RelativeLayout.ALIGN_PARENT_START);
    params.removeRule(RelativeLayout.ALIGN_PARENT_LEFT);
    params.addRule(invalidDifficulty ? RelativeLayout.ALIGN_PARENT_START : RelativeLayout.ALIGN_PARENT_END);
    mTimeContainer.setLayoutParams(params);

    if (invalidDifficulty)
      return;

    for (int i = 0; i < level; i++)
      mDifficultyLevels[i].setEnabled(true);
  }

  public void onSave(@NonNull Bundle outState)
  {
    //    outState.putParcelable(PlacePageUtils.EXTRA_PLACE_PAGE_DATA, mElevationInfo);
  }

  public void onRestore(@NonNull Bundle inState)
  {
    //    mElevationInfo = BundleCompat.getParcelable(inState, PlacePageUtils.EXTRA_PLACE_PAGE_DATA,
    //    ElevationInfo.class); if (mElevationInfo != null)
    //      render(mElevationInfo);
  }

  public void onHide()
  {
    mScrollView.scrollTo(0, 0);
    mChartController.onHide();
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    // MapObject could be something else than a Track if the user already has the place page
    // opened and clicks on a non-Track POI.
    // This callback would be called before the fragment had time to be destroyed
    if (mapObject != null && mapObject.isTrack())
    {
      track = (Track) mapObject;
      render(track.getElevationInfo());
    }
  }
}
