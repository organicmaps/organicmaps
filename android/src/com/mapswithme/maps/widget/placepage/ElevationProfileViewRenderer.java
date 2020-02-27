package com.mapswithme.maps.widget.placepage;

import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;

import java.util.Objects;

public class ElevationProfileViewRenderer implements PlacePageViewRenderer<MapObject>
{
  private static final int MAX_DIFFICULTY_LEVEL = 3;

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

  @Override
  public void render(@NonNull MapObject object)
  {
    mTitle.setText(object.getTitle());
    // TODO: just for testing.
    setDifficulty(2);
    mAscent.setText("980 m");
    mDescent.setText("1000 m");
    mMaxAltitude.setText("2243 m");
    mMinAltitude.setText("20 m");
    mTime.setText("3 h 45 min");
  }

  @Override
  public void initialize(@Nullable View view)
  {
    Objects.requireNonNull(view);
    mTitle = view.findViewById(R.id.title);
    mAscent = view.findViewById(R.id.ascent);
    mDescent = view.findViewById(R.id.descent);
    mMaxAltitude = view.findViewById(R.id.max_altitude);
    mMinAltitude = view.findViewById(R.id.min_altitude);
    mTime = view.findViewById(R.id.time);
    mDifficultyLevels[0] = view.findViewById(R.id.difficulty_level_1);
    mDifficultyLevels[1] = view.findViewById(R.id.difficulty_level_2);
    mDifficultyLevels[2] = view.findViewById(R.id.difficulty_level_3);
  }

  @Override
  public void destroy()
  {
    // No op.
  }

  private void setDifficulty(int level)
  {
    for (View levelView : mDifficultyLevels)
      levelView.setEnabled(false);

    for (int i = 0; i < level; i++)
      mDifficultyLevels[i].setEnabled(true);
  }
}
