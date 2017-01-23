package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StyleRes;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.NavUtils;

import com.mapswithme.maps.R;
import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.util.ThemeUtils;

public class SearchActivity extends BaseMwmFragmentActivity implements CustomNavigateUpListener
{
  public static final String EXTRA_QUERY = "search_query";
  public static final String EXTRA_HOTELS_FILTER = "hotels_filter";

  public static void start(@NonNull Activity activity, @Nullable String query,
                           @Nullable HotelsFilter filter)
  {
    final Intent i = new Intent(activity, SearchActivity.class);
    i.putExtra(EXTRA_QUERY, query);
    i.putExtra(EXTRA_HOTELS_FILTER, filter);
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
    return SearchFragment.class;
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

  @Override
  public void customOnNavigateUp()
  {
    final FragmentManager manager = getSupportFragmentManager();
    if (manager.getBackStackEntryCount() == 0)
    {
      for (Fragment fragment : manager.getFragments())
      {
        if (fragment instanceof HotelsFilterHolder)
        {
          HotelsFilter filter = ((HotelsFilterHolder) fragment).getHotelsFilter();
          if (filter != null)
          {
            Intent intent = NavUtils.getParentActivityIntent(this);
            intent.putExtra(EXTRA_HOTELS_FILTER, filter);
            NavUtils.navigateUpTo(this, intent);
            return;
          }
        }
      }
      NavUtils.navigateUpFromSameTask(this);
      overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
      return;
    }

    manager.popBackStack();
  }

  @Override
  public void onBackPressed()
  {
    for (Fragment f : getSupportFragmentManager().getFragments())
      if ((f instanceof OnBackPressListener) && ((OnBackPressListener)f).onBackPressed())
        return;

    super.onBackPressed();
    overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
  }
}
