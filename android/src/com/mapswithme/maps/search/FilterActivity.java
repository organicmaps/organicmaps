package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.util.ThemeUtils;

public class FilterActivity extends BaseMwmFragmentActivity
    implements FilterFragment.Listener, CustomNavigateUpListener
{
  public static final int REQ_CODE_FILTER = 101;
  public static final String EXTRA_FILTER = "extra_filter";
  public static final String ACTION_FILTER_APPLY = "action_filter_apply";

  public static void startForResult(@NonNull Activity activity, @Nullable HotelsFilter filter,
                                    int requestCode)
  {
    Intent i = buildFilterIntent(activity, filter);
    activity.startActivityForResult(i, requestCode);
  }

  public static void startForResult(@NonNull Fragment fragment, @Nullable HotelsFilter filter,
                                    int requestCode)
  {
    Intent i = buildFilterIntent(fragment.getActivity(), filter);
    fragment.startActivityForResult(i, requestCode);
  }

  @NonNull
  private static Intent buildFilterIntent(@NonNull Activity activity, @Nullable HotelsFilter filter)
  {
    Intent i = new Intent(activity, FilterActivity.class);
    Bundle args = new Bundle();
    args.putParcelable(FilterFragment.ARG_FILTER, filter);
    i.putExtras(args);
    return i;
  }

  @Override
  protected boolean useColorStatusBar()
  {
    return true;
  }

  @Override
  public int getThemeResourceId(@NonNull String theme)
  {
    return ThemeUtils.getCardBgThemeResourceId(theme);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return FilterFragment.class;
  }

  @Override
  public void onFilterApply(@Nullable HotelsFilter filter)
  {
    setResult(filter, ACTION_FILTER_APPLY);
  }

  private void setResult(@Nullable HotelsFilter filter, @NonNull String action)
  {
    Intent i = new Intent(action);
    i.putExtra(EXTRA_FILTER, filter);
    setResult(Activity.RESULT_OK, i);
    finish();
  }

  @Override
  public void customOnNavigateUp()
  {
    finish();
  }
}
