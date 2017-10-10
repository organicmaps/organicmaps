package com.mapswithme.maps.ugc;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.StyleRes;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.util.ThemeUtils;

import java.util.ArrayList;

public class UGCEditorActivity extends BaseMwmFragmentActivity
{


  public static void start(@NonNull Activity activity, @NonNull String title,
                           @NonNull FeatureId featureId, @NonNull ArrayList<UGC.Rating> ratings,
                           @UGC.Impress int defaultRating)
  {
    final Intent i = new Intent(activity, UGCEditorActivity.class);
    Bundle args = new Bundle();
    args.putParcelable(UGCEditorFragment.ARG_FEATURE_ID, featureId);
    args.putString(UGCEditorFragment.ARG_TITLE, title);
    args.putInt(UGCEditorFragment.ARG_DEFAULT_RATING, defaultRating);
    args.putParcelableArrayList(UGCEditorFragment.ARG_RATING_LIST, ratings);
    i.putExtras(args);
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
