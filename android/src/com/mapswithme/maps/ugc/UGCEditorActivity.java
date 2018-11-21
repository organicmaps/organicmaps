package com.mapswithme.maps.ugc;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.StyleRes;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.metrics.UserActionsLogger;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.statistics.Statistics;

public class UGCEditorActivity extends BaseMwmFragmentActivity
{
  public static void start(@NonNull Activity activity, @NonNull EditParams params)
  {
    Statistics.INSTANCE.trackUGCStart(false, params.isFromPP(), params.isFromNotification());
    UserActionsLogger.logUgcEditorOpened();
    final Intent i = new Intent(activity, UGCEditorActivity.class);
    Bundle args = new Bundle();
    args.putParcelable(UGCEditorFragment.ARG_FEATURE_ID, params.getFeatureId());
    args.putString(UGCEditorFragment.ARG_TITLE, params.getTitle());
    args.putInt(UGCEditorFragment.ARG_DEFAULT_RATING, params.getDefaultRating());
    args.putParcelableArrayList(UGCEditorFragment.ARG_RATING_LIST, params.getRatings());
    args.putBoolean(UGCEditorFragment.ARG_CAN_BE_REVIEWED, params.canBeReviewed());
    args.putDouble(UGCEditorFragment.ARG_LAT, params.getLat());
    args.putDouble(UGCEditorFragment.ARG_LON, params.getLon());
    args.putString(UGCEditorFragment.ARG_ADDRESS, params.getAddress());
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

  @Override
  public void onBackPressed()
  {
    Statistics.INSTANCE.trackEvent(Statistics.EventName.UGC_REVIEW_CANCEL);
    super.onBackPressed();
  }
}
