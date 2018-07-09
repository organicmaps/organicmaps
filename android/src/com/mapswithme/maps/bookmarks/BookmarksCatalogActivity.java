package com.mapswithme.maps.bookmarks;

import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseToolbarActivity;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.util.Utils;

public class BookmarksCatalogActivity extends BaseToolbarActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarksCatalogFragment.class;
  }

  @Override
  protected boolean onBackPressedInternal(@NonNull Fragment currentFragment)
  {
    return Utils.<OnBackPressListener>castTo(currentFragment).onBackPressed();
  }
}
