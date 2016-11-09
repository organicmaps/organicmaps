package com.mapswithme.maps.bookmarks;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseToolbarActivity;
import com.mapswithme.util.ThemeUtils;

public class BookmarkCategoriesActivity extends BaseToolbarActivity
{
  @Override
  public int getThemeResourceId(String theme)
  {
    if (ThemeUtils.isDefaultTheme(theme))
      return R.style.MwmTheme_CardBg;

    if (ThemeUtils.isNightTheme(theme))
      return R.style.MwmTheme_Night_CardBg;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarkCategoriesFragment.class;
  }
}