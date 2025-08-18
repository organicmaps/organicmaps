package app.organicmaps.widget.placepage;

import android.content.Context;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.widget.NestedScrollView;
import app.organicmaps.ChartController;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.sdk.bookmarks.data.TrackStatistics;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import java.util.Objects;

public class ElevationProfileViewRenderer implements PlacePageStateListener
{
  // Must be correspond to map/elevation_info.hpp constants.
  private static final int MAX_DIFFICULTY_LEVEL = 3;
  private static final int UNKNOWN_DIFFICULTY = 0;
  @NonNull
  private final View[] mDifficultyLevels = new View[MAX_DIFFICULTY_LEVEL];
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
  private View mTitleContainer;

  public void render(@NonNull Track track)
  {
    final Context context = mAscent.getContext();
    TrackStatistics stats = track.getTrackStatistics();

    mElevationInfo = track.getElevationInfo();
    mChartController.setData(track);
    UiUtils.hide(mTitleContainer);
    mTitle.setText(track.getName());
    setDifficulty(mElevationInfo.getDifficulty());
    mAscent.setText(formatDistance(context, (int) stats.getAscent()));
    mDescent.setText(formatDistance(context, (int) stats.getDescent()));
    mMaxAltitude.setText(formatDistance(context, stats.getMaxElevation()));
    mMinAltitude.setText(formatDistance(context, stats.getMinElevation()));
    UiUtils.hide(mTimeContainer);
    mTime.setText(Utils.formatRoutingTime(mAscent.getContext(), (int) stats.getDuration(), R.dimen.text_size_body_2));
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
    mTitleContainer = view.findViewById(R.id.title_container);
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

  public void onChartElevationActivePointChanged()
  {
    mChartController.onElevationActivePointChanged();
  }

  public void onChartCurrentPositionChanged()
  {
    mChartController.onCurrentPositionChanged();
  }
}
