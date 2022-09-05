package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import androidx.fragment.app.Fragment;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.util.ThemeUtils;

public class SearchActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_QUERY = "search_query";
  public static final String EXTRA_LOCALE = "locale";
  public static final String EXTRA_SEARCH_ON_MAP = "search_on_map";

  public static void start(@NonNull Activity activity, @Nullable String query)
  {
    start(activity, query, null /* locale */, false /* isSearchOnMap */);
  }

  public static void start(@NonNull Activity activity, @Nullable String query, @Nullable String locale,
                           boolean isSearchOnMap)
  {
    final Intent i = new Intent(activity, SearchActivity.class);
    Bundle args = new Bundle();
    args.putString(EXTRA_QUERY, query);
    args.putString(EXTRA_LOCALE, locale);
    args.putBoolean(EXTRA_SEARCH_ON_MAP, isSearchOnMap);
    i.putExtras(args);
    activity.startActivity(i);
  }

  @Override
  @StyleRes
  public int getThemeResourceId(@NonNull String theme)
  {
    return ThemeUtils.getCardBgThemeResourceId(getApplicationContext(), theme);
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
}
