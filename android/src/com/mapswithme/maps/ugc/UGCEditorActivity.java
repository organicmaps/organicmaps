package com.mapswithme.maps.ugc;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.StyleRes;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.util.ThemeUtils;

public class UGCEditorActivity extends BaseMwmFragmentActivity
{
  private static final String EXTRA_FEATURE_INDEX = "extra_feature_index";
  static final String EXTRA_UGC = "extra_ugc";
  static final String EXTRA_TITLE = "extra_title";
  static final String EXTRA_AVG_RATING = "extra_avg_rating";

  public static void start(@NonNull Activity activity, @NonNull String title,
                           int featureIndex, @NonNull UGC ugc, @UGC.UGCRating int rating)
  {
    final Intent i = new Intent(activity, UGCEditorActivity.class);
    i.putExtra(EXTRA_FEATURE_INDEX, featureIndex);
    i.putExtra(EXTRA_UGC, ugc);
    i.putExtra(EXTRA_TITLE, title);
    i.putExtra(EXTRA_AVG_RATING, rating);
    activity.startActivity(i);
  }

  @Override
  @StyleRes
  public int getThemeResourceId(@NonNull String theme)
  {
    return ThemeUtils.getCardBgThemeResourceId(theme);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return UGCEditorFragment.class;
  }
}
