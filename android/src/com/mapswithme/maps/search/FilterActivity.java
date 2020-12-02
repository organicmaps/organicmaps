package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import com.mapswithme.maps.base.CustomNavigateUpListener;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.statistics.Statistics;

public class FilterActivity extends BaseMwmFragmentActivity
    implements FilterFragment.Listener, CustomNavigateUpListener
{
  public static final int REQ_CODE_FILTER = 101;
  public static final String EXTRA_FILTER = "extra_filter";
  public static final String ACTION_FILTER_APPLY = "action_filter_apply";

  public static void startForResult(@NonNull Activity activity, @Nullable HotelsFilter filter,
                                    @Nullable BookingFilterParams params, int requestCode)
  {
    Intent i = buildFilterIntent(activity, filter, params);
    activity.startActivityForResult(i, requestCode);
  }

  public static void startForResult(@NonNull Fragment fragment, @Nullable HotelsFilter filter,
                                    @Nullable BookingFilterParams params, int requestCode)
  {
    Intent i = buildFilterIntent(fragment.requireActivity(), filter, params);
    fragment.startActivityForResult(i, requestCode);
  }

  @NonNull
  private static Intent buildFilterIntent(@NonNull Activity activity, @Nullable HotelsFilter filter,
                                          @Nullable BookingFilterParams params)
  {
    Intent i = new Intent(activity, FilterActivity.class);
    Bundle args = new Bundle();
    args.putParcelable(FilterFragment.ARG_FILTER, filter);
    args.putParcelable(FilterFragment.ARG_FILTER_PARAMS, params);
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
    return ThemeUtils.getCardBgThemeResourceId(getApplicationContext(), theme);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return FilterFragment.class;
  }

  @Override
  public void onFilterApply(@Nullable HotelsFilter filter)
  {
    Intent i = new Intent(ACTION_FILTER_APPLY);
    i.putExtra(EXTRA_FILTER, filter);
    setResult(Activity.RESULT_OK, i);
    finish();
  }

  @Override
  public void customOnNavigateUp()
  {
    Statistics.INSTANCE.trackFilterEvent(Statistics.EventName.SEARCH_FILTER_CANCEL,
                                         Statistics.EventParam.HOTEL);
    finish();
  }

  @Override
  public void onBackPressed()
  {
    Statistics.INSTANCE.trackFilterEvent(Statistics.EventName.SEARCH_FILTER_CANCEL,
                                         Statistics.EventParam.HOTEL);
    super.onBackPressed();
  }
}
