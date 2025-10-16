package app.organicmaps.widget.placepage;

import android.view.View;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.ChartController;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.sdk.bookmarks.data.TrackStatistics;
import app.organicmaps.util.UiUtils;

public class ElevationProfileViewRenderer implements PlacePageStateListener
{
  // Must be correspond to map/elevation_info.hpp constants.
  private static final int MAX_DIFFICULTY_LEVEL = 3;
  private static final int UNKNOWN_DIFFICULTY = 0;
  @NonNull
  private final View[] mDifficultyLevels = new View[MAX_DIFFICULTY_LEVEL];

  @NonNull
  private final ChartController mChartController;
  @NonNull
  private final TextView mAscent;
  @NonNull
  private final TextView mDescent;
  @NonNull
  private final TextView mMaxAltitude;
  @NonNull
  private final TextView mMinAltitude;
  @NonNull
  private final View mDifficultyContainer;

  public ElevationProfileViewRenderer(@NonNull View view)
  {
    mChartController = new ChartController(view);
    mAscent = view.findViewById(R.id.ascent);
    mDescent = view.findViewById(R.id.descent);
    mMaxAltitude = view.findViewById(R.id.max_altitude);
    mMinAltitude = view.findViewById(R.id.min_altitude);
    mDifficultyContainer = view.findViewById(R.id.difficulty_container);
    mDifficultyLevels[0] = mDifficultyContainer.findViewById(R.id.difficulty_level_1);
    mDifficultyLevels[1] = mDifficultyContainer.findViewById(R.id.difficulty_level_2);
    mDifficultyLevels[2] = mDifficultyContainer.findViewById(R.id.difficulty_level_3);
  }

  public void render(@Nullable Track track, @NonNull ElevationInfo elevationInfo, @NonNull TrackStatistics stats)
  {
    mChartController.setData(track, elevationInfo, stats);
    setDifficulty(elevationInfo.getDifficulty());
    mAscent.setText(Framework.nativeFormatAltitude((int) stats.getAscent()));
    mDescent.setText(Framework.nativeFormatAltitude((int) stats.getDescent()));
    mMaxAltitude.setText(Framework.nativeFormatAltitude(stats.getMaxElevation()));
    mMinAltitude.setText(Framework.nativeFormatAltitude(stats.getMinElevation()));
  }

  public void onChartElevationActivePointChanged()
  {
    mChartController.onElevationActivePointChanged();
  }

  public void onChartCurrentPositionChanged()
  {
    mChartController.onCurrentPositionChanged();
  }

  private void setDifficulty(int level)
  {
    for (View levelView : mDifficultyLevels)
      levelView.setEnabled(false);

    boolean invalidDifficulty = level > MAX_DIFFICULTY_LEVEL || level == UNKNOWN_DIFFICULTY;
    UiUtils.hideIf(invalidDifficulty, mDifficultyContainer);

    if (invalidDifficulty)
      return;

    for (int i = 0; i < level; i++)
      mDifficultyLevels[i].setEnabled(true);
  }
}
