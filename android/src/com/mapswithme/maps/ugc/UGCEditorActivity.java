package com.mapswithme.maps.ugc;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.StyleRes;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.util.ThemeUtils;

public class UGCEditorActivity extends BaseMwmFragmentActivity
{
  private static final String EXTRA_FEATURE_INDEX = "extra_feature_index";
  private static final String EXTRA_RATING = "extra_rating";

  public static void start(@NonNull Activity activity, int featureIndex, @UGC.UGCRating int rating)
  {
    final Intent i = new Intent(activity, UGCEditorActivity.class);
    i.putExtra(EXTRA_FEATURE_INDEX, featureIndex);
    i.putExtra(EXTRA_RATING, rating);
    activity.startActivity(i);
    activity.overridePendingTransition(R.anim.search_fade_in, R.anim.search_fade_out);
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

  @Override
  protected boolean useTransparentStatusBar()
  {
    return false;
  }

  @Override
  protected boolean useColorStatusBar()
  {
    return true;
  }
}
