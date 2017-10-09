package com.mapswithme.maps.ugc;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.StyleRes;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.util.ThemeUtils;

public class UGCEditorActivity extends BaseMwmFragmentActivity
{
  static final String EXTRA_FEATURE_ID = "extra_feautre_id";
  static final String EXTRA_UGC = "extra_ugc";
  static final String EXTRA_TITLE = "extra_title";
  static final String EXTRA_AVG_RATING = "extra_avg_rating";

  public static void start(@NonNull Activity activity, @NonNull String title,
                           @NonNull FeatureId featureId, @NonNull UGC ugc, @UGC.Impress int rating)
  {
    final Intent i = new Intent(activity, UGCEditorActivity.class);
    i.putExtra(EXTRA_FEATURE_ID, featureId);
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
